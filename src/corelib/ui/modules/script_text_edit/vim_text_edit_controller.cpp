#include "vim_text_edit_controller.h"

#include "script_text_edit.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/document/text/text_document.h>
#include <business_layer/model/text/text_model.h>
#include <business_layer/templates/text_template.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QTextTable>

#include <limits>

namespace Ui {

namespace {

/**
 * @brief Максимальное количество повторений команды
 * @note Ограничиваем, чтобы случайно набранное число не подвесило интерфейс
 */
const int kMaxCount = 10000;

/**
 * @brief Максимальное количество цифр в числе повторений команды
 */
const int kMaxCountDigits = 5;

/**
 * @brief Максимальная глубина вложенности воспроизведения макросов
 */
const int kMaxReplayDepth = 32;

/**
 * @brief Максимальное количество нажатий, воспроизводимых одной командой
 * @note Защищает от макросов, которые вызывают сами себя
 */
const int kMaxReplayedKeys = 100000;

/**
 * @brief Максимальная длина записываемого макроса и повторяемой команды
 */
const int kMaxRecordedKeys = 100000;

/**
 * @brief Максимальное время поиска и замены, а также объединения строк с количеством
 * @note Некоторые шаблоны выполняются очень долго, а каждое объединение строк - это правка
 *       документа со всеми его корректировками, интерфейс же в это время не отвечает
 */
const int kSearchTimeoutMs = 2000;

/**
 * @brief Максимальная длина текста, вставляемого одной командой с количеством
 */
const int kMaxPastedTextLength = 1000000;

/**
 * @brief Максимальное количество строк, вставляемых одной командой с количеством
 * @note Каждая строка - это абзац документа, создание которого заметно дороже вставки текста
 */
const int kMaxPastedLines = 1000;

/**
 * @brief Выравнивание курсора при прокрутке командами z
 */
enum ScrollAlignment {
    ScrollToCenter,
    ScrollToTop,
    ScrollToBottom,
};

/**
 * @brief Модификатор, который в vim называется Ctrl
 * @note В macOS Qt сообщает о клавише Control как о Meta
 */
Qt::KeyboardModifier vimControlModifier()
{
#ifdef Q_OS_MACOS
    return Qt::MetaModifier;
#else
    return Qt::ControlModifier;
#endif
}

bool hasVimControl(QKeyEvent* _event)
{
    //
    // ... только Ctrl, без других модификаторов: Ctrl+Shift и Ctrl+Alt - это сочетания
    //     редактора, а в Windows Ctrl+Alt ещё и AltGr, которым набираются символы
    //
    const auto modifiers = _event->modifiers() & ~(Qt::KeypadModifier | Qt::GroupSwitchModifier);
    return modifiers == vimControlModifier();
}

bool isEscape(QKeyEvent* _event)
{
    return _event->key() == Qt::Key_Escape
        || (_event->key() == Qt::Key_BracketLeft && hasVimControl(_event));
}

bool isEnter(QKeyEvent* _event)
{
    return _event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter;
}

/**
 * @brief Является ли нажатие нажатием самого модификатора, или мёртвой клавиши
 */
bool isModifierKey(int _key)
{
    switch (_key) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
    case Qt::Key_AltGr:
    case Qt::Key_CapsLock:
    case Qt::Key_NumLock:
    case Qt::Key_ScrollLock:
    case Qt::Key_Super_L:
    case Qt::Key_Super_R:
    case Qt::Key_Hyper_L:
    case Qt::Key_Hyper_R:
    case Qt::Key_Mode_switch: {
        return true;
    }
    default: {
        return _key >= Qt::Key_Dead_Grave && _key <= Qt::Key_Dead_Longsolidusoverlay;
    }
    }
}

/**
 * @brief Код первого символа текста с учётом суррогатных пар
 */
uint firstCodePoint(const QString& _text)
{
    if (_text.isEmpty()) {
        return 0;
    }
    if (_text.size() > 1 && _text.at(0).isHighSurrogate() && _text.at(1).isLowSurrogate()) {
        return QChar::surrogateToUcs4(_text.at(0), _text.at(1));
    }
    return _text.at(0).unicode();
}

/**
 * @brief Вводит ли нажатие символ, с учётом раскладок, в которых символы набираются с Alt
 */
bool isCharacterInput(QKeyEvent* _event)
{
    const auto text = _event->text();
    if (text.isEmpty() || !QChar::isPrint(firstCodePoint(text))) {
        return false;
    }

    const auto modifiers
        = _event->modifiers() & ~(Qt::ShiftModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier);
    if (modifiers == Qt::NoModifier) {
        return true;
    }
#if defined(Q_OS_MACOS)
    return modifiers == Qt::AltModifier;
#elif defined(Q_OS_WIN)
    return modifiers == (Qt::ControlModifier | Qt::AltModifier);
#else
    return false;
#endif
}

/**
 * @brief Символ команды, если нажатие вводит символ
 */
QChar commandCharacter(QKeyEvent* _event)
{
    return isCharacterInput(_event) ? _event->text().at(0) : QChar();
}

/**
 * @brief Текст символа, который вводит нажатие, - для команд, принимающих любой символ, включая
 *        эмодзи, например f или r
 */
QString characterText(QKeyEvent* _event)
{
    return isCharacterInput(_event) ? _event->text() : QString();
}

/**
 * @brief Параметры сопоставления шаблона со строкой
 * @note Строку проверяем на корректность один раз, а не при каждом поиске в ней, иначе поиск
 *       всех совпадений в длинном абзаце занимает квадратичное время
 */
QRegularExpression::MatchOptions matchOptions(const QString& _text)
{
    for (int index = 0; index < _text.size(); ++index) {
        const auto character = _text.at(index);
        if (character.isHighSurrogate()) {
            if (index + 1 >= _text.size() || !_text.at(index + 1).isLowSurrogate()) {
                return QRegularExpression::NoMatchOption;
            }
            ++index;
        } else if (character.isLowSurrogate()) {
            return QRegularExpression::NoMatchOption;
        }
    }
    return QRegularExpression::DontCheckSubjectStringMatchOption;
}

/**
 * @brief Позиция через заданное количество символов в пределах строки
 * @param _count - положительное значение означает движение вперёд, отрицательное - назад
 * @param _moved - на сколько символов удалось сдвинуться
 */
int movePositionByCharacters(const QTextBlock& _block, int _position, int _count,
                             int* _moved = nullptr)
{
    const int lineStart = _block.position();
    const int lineEnd = Vim::lineEndPosition(_block);
    int position = _position;
    int moved = 0;
    for (; moved < qAbs(_count); ++moved) {
        if (_count > 0) {
            if (position >= lineEnd) {
                break;
            }
            position = Vim::nextCharacterPosition(_block, position);
        } else {
            if (position <= lineStart) {
                break;
            }
            position = Vim::previousCharacterPosition(_block, position);
        }
    }
    if (_moved != nullptr) {
        *_moved = moved;
    }
    return position;
}

/**
 * @brief Количество символов в заданном диапазоне строки
 */
int charactersCount(const QTextBlock& _block, int _from, int _to)
{
    int count = 0;
    for (int position = _from; position < _to;
         position = Vim::nextCharacterPosition(_block, position)) {
        ++count;
    }
    return count;
}

/**
 * @brief Код клавиши для символа - чтобы синтезированные нажатия выглядели, как настоящие
 */
int keyForCharacter(QChar _character)
{
    const auto code = _character.toUpper().unicode();
    return code >= 0x20 && code < 0x7f ? static_cast<int>(code) : static_cast<int>(Qt::Key_unknown);
}

/**
 * @brief Экранировать текст для использования в шаблоне поиска vim
 */
QString escapedPattern(const QString& _text)
{
    QString result;
    for (const auto& character : _text) {
        if (QStringLiteral("\\/.*$^~[").contains(character)) {
            result += '\\';
        }
        result += character;
    }
    return result;
}

/**
 * @brief Находятся ли блоки в одной ячейке таблицы, или оба вне таблиц
 * @note Редактор считает выделение, выходящее за пределы ячейки, выделением всей таблицы
 */
bool isSameContainer(const QTextBlock& _lhs, const QTextBlock& _rhs)
{
    const QTextCursor lhsCursor(_lhs);
    const QTextCursor rhsCursor(_rhs);
    const auto lhsTable = lhsCursor.currentTable();
    const auto rhsTable = rhsCursor.currentTable();
    if (lhsTable != rhsTable) {
        return false;
    }
    if (lhsTable == nullptr) {
        return true;
    }
    return lhsTable->cellAt(lhsCursor) == rhsTable->cellAt(rhsCursor);
}

/**
 * @brief Можно ли удалить разделитель абзацев между блоком и его соседом
 */
bool canMergeWith(const QTextBlock& _block, const QTextBlock& _neighbour)
{
    return _neighbour.isValid() && _neighbour.isVisible()
        && BusinessLayer::TextBlockStyle::forBlock(_neighbour)
        != BusinessLayer::TextParagraphType::PageSplitter
        && isSameContainer(_block, _neighbour);
}

/**
 * @brief Относятся ли блоки к одному абзацу, возможно, разорванному переносом на новую страницу
 */
bool isSameParagraph(const QTextBlock& _first, const QTextBlock& _second)
{
    if (_first == _second) {
        return true;
    }
    using BusinessLayer::TextBlockStyle;
    if (!_first.blockFormat().boolProperty(TextBlockStyle::PropertyIsBreakCorrectionStart)) {
        return false;
    }
    for (auto block = _first.next(); block.isValid(); block = block.next()) {
        if (block == _second) {
            return block.blockFormat().boolProperty(TextBlockStyle::PropertyIsBreakCorrectionEnd);
        }
        if (!block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)) {
            return false;
        }
    }
    return false;
}

/**
 * @brief Является ли блок заголовком, или окончанием папки
 * @note Такой блок нельзя разделить, или скопировать переводом строки, т.к. у новой части не
 *       будет пары, и структура документа сломается
 */
bool isFolderBoundary(const QTextBlock& _block)
{
    using BusinessLayer::TextParagraphType;
    switch (BusinessLayer::TextBlockStyle::forBlock(_block)) {
    case TextParagraphType::ActHeading:
    case TextParagraphType::ActFooter:
    case TextParagraphType::SequenceHeading:
    case TextParagraphType::SequenceFooter:
    case TextParagraphType::PartHeading:
    case TextParagraphType::PartFooter:
    case TextParagraphType::ChapterHeading:
    case TextParagraphType::ChapterFooter: {
        return true;
    }
    default: {
        return false;
    }
    }
}

/**
 * @brief Является ли блок окончанием папки
 */
bool isFolderFooter(const QTextBlock& _block)
{
    using BusinessLayer::TextParagraphType;
    const auto type = BusinessLayer::TextBlockStyle::forBlock(_block);
    return type == TextParagraphType::ActFooter || type == TextParagraphType::SequenceFooter
        || type == TextParagraphType::PartFooter || type == TextParagraphType::ChapterFooter;
}

/**
 * @brief Начинает ли абзац заданного типа новую группу: сцену, бит, главу и т.п.
 */
bool isGroupHeading(BusinessLayer::TextParagraphType _type)
{
    using BusinessLayer::TextParagraphType;
    switch (_type) {
    case TextParagraphType::SceneHeading:
    case TextParagraphType::BeatHeading:
    case TextParagraphType::PageHeading:
    case TextParagraphType::PanelHeading:
    case TextParagraphType::ChapterHeading1:
    case TextParagraphType::ChapterHeading2:
    case TextParagraphType::ChapterHeading3:
    case TextParagraphType::ChapterHeading4:
    case TextParagraphType::ChapterHeading5:
    case TextParagraphType::ChapterHeading6: {
        return true;
    }
    default: {
        return false;
    }
    }
}

/**
 * @brief Начертание фрагмента, вставленного заданное количество раз подряд
 * @param _step - расстояние между началами соседних копий во вставленном тексте
 */
QVector<Vim::RegisterContent::TextFormat> repeatedFormats(const Vim::RegisterContent& _content,
                                                          int _count, int _step)
{
    QVector<Vim::RegisterContent::TextFormat> formats;
    if (_content.formats.isEmpty()) {
        return formats;
    }

    formats.reserve(_content.formats.size() * _count);
    for (int index = 0; index < _count; ++index) {
        for (auto format : _content.formats) {
            format.from += index * _step;
            formats.append(format);
        }
    }
    return formats;
}

/**
 * @brief Количество пробельных символов в начале строки
 */
int leadingWhiteCount(const QString& _text)
{
    int count = 0;
    while (count < _text.size() && (_text.at(count) == ' ' || _text.at(count) == '\t')) {
        ++count;
    }
    return count;
}

} // namespace


struct VimTextEditController::SharedState {
    Vim::Registers registers;
    QHash<QChar, QVector<RecordedKey>> macros;
    QChar lastPlayedMacro;

    QString searchPattern;
    QString searchOffset;
    bool searchForward = true;

    QString lastExCommand;
    QString lastInsertedText;

    QString findCharacter;
    QChar findCommand;

    QString substitutePattern;
    QString substituteReplacement;
    QString substituteFlags;
};

VimTextEditController::SharedState& VimTextEditController::shared()
{
    static SharedState state;
    return state;
}


// ****


VimTextEditController::VimTextEditController(ScriptTextEdit* _editor)
    : m_editor(_editor)
{
}

bool VimTextEditController::isEnabled() const
{
    return m_enabled;
}

void VimTextEditController::setEnabled(bool _enabled)
{
    if (m_enabled == _enabled) {
        return;
    }

    QScopedValueRollback<int> editingRollback(m_editingDepth, m_editingDepth + 1);
    m_enabled = _enabled;
    resetPendingCommand();
    closeCommandLine();
    m_visualAnchor = -1;
    m_visualCursor = -1;
    m_recordingRegister = QChar();
    m_recordedKeys.clear();
    m_commandKeys.clear();
    m_insertRepeat = 1;
    m_mode = _enabled ? Mode::Normal : Mode::Insert;

    if (_enabled) {
        auto cursor = m_editor->textCursor();
        if (cursor.hasSelection()) {
            cursor.clearSelection();
            m_editor->setTextCursor(cursor);
        }
    }
}

VimTextEditController::Mode VimTextEditController::mode() const
{
    return m_mode;
}

QString VimTextEditController::modeName() const
{
    switch (m_mode) {
    case Mode::Insert:
        return QStringLiteral("INSERT");
    case Mode::Replace:
        return QStringLiteral("REPLACE");
    case Mode::Normal:
        return m_insertOneCommand ? QStringLiteral("(INSERT)") : QStringLiteral("NORMAL");
    case Mode::Visual:
        return QStringLiteral("VISUAL");
    case Mode::VisualLine:
        return QStringLiteral("VISUAL LINE");
    }

    return {};
}

QString VimTextEditController::statusText() const
{
    if (m_commandLineActive) {
        return m_commandLine;
    }

    if (!m_message.isEmpty()) {
        return m_message;
    }

    //
    // ... набираемая команда показывается так же, как опцией showcmd в vim
    //
    const auto pendingKeys
        = hasPendingInput() || m_awaitingInsertRegister ? m_pendingKeys : QString();
    if (!m_recordingRegister.isNull()) {
        const auto recording
            = QCoreApplication::translate("Ui::VimTextEditController", "recording @%1")
                  .arg(m_recordingRegister.toLower());
        return pendingKeys.isEmpty() ? recording : recording + QStringLiteral("  ") + pendingKeys;
    }

    return pendingKeys;
}

bool VimTextEditController::usesBlockCursor() const
{
    return m_enabled && !isInsertMode();
}

int VimTextEditController::blockCursorPosition() const
{
    return currentPosition();
}

bool VimTextEditController::handleShortcutOverride(QKeyEvent* _event) const
{
    if (!m_enabled || m_passThroughDepth > 0 || _event == nullptr) {
        return false;
    }

    if (isEscape(_event)) {
        return true;
    }

    //
    // ... пока пользователь набирает команду, все нажатия достаются нам
    //
    if (m_commandLineActive) {
        return true;
    }

    if (!hasVimControl(_event)) {
        return false;
    }

    //
    // ... в режиме вставки перехватываем только команды vim, они важнее сочетаний редактора на
    //     тех же клавишах, а остальные сочетания, например жирный текст, остаются за редактором
    //
    if (isInsertMode()) {
        return _event->key() == Qt::Key_W || _event->key() == Qt::Key_U
            || _event->key() == Qt::Key_H || _event->key() == Qt::Key_R
            || (_event->key() == Qt::Key_O && m_mode == Mode::Insert);
    }

    //
    // ... а в визуальном режиме выделение - это обычное выделение редактора, поэтому сочетания
    //     форматирования и буфера обмена работают с ним как обычно, кроме прокрутки, ведь Ctrl+D
    //     в редакторе ещё и превращает выделение в диалог в две колонки
    //
    if (isVisualMode()) {
        return _event->key() == Qt::Key_D || _event->key() == Qt::Key_U
            || isProtectedSelectionEdit(_event);
    }

    return _event->key() == Qt::Key_R || _event->key() == Qt::Key_D || _event->key() == Qt::Key_U
        || _event->key() == Qt::Key_A || _event->key() == Qt::Key_X;
}

bool VimTextEditController::handleKeyPress(QKeyEvent* _event)
{
    if (!m_enabled || m_passThroughDepth > 0 || _event == nullptr) {
        return false;
    }

    //
    // Нажатия самих модификаторов приходят отдельными событиями, они не должны сбрасывать
    // набираемую команду, иначе не получится набрать ни одной команды с заглавной буквой
    //
    if (isModifierKey(_event->key())) {
        return false;
    }

    QScopedValueRollback<int> editingRollback(m_editingDepth, m_editingDepth + 1);
    const RecordedKey recordedKey{ _event->key(), _event->modifiers(), _event->text() };
    const bool isTopLevel = m_replayDepth == 0;
    if (isTopLevel) {
        m_replayedKeys = 0;
        m_commandFailed = false;
        m_message.clear();
        m_isAutoRepeat = _event->isAutoRepeat();
        if (!m_isAutoRepeat && m_hasUncommittedChange) {
            commitUndoStep();
        }
        if (m_mode == Mode::Normal && !hasPendingInput()) {
            m_commandKeys.clear();
            m_commandKeysOverflow = false;
            m_commandRevision = document()->revision();
            m_commandIsChange = true;
        } else if (isVisualMode() && m_commandKeys.isEmpty()) {
            //
            // ... выделение мышью не повторить, поэтому и команду над ним тоже
            //
            m_commandIsChange = false;
        }
    }

    const bool wasInsertOneCommand = m_insertOneCommand;
    const bool handled = handleKeyPressImpl(_event);

    //
    // ... после команды, выполненной по Ctrl-O, возвращаемся в режим вставки
    //
    if (m_insertOneCommand) {
        if (m_mode != Mode::Normal) {
            m_insertOneCommand = !isInsertMode();
        } else if (wasInsertOneCommand && !hasPendingInput()) {
            m_insertOneCommand = false;
            int position = m_editor->textCursor().position();
            if (m_preferLineEnd) {
                position = Vim::lineEndPosition(currentBlock());
            }
            QTextCursor cursor(document());
            cursor.setPosition(position);
            m_editor->setTextCursor(cursor);
            enterInsertMode(1, 'i');
        }
    }

    if (isTopLevel) {
        if ((hasPendingInput() || m_awaitingInsertRegister) && !m_commandLineActive) {
            const auto text = _event->text();
            const auto code = text.isEmpty() ? 0 : text.at(0).unicode();
            if (code > 0 && code < 0x20) {
                m_pendingKeys += QStringLiteral("^") + QChar(code + 0x40);
            } else {
                m_pendingKeys += text;
            }
        } else {
            m_pendingKeys.clear();
        }
        trackKeyPress(recordedKey);
    }
    return handled;
}

bool VimTextEditController::handleInputMethodEvent(QInputMethodEvent* _event)
{
    if (!m_enabled || m_passThroughDepth > 0 || _event == nullptr) {
        return false;
    }

    //
    // В режиме вставки текст вводит сам редактор, но его нужно запомнить так же, как набранный
    // с клавиатуры, для повтора, макросов и регистра .
    //
    if (m_mode == Mode::Insert) {
        const auto commitString = _event->commitString();
        for (int index = 0; index < commitString.size(); ++index) {
            auto text = QString(commitString.at(index));
            if (commitString.at(index).isHighSurrogate() && index + 1 < commitString.size()) {
                text += commitString.at(++index);
            }
            const RecordedKey key{ keyForCharacter(text.at(0)), Qt::NoModifier, text };
            m_insertedText += text;
            if (!m_replayingInsert && m_insertRepeat > 1
                && m_insertKeys.size() < kMaxRecordedKeys) {
                m_insertKeys.append(key);
            }
            if (m_replayDepth == 0) {
                trackKeyPress(key);
            }
        }
        return false;
    }

    //
    // В командных режимах символы, набранные через метод ввода (например с мёртвыми клавишами),
    // обрабатываем так же, как обычные нажатия
    //
    const auto commitString = _event->commitString();
    for (int index = 0; index < commitString.size(); ++index) {
        auto text = QString(commitString.at(index));
        if (commitString.at(index).isHighSurrogate() && index + 1 < commitString.size()) {
            text += commitString.at(++index);
        }
        QKeyEvent keyEvent(QEvent::KeyPress, keyForCharacter(text.at(0)), Qt::NoModifier, text);
        handleKeyPress(&keyEvent);
    }
    _event->accept();
    return true;
}

void VimTextEditController::cancelPendingInput()
{
    if (!m_enabled) {
        return;
    }

    if (m_commandLineActive) {
        closeCommandLine();
    }
    m_awaitingInsertRegister = false;
    if (!isInsertMode()) {
        resetPendingCommand();
    }
    m_pendingKeys.clear();
}

void VimTextEditController::handleSelectionChange()
{
    if (!m_enabled || m_editingDepth > 0 || m_passThroughDepth > 0 || isInsertMode()) {
        return;
    }

    //
    // Выделение, сделанное мышью, или самим редактором, переводит в режим VISUAL, а его
    // снятие - обратно в NORMAL
    //
    const auto cursor = m_editor->textCursor();
    if (cursor.hasSelection()) {
        const int anchor = cursor.anchor();
        const int position = cursor.position();
        resetPendingCommand();
        if (!isVisualMode()) {
            m_mode = Mode::Visual;
        }
        m_visualAnchor = anchor > position ? anchor - 1 : anchor;
        m_visualCursor = position > anchor ? position - 1 : position;
    } else if (isVisualMode()) {
        leaveVisualMode();
    }
}


// ****


bool VimTextEditController::isVisualMode() const
{
    return m_mode == Mode::Visual || m_mode == Mode::VisualLine;
}

bool VimTextEditController::hasPendingInput() const
{
    return m_awaiting != Awaiting::None || m_pendingOperator != Operator::None || !m_count.isEmpty()
        || !m_pendingRegister.isNull() || m_commandLineActive;
}

void VimTextEditController::resetPendingCommand()
{
    m_awaiting = Awaiting::None;
    m_pendingOperator = Operator::None;
    m_operatorCount = 0;
    m_count.clear();
    m_pendingRegister = QChar();
    m_textObjectAround = false;
}

bool VimTextEditController::isInsertMode() const
{
    return m_mode == Mode::Insert || m_mode == Mode::Replace;
}

void VimTextEditController::enterInsertMode(int _repeat, QChar _command)
{
    m_autoIndentLine = QTextCursor();
    if (isVisualMode()) {
        leaveVisualMode();
    }
    resetPendingCommand();
    m_mode = _command == 'R' ? Mode::Replace : Mode::Insert;
    m_insertRepeat = qBound(1, _repeat, kMaxCount);
    m_insertCommand = _command;
    m_insertKeys.clear();
    m_replacedCharacters.clear();
    m_insertedText.clear();
    m_insertStart = QTextCursor(document());
    m_insertStart.setPosition(m_editor->textCursor().position());
    m_insertStart.setKeepPositionOnInsert(true);
    m_preferredX = -1;
    m_preferLineEnd = false;
}

void VimTextEditController::leaveInsertMode()
{
    //
    // Убираем отступ, оставшийся от изменения строк, если ничего не было введено
    //
    if (!m_autoIndentLine.isNull() && m_autoIndentRevision == document()->revision()) {
        const auto block = m_autoIndentLine.block();
        if (!block.text().isEmpty() && block.text().trimmed().isEmpty()) {
            const int blockPosition = block.position();
            removeText(blockPosition, Vim::lineEndPosition(block));
            QTextCursor cursor(document());
            cursor.setPosition(blockPosition);
            m_editor->setTextCursor(cursor);
        }
    }
    m_autoIndentLine = QTextCursor();

    //
    // Повторяем набранный текст, если команда вставки была с количеством
    //
    if (m_insertRepeat > 1 && !m_insertKeys.isEmpty() && canEdit()) {
        const auto keys = m_insertKeys;

        //
        // ... обычный текст вставляем сразу целиком, т.к. повторять нажатия долго
        //
        QString text;
        bool isPlainText
            = m_mode == Mode::Insert && m_insertCommand != 'o' && m_insertCommand != 'O';
        for (const auto& key : keys) {
            const auto modifiers = key.modifiers
                & ~(Qt::ShiftModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier);
            if (!isPlainText || key.text.isEmpty() || modifiers != Qt::NoModifier
                || !QChar::isPrint(firstCodePoint(key.text))) {
                isPlainText = false;
                break;
            }
            text += key.text;
        }
        if (isPlainText) {
            const int count = m_insertRepeat - 1;
            if (static_cast<qint64>(text.size()) * count > kMaxPastedTextLength) {
                fail(QCoreApplication::translate("Ui::VimTextEditController",
                                                 "E1240: Resulting text too long"));
            } else {
                auto cursor = m_editor->textCursor();
                const int position = cursor.position();
                cursor.insertText(text.repeated(count));
                m_editor->setTextCursor(cursor);
                m_editor->markInsertedText(position, cursor.position());
            }
        } else {
            QScopedValueRollback<bool> replayRollback(m_replayingInsert, true);
            for (int index = 1; index < m_insertRepeat && !m_commandFailed; ++index) {
                if (m_insertCommand == 'o' || m_insertCommand == 'O') {
                    openLine(true);
                }
                replayKeys(keys);
            }
        }
    }
    m_insertRepeat = 1;
    m_insertCommand = QChar();
    m_insertKeys.clear();
    m_replacedCharacters.clear();
    m_awaitingInsertRegister = false;
    m_insertOneCommand = false;
    if (!m_insertedText.isEmpty()) {
        shared().lastInsertedText = m_insertedText;
    }

    m_mode = Mode::Normal;
    resetPendingCommand();

    //
    // ... курсор, как и в vim, встаёт на последний введённый символ
    //
    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    m_lastInsertPosition = QTextCursor(document());
    m_lastInsertPosition.setPosition(cursor.position());
    if (!cursor.atBlockStart()) {
        cursor.movePosition(QTextCursor::PreviousCharacter);
    }
    m_editor->setTextCursor(cursor);
    m_preferredX = -1;
    m_preferLineEnd = false;
}

bool VimTextEditController::handleInsertControl(QKeyEvent* _event)
{
    switch (_event->key()) {
    case Qt::Key_H: {
        if (m_mode == Mode::Replace) {
            QKeyEvent backspace(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier,
                                QStringLiteral("\b"));
            return handleReplaceKey(&backspace);
        }
        sendKeyPressToEditor(Qt::Key_Backspace, QStringLiteral("\b"));
        return true;
    }

    case Qt::Key_W:
    case Qt::Key_U: {
        if (canEdit()) {
            deleteBeforeCursor(_event->key() == Qt::Key_W);
        }
        return true;
    }

    case Qt::Key_R: {
        m_awaitingInsertRegister = true;
        return true;
    }

    case Qt::Key_O: {
        if (m_mode != Mode::Insert) {
            return false;
        }

        //
        // ... выполняем одну команду обычного режима, оставляя курсор на месте, а затем
        //     возвращаемся в режим вставки
        //
        if (m_editor->isCompleterVisible()) {
            m_editor->closeCompleter();
        }
        m_autoIndentLine = QTextCursor();
        m_insertRepeat = 1;
        m_insertKeys.clear();
        if (!m_insertedText.isEmpty()) {
            shared().lastInsertedText = m_insertedText;
        }
        m_mode = Mode::Normal;
        m_insertOneCommand = true;
        resetPendingCommand();
        return true;
    }

    default: {
        return false;
    }
    }
}

bool VimTextEditController::handleReplaceKey(QKeyEvent* _event)
{
    auto cursor = m_editor->textCursor();
    const int position = cursor.position();
    const auto block = document()->findBlock(position);

    //
    // Backspace возвращает заменённый символ, а за началом замены просто сдвигает курсор
    //
    if (_event->key() == Qt::Key_Backspace) {
        if (position > block.position()) {
            const int previous = Vim::previousCharacterPosition(block, position);
            if (!m_replacedCharacters.isEmpty() && canEdit()) {
                replaceText(previous, position, m_replacedCharacters.takeLast());
            }
            cursor.setPosition(previous);
            m_editor->setTextCursor(cursor);
        }
        return true;
    }

    //
    // ... остальные служебные клавиши, например перенос строки, обрабатывает сам редактор
    //
    const auto text = characterText(_event);
    if (text.isEmpty()) {
        m_replacedCharacters.clear();
        return false;
    }
    if (!canEdit()) {
        return true;
    }

    const int lineEnd = Vim::lineEndPosition(block);
    int textPosition = position;
    if (position < lineEnd) {
        const int next = Vim::nextCharacterPosition(block, position);
        m_replacedCharacters.append(block.text().mid(position - block.position(), next - position));
        if (!replaceText(position, next, text, &textPosition)) {
            return true;
        }
    } else {
        m_replacedCharacters.append(QString());
        if (!insertText(position, text)) {
            return true;
        }
    }
    cursor.setPosition(textPosition + text.size());
    m_editor->setTextCursor(cursor);
    return true;
}

void VimTextEditController::deleteBeforeCursor(bool _word)
{
    const int position = m_editor->textCursor().position();
    const auto block = document()->findBlock(position);

    //
    // ... в начале строки, как и в vim, она объединяется с предыдущей
    //
    if (position <= block.position()) {
        sendKeyPressToEditor(Qt::Key_Backspace, QStringLiteral("\b"));
        return;
    }

    int start = block.position();
    if (_word) {
        //
        // ... слово перед курсором вместе с пробелами за ним
        //
        start = position;
        auto previousClass = [&block, &start] {
            const Vim::Cursor previous(block.document(),
                                       Vim::previousCharacterPosition(block, start));
            return previous.characterClass(false);
        };
        while (start > block.position() && previousClass() == 0) {
            start = Vim::previousCharacterPosition(block, start);
        }
        if (start > block.position()) {
            const int wordClass = previousClass();
            while (start > block.position() && previousClass() == wordClass) {
                start = Vim::previousCharacterPosition(block, start);
            }
        }
    } else {
        const int firstNonBlank = Vim::firstNonBlankPosition(block);
        start = position > firstNonBlank ? firstNonBlank : block.position();
    }

    //
    // ... и как в vim, удаление один раз останавливается в начале вставки
    //
    if (!m_insertStart.isNull() && m_insertStart.block() == block
        && m_insertStart.position() < position && m_insertStart.position() > start) {
        start = m_insertStart.position();
    }

    removeText(start, position);
    QTextCursor cursor(document());
    cursor.setPosition(start);
    m_editor->setTextCursor(cursor);
}

void VimTextEditController::enterNormalMode()
{
    if (isVisualMode()) {
        clampVisualPositions();
        const int position = characterPosition(m_visualCursor);
        leaveVisualMode();
        setCursorPosition(position);
    }
    resetPendingCommand();
    m_mode = Mode::Normal;
}

void VimTextEditController::enterVisualMode(Mode _mode, int _anchor, int _cursor)
{
    resetPendingCommand();
    m_mode = _mode;
    m_visualAnchor = _anchor;
    m_visualCursor = _cursor;
    updateVisualSelection();
}

void VimTextEditController::leaveVisualMode()
{
    clampVisualPositions();

    //
    // ... запоминаем выделение, чтобы его можно было восстановить командой gv
    //
    if (m_visualAnchor >= 0 && m_visualCursor >= 0) {
        m_lastVisualAnchor = QTextCursor(document());
        m_lastVisualAnchor.setPosition(m_visualAnchor);
        m_lastVisualCursor = QTextCursor(document());
        m_lastVisualCursor.setPosition(m_visualCursor);
        m_lastVisualMode = isVisualMode() ? m_mode : m_lastVisualMode;
        //
        // ... при построчном выделении метки стоят в начале первой и в конце последней строки
        //
        const int start = qMin(m_visualAnchor, m_visualCursor);
        const int end = qMax(m_visualAnchor, m_visualCursor);
        if (m_mode == Mode::VisualLine) {
            setMark('<', Vim::navigableBlock(document(), start).position());
            setMark('>',
                    characterPosition(Vim::lineEndPosition(Vim::navigableBlock(document(), end))));
        } else {
            setMark('<', start);
            setMark('>', end);
        }
    }

    m_visualAnchor = -1;
    m_visualCursor = -1;
    m_mode = Mode::Normal;
    resetPendingCommand();

    //
    // ... как и в vim, после выделения курсор снова держит свою колонку, а не конец строки
    //
    m_preferredX = -1;
    m_preferLineEnd = false;

    auto cursor = m_editor->textCursor();
    if (cursor.hasSelection()) {
        cursor.clearSelection();
        m_editor->setTextCursor(cursor);
    }
}

void VimTextEditController::restoreLastVisualSelection()
{
    if (m_lastVisualAnchor.isNull() || m_lastVisualCursor.isNull()) {
        resetPendingCommand();
        return;
    }

    enterVisualMode(m_lastVisualMode, m_lastVisualAnchor.position(), m_lastVisualCursor.position());
}

void VimTextEditController::clampVisualPositions()
{
    //
    // Документ мог измениться не командой vim, например кнопкой на панели инструментов, и
    // границы выделения могли оказаться за его концом
    //
    const int maxPosition = qMax(0, document()->characterCount() - 1);
    m_visualAnchor = qMin(m_visualAnchor, maxPosition);
    m_visualCursor = qMin(m_visualCursor, maxPosition);
}

void VimTextEditController::updateVisualSelection()
{
    clampVisualPositions();
    const auto range = visualRange();
    if (!range.isValid()) {
        return;
    }

    QTextCursor cursor(document());
    if (m_visualCursor < m_visualAnchor) {
        cursor.setPosition(range.end);
        cursor.setPosition(range.start, QTextCursor::KeepAnchor);
    } else {
        cursor.setPosition(range.start);
        cursor.setPosition(range.end, QTextCursor::KeepAnchor);
    }
    m_editor->setTextCursor(cursor);
}

int VimTextEditController::visualLinesCount() const
{
    const auto firstBlock = Vim::navigableBlock(document(), qMin(m_visualAnchor, m_visualCursor));
    const auto lastBlock = Vim::navigableBlock(document(), qMax(m_visualAnchor, m_visualCursor));
    int lines = 1;
    for (auto block = firstBlock; block.isValid() && block != lastBlock;
         block = Vim::nextNavigableBlock(block)) {
        ++lines;
    }
    return lines;
}

Vim::TextRange VimTextEditController::visualRange() const
{
    if (!isVisualMode() || m_visualAnchor < 0 || m_visualCursor < 0) {
        return {};
    }

    if (m_mode == Mode::VisualLine) {
        return linesRange(m_visualAnchor, m_visualCursor);
    }

    //
    // Символ под курсором входит в выделение, а в конце строки - и сам перевод строки
    //
    const int start = qMin(m_visualAnchor, m_visualCursor);
    const Vim::Cursor last(document(), qMax(m_visualAnchor, m_visualCursor));
    int end = last.position() + 1;
    if (last.atLineEnd()) {
        const auto next = last.block().next();
        end = next.isValid() && next.isVisible() && isSameContainer(last.block(), next)
            ? next.position()
            : last.position();
    } else {
        Vim::Cursor next = last;
        next.inc();
        end = next.block() == last.block() ? next.position() : Vim::lineEndPosition(last.block());
    }
    return { Vim::Cursor(document(), start).position(), end, false };
}


// ****


void VimTextEditController::appendCount(QChar _digit)
{
    if (m_count.size() >= kMaxCountDigits) {
        return;
    }

    m_count.append(_digit);
}

bool VimTextEditController::hasCount() const
{
    return !m_count.isEmpty() || m_operatorCount > 0;
}

int VimTextEditController::takeCount()
{
    const int count = m_count.isEmpty() ? 1 : m_count.toInt();
    const int operatorCount = qMax(1, m_operatorCount);
    m_count.clear();
    m_operatorCount = 0;
    return qBound(1, count * operatorCount, kMaxCount);
}

int VimTextEditController::takeOptionalCount()
{
    if (!hasCount()) {
        return 0;
    }

    return takeCount();
}


// ****


bool VimTextEditController::handleKeyPressImpl(QKeyEvent* _event)
{
    if (isEscape(_event)) {
        //
        // Сперва закрываем строку ввода команды и подстановщика, если они показаны, и только
        // следующим нажатием выходим из режима вставки
        //
        if (m_commandLineActive) {
            closeCommandLine();
            m_commandIsChange = false;
        } else if (m_awaitingInsertRegister) {
            m_awaitingInsertRegister = false;
        } else if (m_mode == Mode::Insert) {
            //
            // ... как и в vim, Esc закрывает подсказки и сразу выходит из режима вставки
            //
            if (m_editor->isCompleterVisible()) {
                m_editor->closeCompleter();
            }
            leaveInsertMode();
        } else if (m_mode == Mode::Replace) {
            leaveInsertMode();
        } else if (isVisualMode()) {
            enterNormalMode();
        } else {
            resetPendingCommand();
        }
        finishHandledEvent(_event);
        return true;
    }

    if (m_commandLineActive) {
        handleCommandLine(_event);
        finishHandledEvent(_event);
        return true;
    }

    if (isInsertMode()) {
        //
        // ... запоминаем набранное, чтобы повторить его для команд вставки с количеством
        //
        if (!m_replayingInsert && m_insertRepeat > 1 && m_insertKeys.size() < kMaxRecordedKeys) {
            m_insertKeys.append({ _event->key(), _event->modifiers(), _event->text() });
        }

        //
        // ... имя регистра для вставки его содержимого
        //
        if (m_awaitingInsertRegister) {
            m_awaitingInsertRegister = false;
            const auto name = commandCharacter(_event);
            const auto content
                = Vim::Registers::isValidName(name) ? registerValue(name) : Vim::RegisterContent();
            if (content.isEmpty()) {
                fail();
            } else if (canEdit()) {
                const int position = m_editor->textCursor().position();
                if (insertText(position, content.text)) {
                    QTextCursor cursor(document());
                    cursor.setPosition(position + content.text.size());
                    m_editor->setTextCursor(cursor);
                    m_insertedText += content.text;

                    //
                    // ... при повторе вставляется тот же текст, а не новое содержимое регистра
                    //
                    m_insertedRegisterText = content.text;
                }
            }
            finishHandledEvent(_event);
            return true;
        }

        if (hasVimControl(_event) && handleInsertControl(_event)) {
            finishHandledEvent(_event);
            return true;
        }

        //
        // ... запоминаем набранный текст для регистра .
        //
        if (isEnter(_event)) {
            m_insertedText += '\n';
        } else if (_event->key() == Qt::Key_Backspace) {
            m_insertedText.chop(1);
        } else {
            m_insertedText += characterText(_event);
        }

        if (m_mode == Mode::Replace && handleReplaceKey(_event)) {
            finishHandledEvent(_event);
            return true;
        }
        return false;
    }

    //
    // ... вырезать или заменить вставкой выделение со скрытым текстом нельзя, как и командами vim
    //
    if (isProtectedSelectionEdit(_event)) {
        fail();
        finishHandledEvent(_event);
        return true;
    }

    //
    // ... а скопированное сочетанием редактора выделение, как и в vim, завершает визуальный режим
    //
    if (isVisualMode() && _event->matches(QKeySequence::Copy)) {
        m_editor->copy();
        enterNormalMode();
        finishHandledEvent(_event);
        return true;
    }

    if (hasVimControl(_event)
        && (!isVisualMode() || _event->key() == Qt::Key_D || _event->key() == Qt::Key_U)) {
        switch (_event->key()) {
        case Qt::Key_R: {
            redo(takeCount());
            resetPendingCommand();
            finishHandledEvent(_event);
            return true;
        }
        case Qt::Key_D:
        case Qt::Key_U: {
            scrollHalfPage(_event->key() == Qt::Key_D, takeOptionalCount());
            resetPendingCommand();
            finishHandledEvent(_event);
            return true;
        }
        case Qt::Key_A:
        case Qt::Key_X: {
            const int count = takeCount();
            const bool canIncrement = !isVisualMode() && m_pendingOperator == Operator::None;
            resetPendingCommand();
            if (canIncrement) {
                incrementNumber(_event->key() == Qt::Key_A ? count : -count);
            } else {
                fail();
            }
            finishHandledEvent(_event);
            return true;
        }
        default: {
            //
            // ... остальные сочетания, например копирование или отмену, обрабатывает редактор
            //
            resetPendingCommand();
            return false;
        }
        }
    }

    //
    // ... прочие сочетания с модификаторами отдаём редактору и приложению
    //
    if (!isCharacterInput(_event)
        && (_event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
        return false;
    }

    const bool handled = handleCommand(_event);
    if (handled) {
        finishHandledEvent(_event);
    }
    return handled;
}

bool VimTextEditController::handleCommand(QKeyEvent* _event)
{
    if (m_awaiting != Awaiting::None) {
        return handleAwaitingInput(_event);
    }

    const auto character = commandCharacter(_event);
    if (character.unicode() >= '0' && character.unicode() <= '9'
        && (character != '0' || !m_count.isEmpty())) {
        appendCount(character);
        return true;
    }

    if (m_pendingOperator != Operator::None) {
        return handleOperatorTarget(_event);
    }

    if (handleCommonCommand(_event)) {
        return true;
    }

    return isVisualMode() ? handleVisualCommand(_event) : handleNormalCommand(_event);
}

bool VimTextEditController::handleCommonCommand(QKeyEvent* _event)
{
    const auto character = commandCharacter(_event);

    if (isMotionKey(_event)) {
        const bool withCount = hasCount();
        const int count = takeCount();
        applyMotion(motionForKey(_event, count, withCount));
        return true;
    }

    switch (character.unicode()) {
    case 'f':
    case 'F':
    case 't':
    case 'T': {
        m_findCommand = character;
        m_awaiting = Awaiting::FindCharacter;
        return true;
    }

    case '`':
    case '\'': {
        m_jumpToMarkExact = character == '`';
        m_awaiting = Awaiting::JumpToMark;
        return true;
    }

    case 'g': {
        m_awaiting = Awaiting::GoCommand;
        return true;
    }

    case 'z': {
        m_awaiting = Awaiting::ScrollCommand;
        return true;
    }

    case '"': {
        m_awaiting = Awaiting::RegisterName;
        return true;
    }

    case 'm': {
        m_awaiting = Awaiting::SetMark;
        return true;
    }

    case 'q': {
        if (!m_recordingRegister.isNull()) {
            toggleMacroRecording(m_recordingRegister);
            resetPendingCommand();
        } else {
            m_awaiting = Awaiting::RecordRegister;
        }
        m_commandIsChange = false;
        return true;
    }

    case '/':
    case '?': {
        openCommandLine(QString(character));
        return true;
    }

    case ':': {
        m_commandIsChange = false;
        if (isVisualMode()) {
            const auto range = visualRange();
            leaveVisualMode();
            setCursorPosition(characterPosition(range.start));
            openCommandLine(QStringLiteral(":'<,'>"));
        } else if (hasCount()) {
            const int count = takeCount();
            openCommandLine(count > 1 ? QStringLiteral(":.,.+%1").arg(count - 1)
                                      : QStringLiteral(":."));
        } else {
            openCommandLine(QStringLiteral(":"));
        }
        return true;
    }

    default: {
        return false;
    }
    }
}

bool VimTextEditController::handleNormalCommand(QKeyEvent* _event)
{
    const auto character = commandCharacter(_event);

    //
    // Клавиша Delete работает как x, а при наборе количества стирает его последнюю цифру
    //
    if (_event->key() == Qt::Key_Delete && _event->modifiers() == Qt::NoModifier) {
        if (!m_count.isEmpty()) {
            m_count.chop(1);
            return true;
        }
        const int count = takeCount();
        deleteCharacters(count, false);
        resetPendingCommand();
        return true;
    }

    if (character.isNull()) {
        resetPendingCommand();

        //
        // Tab в пустом абзаце меняет его тип, как и в редакторе, а в абзаце с текстом редактор
        // по Tab добавляет новые абзацы, чего в обычном режиме не ждёшь
        //
        if (_event->key() == Qt::Key_Tab || _event->key() == Qt::Key_Backtab) {
            if (!canEdit() || currentBlock().length() > 1) {
                fail();
                return true;
            }
            sendKeyPressToEditor(_event->key(), _event->text(), _event->modifiers());
            return true;
        }

        //
        // Служебные клавиши, вроде листания страниц, отдаём на откуп самому редактору,
        // а всё, что вводит текст, поглощаем, чтобы не портить документ
        //
        return !_event->text().isEmpty();
    }

    switch (character.unicode()) {
    case 'i':
    case 'a':
    case 'I':
    case 'A':
    case 'R': {
        const int count = takeCount();
        if (!canEdit()) {
            resetPendingCommand();
            return true;
        }
        const auto block = currentBlock();
        if (character == 'I') {
            setCursorPosition(Vim::firstNonBlankPosition(block));
        } else if (character == 'A') {
            setCursorPosition(Vim::lineEndPosition(block));
        } else if (character == 'a' && block.length() > 1) {
            setCursorPosition(Vim::nextCharacterPosition(block, currentPosition()));
        } else {
            setCursorPosition(currentPosition());
        }
        enterInsertMode(count, character);
        return true;
    }

    case 'o':
    case 'O': {
        const int count = takeCount();
        if (!canEdit()) {
            resetPendingCommand();
            return true;
        }
        if (!openLine(character == 'o')) {
            resetPendingCommand();
            fail();
            return true;
        }
        enterInsertMode(count, character);
        return true;
    }

    case 'v':
    case 'V': {
        //
        // ... выделение с количеством сразу захватывает заданное количество символов или строк
        //
        const int count = takeCount();
        const int position = currentPosition();
        int cursorPosition = position;
        if (character == 'v' && count > 1) {
            const auto block = currentBlock();
            const int lineEnd = Vim::lineEndPosition(block);
            for (int index = 1; index < count && cursorPosition < lineEnd; ++index) {
                cursorPosition = Vim::nextCharacterPosition(block, cursorPosition);
            }
        } else if (character == 'V' && count > 1) {
            const int column = position - currentBlock().position();
            auto block = currentBlock();
            for (int index = 1; index < count; ++index) {
                const auto next = Vim::nextNavigableBlock(block);
                if (!next.isValid()) {
                    break;
                }
                block = next;
            }
            cursorPosition = characterPosition(
                block.position() + qMin(column, Vim::lineEndPosition(block) - block.position()));
        }
        enterVisualMode(character == 'V' ? Mode::VisualLine : Mode::Visual, position,
                        cursorPosition);
        return true;
    }

    case 'd':
    case 'c':
    case 'y': {
        m_pendingOperator = character == 'd'
            ? Operator::Delete
            : (character == 'c' ? Operator::Change : Operator::Yank);
        m_operatorCount = takeOptionalCount();
        return true;
    }

    case 'x':
    case 'X': {
        const int count = takeCount();
        deleteCharacters(count, character == 'X');
        resetPendingCommand();
        return true;
    }

    case 's': {
        const int count = takeCount();
        if (!canEdit()) {
            resetPendingCommand();
            return true;
        }
        const int position = currentPosition();
        const int end = movePositionByCharacters(currentBlock(), position, count);
        changeRange({ position, end, false }, m_pendingRegister, false);
        return true;
    }

    case 'S':
    case 'C':
    case 'D':
    case 'Y': {
        const int count = takeCount();
        const int position = currentPosition();
        //
        // ... с количеством команды захватывают и следующие строки
        //
        auto lastBlock = currentBlock();
        for (int index = 1; index < count; ++index) {
            const auto next = Vim::nextNavigableBlock(lastBlock);
            if (!next.isValid()) {
                break;
            }
            lastBlock = next;
        }

        if (character == 'Y') {
            yankRange(linesRange(position, lastBlock.position()), m_pendingRegister, position);
        } else if (character == 'S') {
            changeRange(linesRange(position, lastBlock.position()), m_pendingRegister, false);
        } else {
            const Vim::TextRange range{ position, Vim::lineEndPosition(lastBlock), false };
            if (character == 'C') {
                changeRange(range, m_pendingRegister, false);
            } else {
                deleteRange(range, m_pendingRegister, false);
            }
        }
        if (m_mode != Mode::Insert) {
            resetPendingCommand();
        }
        return true;
    }

    case 'p':
    case 'P': {
        const int count = takeCount();
        paste(m_pendingRegister, character == 'P', count);
        resetPendingCommand();
        return true;
    }

    case 'r': {
        m_awaiting = Awaiting::ReplaceCharacter;
        return true;
    }

    case 'J': {
        const int count = takeCount();
        joinLines(count, true);
        resetPendingCommand();
        return true;
    }

    case '~': {
        const int count = takeCount();
        toggleCase(count);
        resetPendingCommand();
        return true;
    }

    case 'u': {
        const int count = takeCount();
        undo(count);
        resetPendingCommand();
        return true;
    }

    case '.': {
        const int count = takeOptionalCount();
        repeatLastChange(count);
        resetPendingCommand();
        return true;
    }

    case '@': {
        m_awaiting = Awaiting::PlayRegister;
        return true;
    }

    case '&': {
        resetPendingCommand();
        const auto block = currentBlock();
        substitute(block, block, QString());
        return true;
    }

    default: {
        resetPendingCommand();
        fail();
        return true;
    }
    }
}

bool VimTextEditController::handleVisualCommand(QKeyEvent* _event)
{
    const auto character = commandCharacter(_event);
    const auto registerName = m_pendingRegister;

    auto isLinewiseCommand = [character] {
        return character == 'X' || character == 'D' || character == 'Y' || character == 'C'
            || character == 'S' || character == 'R';
    };
    auto commandRange = [this, &isLinewiseCommand] {
        auto range = visualRange();
        if (isLinewiseCommand()) {
            range = linesRange(m_visualAnchor, m_visualCursor);
        }
        return range;
    };

    if (_event->key() == Qt::Key_Delete && _event->modifiers() == Qt::NoModifier) {
        const auto range = visualRange();
        leaveVisualMode();
        deleteRange(range, registerName, false);
        return true;
    }

    //
    // ... Tab с выделением редактор применил бы ко всему выделению, поэтому поглощаем его вместе с
    //     остальным вводом текста
    //
    if (character.isNull()) {
        return !_event->text().isEmpty();
    }

    switch (character.unicode()) {
    case 'v':
    case 'V': {
        const auto targetMode = character == 'V' ? Mode::VisualLine : Mode::Visual;
        if (m_mode == targetMode) {
            enterNormalMode();
        } else {
            m_mode = targetMode;
            resetPendingCommand();
            updateVisualSelection();
        }
        return true;
    }

    case 'o':
    case 'O': {
        std::swap(m_visualAnchor, m_visualCursor);
        resetPendingCommand();
        updateVisualSelection();
        return true;
    }

    case 'i':
    case 'a': {
        m_textObjectAround = character == 'a';
        m_awaiting = Awaiting::TextObject;
        return true;
    }

    case 'y':
    case 'Y': {
        //
        // ... курсор встаёт в начало выделения, а в построчном режиме - в начало первой строки,
        //     если только курсор не стоял выше неё
        //
        const auto range = commandRange();
        int anchor = m_visualAnchor;
        if (range.linewise) {
            anchor = Vim::navigableBlock(document(), anchor).position();
        }
        const int cursorPosition = qMin(anchor, m_visualCursor);
        leaveVisualMode();
        yankRange(range, registerName, cursorPosition);
        return true;
    }

    case 'd':
    case 'x':
    case 'X':
    case 'D': {
        const auto range = commandRange();
        leaveVisualMode();
        deleteRange(range, registerName, false);
        return true;
    }

    case 'c':
    case 's':
    case 'C':
    case 'S':
    case 'R': {
        if (!canEdit()) {
            enterNormalMode();
            return true;
        }
        const auto range = commandRange();
        leaveVisualMode();
        changeRange(range, registerName, false);
        return true;
    }

    case 'r': {
        m_awaiting = Awaiting::ReplaceCharacter;
        return true;
    }

    case 'p':
    case 'P': {
        const int count = takeCount();
        replaceVisualWithRegister(registerName, character == 'P', count);
        return true;
    }

    case 'u':
    case 'U':
    case '~': {
        const auto range = visualRange();
        leaveVisualMode();
        changeCaseRange(range,
                        character == 'u'
                            ? Operator::Lowercase
                            : (character == 'U' ? Operator::Uppercase : Operator::SwapCase));
        return true;
    }

    case 'J': {
        const int lines = visualLinesCount();
        const int start = qMin(m_visualAnchor, m_visualCursor);
        leaveVisualMode();
        setCursorPosition(start);
        joinLines(qMax(2, lines), true);
        return true;
    }

    default: {
        resetPendingCommand();
        fail();
        return true;
    }
    }
}

bool VimTextEditController::handleOperatorTarget(QKeyEvent* _event)
{
    const auto character = commandCharacter(_event);

    //
    // Удвоенный оператор работает построчно: dd, cc, yy, guu, gUU, g~~
    //
    const bool doubledOperator = (character == 'd' && m_pendingOperator == Operator::Delete)
        || (character == 'c' && m_pendingOperator == Operator::Change)
        || (character == 'y' && m_pendingOperator == Operator::Yank)
        || (character == 'u' && m_pendingOperator == Operator::Lowercase)
        || (character == 'U' && m_pendingOperator == Operator::Uppercase)
        || (character == '~' && m_pendingOperator == Operator::SwapCase);
    if (doubledOperator) {
        const int count = takeCount();
        auto lastBlock = currentBlock();
        for (int index = 1; index < count; ++index) {
            const auto next = Vim::nextNavigableBlock(lastBlock);
            if (!next.isValid()) {
                //
                // ... если строк не хватает даже для одного шага, то команда не выполняется
                //
                if (index == 1) {
                    resetPendingCommand();
                    fail();
                    return true;
                }
                break;
            }
            lastBlock = next;
        }
        //
        // ... как и в vim, курсор при этом встаёт на первый значимый символ последней строки,
        //     кроме копирования, которое курсор не двигает
        //
        Motion motion;
        if (m_pendingOperator == Operator::Yank) {
            motion.position
                = lastBlock == currentBlock() ? currentPosition() : lastBlock.position();
        } else {
            motion.position = Vim::firstNonBlankPosition(lastBlock);
        }
        motion.linewise = true;
        applyOperatorToMotion(motion);
        return true;
    }

    //
    // ... текстовые объекты
    //
    if (character == 'i' || character == 'a') {
        m_textObjectAround = character == 'a';
        m_awaiting = Awaiting::TextObject;
        return true;
    }

    //
    // ... поиск в качестве цели
    //
    if (character == '/' || character == '?') {
        openCommandLine(QString(character));
        return true;
    }

    if (isMotionKey(_event)) {
        const bool withCount = hasCount();
        const int count = takeCount();
        applyMotion(motionForKey(_event, count, withCount));
        return true;
    }

    switch (character.unicode()) {
    case 'f':
    case 'F':
    case 't':
    case 'T': {
        m_findCommand = character;
        m_awaiting = Awaiting::FindCharacter;
        return true;
    }
    case '`':
    case '\'': {
        m_jumpToMarkExact = character == '`';
        m_awaiting = Awaiting::JumpToMark;
        return true;
    }
    case 'g': {
        m_awaiting = Awaiting::GoCommand;
        return true;
    }
    case 'z': {
        m_awaiting = Awaiting::ScrollCommand;
        return true;
    }
    case '"': {
        m_awaiting = Awaiting::RegisterName;
        return true;
    }
    default: {
        resetPendingCommand();
        fail();
        return true;
    }
    }
}

bool VimTextEditController::handleAwaitingInput(QKeyEvent* _event)
{
    const auto awaiting = m_awaiting;
    m_awaiting = Awaiting::None;
    const auto character = commandCharacter(_event);

    switch (awaiting) {
    case Awaiting::TextObject: {
        return handleTextObject(_event);
    }

    case Awaiting::GoCommand: {
        return handleGoCommand(_event);
    }

    case Awaiting::ScrollCommand: {
        return handleScrollCommand(_event);
    }

    case Awaiting::FindCharacter: {
        const auto text = characterText(_event);
        if (text.isEmpty()) {
            resetPendingCommand();
            return true;
        }
        shared().findCharacter = text;
        shared().findCommand = m_findCommand;
        const int count = takeCount();
        applyMotion(findCharacterMotion(text, m_findCommand == 'f' || m_findCommand == 't',
                                        m_findCommand == 't' || m_findCommand == 'T', count,
                                        false));
        return true;
    }

    case Awaiting::ReplaceCharacter: {
        const bool isLineBreak = isEnter(_event);
        const auto text = characterText(_event);
        if (text.isEmpty() && !isLineBreak) {
            if (isVisualMode()) {
                enterNormalMode();
            } else {
                resetPendingCommand();
            }
            return true;
        }

        const int count = takeCount();
        if (isVisualMode()) {
            const auto range = visualRange();
            leaveVisualMode();
            if (canEdit() && !isLineBreak) {
                //
                // ... заменяем символы, не трогая переводы строк
                //
                for (const auto& lineRange : lineRanges(range)) {
                    const auto block = document()->findBlock(lineRange.start);
                    replaceText(
                        lineRange.start, lineRange.end,
                        text.repeated(charactersCount(block, lineRange.start, lineRange.end)));
                }
            }
            setCursorPosition(characterPosition(range.start));
            return true;
        }

        resetPendingCommand();
        if (isLineBreak) {
            //
            // ... r<Enter> заменяет символы одним переводом строки
            //
            const int position = currentPosition();
            int moved = 0;
            const int end = movePositionByCharacters(currentBlock(), position, count, &moved);
            if (!canEdit() || moved < count) {
                fail();
                return true;
            }
            removeText(position, end);
            QTextCursor cursor(document());
            cursor.setPosition(position);
            m_editor->setTextCursor(cursor);
            sendKeyPressToEditor(Qt::Key_Return, QStringLiteral("\r"));
            setCursorPosition(characterPosition(m_editor->textCursor().position()));
            return true;
        }
        replaceCharacters(text, count);
        return true;
    }

    case Awaiting::SetMark: {
        if (!character.isNull()
            && ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')
                || character == '`' || character == '\'' || character == '<' || character == '>')) {
            setMark(character == '\'' ? QChar('`') : character, currentPosition());
        }
        resetPendingCommand();
        return true;
    }

    case Awaiting::JumpToMark: {
        if (character.isNull()) {
            resetPendingCommand();
            return true;
        }
        applyMotion(markMotion(character, m_jumpToMarkExact));
        return true;
    }

    case Awaiting::RegisterName: {
        if (character.isNull() || !Vim::Registers::isValidName(character)) {
            resetPendingCommand();
            return true;
        }
        m_pendingRegister = character;
        return true;
    }

    case Awaiting::RecordRegister: {
        resetPendingCommand();
        if (!character.isNull()
            && ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')
                || (character >= '0' && character <= '9') || character == '"')) {
            toggleMacroRecording(character);
        }
        return true;
    }

    case Awaiting::PlayRegister: {
        const int count = takeCount();
        resetPendingCommand();
        m_commandIsChange = false;
        //
        // ... @: повторяет последнюю команду строки ввода
        //
        const auto name = character == '@' ? shared().lastPlayedMacro : character;
        if (name == ':') {
            if (shared().lastExCommand.isEmpty()) {
                fail(QCoreApplication::translate("Ui::VimTextEditController",
                                                 "E30: No previous command line"));
                return true;
            }
            shared().lastPlayedMacro = name;
            for (int step = 0; step < count && !m_commandFailed; ++step) {
                executeExCommands(shared().lastExCommand);
            }
            return true;
        }
        if (name.isNull()) {
            fail(character == '@' ? QCoreApplication::translate("Ui::VimTextEditController",
                                                                "E748: No previously used register")
                                  : QString());
            return true;
        }
        playMacro(name, count);
        return true;
    }

    case Awaiting::None:
    default: {
        resetPendingCommand();
        return true;
    }
    }
}

bool VimTextEditController::handleGoCommand(QKeyEvent* _event)
{
    const auto character = commandCharacter(_event);

    //
    // Перемещения
    //
    if (character == 'g' || character == 'e' || character == 'E') {
        const bool withCount = hasCount();
        const int count = takeCount();
        Motion motion;
        if (character == 'g') {
            motion = lineMotion(withCount ? count : 1, true);
            motion.jump = true;
        } else {
            const int position = currentPosition();
            Vim::Cursor cursor(document(), position);
            const bool success = Vim::backwardEndOfWord(cursor, count, character == 'E', false);
            if (success || cursor.position() != position) {
                motion.position = cursor.position();
                motion.failed = !success;
            }
            motion.inclusive = true;
        }
        applyMotion(motion);
        return true;
    }

    //
    // ... к последнему значимому символу строки
    //
    if (character == '_') {
        const int count = takeCount();
        Motion motion;
        motion.inclusive = true;
        auto block = currentBlock();
        for (int index = 1; index < count && block.isValid(); ++index) {
            block = Vim::nextNavigableBlock(block);
        }
        if (block.isValid()) {
            int end = Vim::lineEndPosition(block);
            while (end > block.position()
                   && QStringLiteral(" \t").contains(document()->characterAt(end - 1))) {
                end = Vim::previousCharacterPosition(block, end);
            }
            motion.position
                = end > block.position() ? Vim::previousCharacterPosition(block, end) : end;
        }
        applyMotion(motion);
        return true;
    }

    //
    // ... по строкам на экране, на которые разбит абзац
    //
    if (character == 'j' || character == 'k') {
        const int count = takeCount();
        auto motion = verticalMotion(count, character == 'j', true);
        motion.linewise = false;
        applyMotion(motion);
        return true;
    }
    if (character == '0' || character == '^' || character == '$') {
        takeCount();
        const int position = currentPosition();
        const auto block = Vim::navigableBlock(document(), position);
        const auto layout = block.layout();
        const auto line = layout != nullptr
            ? layout->lineForTextPosition(position - block.position())
            : QTextLine();
        const int lineStart = block.position() + (line.isValid() ? line.textStart() : 0);
        const int lineEnd = line.isValid()
            ? qMin(Vim::lineEndPosition(block), lineStart + line.textLength())
            : Vim::lineEndPosition(block);
        Motion motion;
        if (character == '0') {
            motion.position = lineStart;
        } else if (character == '^') {
            motion.position = lineStart;
            while (motion.position < lineEnd
                   && QStringLiteral(" \t").contains(document()->characterAt(motion.position))) {
                motion.position = Vim::nextCharacterPosition(block, motion.position);
            }
        } else {
            motion.position
                = lineEnd > lineStart ? Vim::previousCharacterPosition(block, lineEnd) : lineStart;
            motion.inclusive = true;
        }
        applyMotion(motion);
        return true;
    }

    if (character == '*' || character == '#') {
        const int count = takeCount();
        searchWordUnderCursor(character == '*', false, count);
        return true;
    }

    //
    // ... gn и gN выделяют ближайшее совпадение с последним поиском, или применяют к нему оператор
    //
    if (character == 'n' || character == 'N') {
        takeCount();
        const auto match = searchMatchRange(character == 'n');
        if (!match.isValid()) {
            resetPendingCommand();
            return true;
        }
        const auto block = document()->findBlock(match.start);
        const int last = match.end > match.start ? Vim::previousCharacterPosition(block, match.end)
                                                 : match.start;
        if (m_pendingOperator != Operator::None) {
            setCursorPosition(match.start);
            Motion motion;
            motion.position = last;
            motion.inclusive = match.end > match.start;
            applyOperatorToMotion(motion);
            return true;
        }
        if (character == 'n') {
            enterVisualMode(Mode::Visual, match.start, last);
        } else {
            enterVisualMode(Mode::Visual, last, match.start);
        }
        return true;
    }

    //
    // Операторы смены регистра
    //
    if (character == 'u' || character == 'U' || character == '~') {
        const auto caseOperator = character == 'u'
            ? Operator::Lowercase
            : (character == 'U' ? Operator::Uppercase : Operator::SwapCase);
        if (isVisualMode()) {
            const auto range = visualRange();
            leaveVisualMode();
            changeCaseRange(range, caseOperator);
            return true;
        }
        //
        // ... gugu, gUgU и g~g~ работают построчно, как guu
        //
        if (m_pendingOperator != Operator::None) {
            if (m_pendingOperator == caseOperator) {
                QKeyEvent doubled(QEvent::KeyPress, keyForCharacter(character), Qt::NoModifier,
                                  QString(character));
                return handleOperatorTarget(&doubled);
            }
            resetPendingCommand();
            return true;
        }
        m_pendingOperator = caseOperator;
        m_operatorCount = takeOptionalCount();
        return true;
    }

    if (m_pendingOperator != Operator::None) {
        resetPendingCommand();
        return true;
    }

    switch (character.unicode()) {
    case 'v': {
        if (isVisualMode()) {
            resetPendingCommand();
            return true;
        }
        restoreLastVisualSelection();
        return true;
    }

    case 'J': {
        if (isVisualMode()) {
            const int lines = visualLinesCount();
            const int start = qMin(m_visualAnchor, m_visualCursor);
            leaveVisualMode();
            setCursorPosition(start);
            joinLines(qMax(2, lines), false);
            return true;
        }
        const int count = takeCount();
        joinLines(count, false);
        resetPendingCommand();
        return true;
    }

    case 'I':
    case 'i': {
        if (isVisualMode()) {
            resetPendingCommand();
            return true;
        }
        const int count = takeCount();
        if (!canEdit()) {
            resetPendingCommand();
            return true;
        }
        if (character == 'I') {
            setCursorPosition(currentBlock().position());
        } else if (!m_lastInsertPosition.isNull()) {
            QTextCursor cursor(document());
            cursor.setPosition(m_lastInsertPosition.position());
            m_editor->setTextCursor(cursor);
        }
        enterInsertMode(count, character);
        return true;
    }

    case '&': {
        resetPendingCommand();
        substitute(Vim::firstNavigableBlock(document()), Vim::lastNavigableBlock(document()),
                   QStringLiteral("&&"));
        return true;
    }

    case 'p':
    case 'P': {
        //
        // ... gp и gP вставляют, как p и P, но оставляют курсор сразу за вставленным текстом
        //
        if (isVisualMode()) {
            resetPendingCommand();
            return true;
        }
        const int count = takeCount();
        const auto registerName = m_pendingRegister;
        resetPendingCommand();
        paste(registerName, character == 'P', count, true);
        return true;
    }

    default: {
        resetPendingCommand();
        fail();
        return true;
    }
    }
}

bool VimTextEditController::handleScrollCommand(QKeyEvent* _event)
{
    const auto character = commandCharacter(_event);
    const bool isLineBreak = isEnter(_event);

    //
    // ... с набранным оператором команды прокрутки не работают
    //
    if (m_pendingOperator != Operator::None) {
        resetPendingCommand();
        return true;
    }
    resetPendingCommand();

    if (character == 'z' || character == '.') {
        scrollCursorTo(ScrollToCenter);
    } else if (character == 't' || isLineBreak) {
        scrollCursorTo(ScrollToTop);
    } else if (character == 'b' || character == '-') {
        scrollCursorTo(ScrollToBottom);
    } else {
        return true;
    }

    if (character == '.' || character == '-' || isLineBreak) {
        setCursorPosition(Vim::firstNonBlankPosition(currentBlock()));
    }
    return true;
}

bool VimTextEditController::handleTextObject(QKeyEvent* _event)
{
    const auto character = commandCharacter(_event);
    const bool around = m_textObjectAround;
    const bool visual = isVisualMode();
    auto document = this->document();
    const int count = takeCount();
    const int start = visual ? m_visualAnchor : currentPosition();
    const int cursor = visual ? m_visualCursor : currentPosition();

    Vim::ObjectRange object;
    switch (character.unicode()) {
    case 'w':
    case 'W': {
        object = Vim::wordObject(document, start, cursor, visual, count, around, character == 'W');
        break;
    }
    case 's': {
        object = Vim::sentenceObject(document, start, cursor, visual, count, around);
        break;
    }
    case 'p': {
        object = Vim::paragraphObject(document, start, cursor, visual, m_mode == Mode::VisualLine,
                                      count, around);
        break;
    }
    case '(':
    case ')':
    case 'b': {
        object = Vim::bracketObject(document, start, cursor, visual, count, around, '(', ')');
        break;
    }
    case '[':
    case ']': {
        object = Vim::bracketObject(document, start, cursor, visual, count, around, '[', ']');
        break;
    }
    case '{':
    case '}':
    case 'B': {
        object = Vim::bracketObject(document, start, cursor, visual, count, around, '{', '}');
        break;
    }
    case '<':
    case '>': {
        object = Vim::bracketObject(document, start, cursor, visual, count, around, '<', '>');
        break;
    }
    case '"':
    case '\'':
    case '`': {
        object = Vim::quoteObject(document, start, cursor, visual, count, around, character);
        break;
    }
    default: {
        break;
    }
    }

    if (!object.isValid()) {
        if (!visual) {
            resetPendingCommand();
        }
        fail();
        return true;
    }

    //
    // ... в визуальном режиме объект просто становится выделением
    //
    if (visual) {
        m_visualAnchor = object.start;
        m_visualCursor = object.cursor;
        m_mode = object.linewise ? Mode::VisualLine : Mode::Visual;
        resetPendingCommand();
        updateVisualSelection();
        return true;
    }

    applyOperatorToObject(object);
    return true;
}


// ****


bool VimTextEditController::isMotionKey(QKeyEvent* _event) const
{
    switch (_event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_Backspace:
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        return _event->modifiers() == Qt::NoModifier || _event->modifiers() == Qt::KeypadModifier;
    }
    default: {
        break;
    }
    }

    const auto character = commandCharacter(_event);
    return !character.isNull()
        && QStringLiteral("hjkl 0^$|wWbBeE(){}%GHML+-_nN*#;,").contains(character);
}

VimTextEditController::Motion VimTextEditController::motionForKey(QKeyEvent* _event, int _count,
                                                                  bool _hasCount)
{
    const auto character = commandCharacter(_event);
    const auto key = _event->key();
    const bool forOperator = m_pendingOperator != Operator::None;
    auto document = this->document();
    const int position = currentPosition();
    Motion motion;

    //
    // Влево
    //
    if (character == 'h' || key == Qt::Key_Left || key == Qt::Key_Backspace) {
        const bool wrap = key == Qt::Key_Backspace;
        Vim::Cursor cursor(document, position);
        int moved = 0;
        for (int count = _count; count > 0; --count, ++moved) {
            if (cursor.column() > 0) {
                cursor.setColumn(cursor.column() - 1);
                continue;
            }
            const auto previous = Vim::previousNavigableBlock(cursor.block());
            if (wrap && previous.isValid()) {
                //
                // ... при удалении перевод строки тоже считается символом
                //
                cursor.setPosition(Vim::lineEndPosition(previous));
                if (!(forOperator
                      && (m_pendingOperator == Operator::Delete
                          || m_pendingOperator == Operator::Change)
                      && !cursor.isLineEmpty())) {
                    cursor.setColumn(qMax(0, cursor.lineLength() - 1));
                } else {
                    motion.keepOperatorEnd = true;
                }
                continue;
            }
            break;
        }
        //
        // ... как и в vim, если сдвинуться не удалось вовсе, то это ошибка
        //
        if (moved == 0) {
            return motion;
        }
        motion.position = cursor.position();
        return motion;
    }

    //
    // Вправо
    //
    if (character == 'l' || character == ' ' || key == Qt::Key_Right) {
        const bool wrap = character == ' ';
        const bool pastLine = isVisualMode();
        Vim::Cursor cursor(document, position);
        int moved = 0;
        for (int count = _count; count > 0; --count, ++moved) {
            const int nextColumn = Vim::nextCharacterPosition(cursor.block(), cursor.position())
                - cursor.block().position();
            const bool canMove = pastLine ? !cursor.atLineEnd() : nextColumn < cursor.lineLength();
            if (canMove) {
                cursor.setColumn(nextColumn);
                continue;
            }
            const auto next = Vim::nextNavigableBlock(cursor.block());
            if (wrap && next.isValid()) {
                if (forOperator && !motion.inclusive && !cursor.isLineEmpty()) {
                    motion.inclusive = true;
                } else {
                    cursor.setPosition(next.position());
                    motion.inclusive = false;
                }
                continue;
            }
            if (forOperator && !cursor.isLineEmpty()) {
                motion.inclusive = true;
            }
            break;
        }
        if (moved == 0 && !forOperator) {
            return motion;
        }
        motion.position = cursor.position();
        return motion;
    }

    //
    // Вверх и вниз
    //
    if (character == 'j' || character == 'k' || key == Qt::Key_Down || key == Qt::Key_Up) {
        const bool down = character == 'j' || key == Qt::Key_Down;
        return verticalMotion(_count, down, !forOperator && m_mode != Mode::VisualLine);
    }
    if (character == '+' || character == '-' || character == '_' || key == Qt::Key_Return
        || key == Qt::Key_Enter) {
        const bool down = character != '-';
        const int count = character == '_' ? _count - 1 : _count;
        auto block = Vim::navigableBlock(document, position);
        for (int index = 0; index < count; ++index) {
            const auto next
                = down ? Vim::nextNavigableBlock(block) : Vim::previousNavigableBlock(block);
            if (!next.isValid()) {
                if (index == 0) {
                    return motion;
                }
                break;
            }
            block = next;
        }
        motion.position = Vim::firstNonBlankPosition(block);
        motion.linewise = true;
        return motion;
    }

    //
    // Начало и конец строки
    //
    if (character == '0' || key == Qt::Key_Home) {
        motion.position = Vim::navigableBlock(document, position).position();
        return motion;
    }
    if (character == '^') {
        motion.position = Vim::firstNonBlankPosition(Vim::navigableBlock(document, position));
        return motion;
    }
    if (character == '$' || key == Qt::Key_End) {
        auto block = Vim::navigableBlock(document, position);
        for (int index = 1; index < _count; ++index) {
            const auto next = Vim::nextNavigableBlock(block);
            if (!next.isValid()) {
                return motion;
            }
            block = next;
        }
        const int lineEnd = Vim::lineEndPosition(block);
        motion.position = isVisualMode() || lineEnd == block.position()
            ? lineEnd
            : Vim::previousCharacterPosition(block, lineEnd);
        motion.inclusive = true;
        motion.toLineEnd = true;
        motion.keepPreferredX = true;
        return motion;
    }
    if (character == '|') {
        const auto block = Vim::navigableBlock(document, position);
        motion.position = Vim::characterStartPosition(
            block, block.position() + qMin(_count - 1, qMax(0, block.length() - 2)));
        return motion;
    }

    //
    // Слова
    //
    if (character == 'w' || character == 'W' || character == 'e' || character == 'E') {
        bool wordEnd = character == 'e' || character == 'E';
        const bool bigWord = character == 'W' || character == 'E';
        Vim::Cursor cursor(document, position);
        motion.inclusive = wordEnd;
        bool stop = false;
        //
        // ... cw и cW работают как ce и cE, т.е. не захватывают пробелы за словом
        //
        if (!wordEnd && m_pendingOperator == Operator::Change && !cursor.atLineEnd()) {
            if (!cursor.character().isSpace()) {
                motion.inclusive = true;
                wordEnd = true;
            }
            stop = true;
        }

        const bool success = wordEnd ? Vim::endOfWord(cursor, _count, bigWord, stop, false)
                                     : Vim::forwardWord(cursor, _count, bigWord, forOperator);

        //
        // ... курсор не может остаться в конце строки, разве что в визуальном режиме
        //
        if (position < cursor.position() && cursor.column() > 0 && cursor.atLineEnd()
            && !isVisualMode()) {
            cursor.setColumn(cursor.column() - 1);
            motion.inclusive = true;
        }

        //
        // ... как и в vim, если слов не хватило, курсор всё равно перемещается, но если он так и
        //     не сдвинулся, то перемещение невозможно
        //
        if (!success && !forOperator && cursor.position() == position) {
            return motion;
        }
        motion.position = cursor.position();
        motion.failed = !success;
        return motion;
    }
    if (character == 'b' || character == 'B') {
        Vim::Cursor cursor(document, position);
        const bool success = Vim::backwardWord(cursor, _count, character == 'B', false);
        if (success || cursor.position() != position) {
            motion.position = cursor.position();
            motion.failed = !success;
        }
        return motion;
    }

    //
    // Предложения
    //
    if (character == '(' || character == ')') {
        const bool paragraphBreaks = true;
        Vim::Cursor cursor(document, position, paragraphBreaks);
        if (!Vim::findSentence(cursor, character == ')', _count)) {
            return motion;
        }
        if (cursor.column() > 0 && cursor.atLineEnd() && !isVisualMode()) {
            cursor.setColumn(cursor.column() - 1);
            motion.inclusive = true;
        }
        motion.position = cursor.position();
        motion.jump = true;
        motion.toNumberedRegister = true;
        return motion;
    }

    //
    // Абзацы
    //
    if (character == '{' || character == '}') {
        bool inclusive = false;
        motion.position
            = Vim::paragraphPosition(document, position, character == '}', _count, &inclusive);
        motion.inclusive = inclusive;
        motion.jump = true;
        motion.toNumberedRegister = true;
        return motion;
    }

    //
    // Парная скобка, или переход к проценту документа
    //
    if (character == '%') {
        if (_hasCount) {
            if (_count > 100) {
                return motion;
            }
            const int lines = Vim::navigableBlocksCount(document);
            motion = lineMotion((_count * lines + 99) / 100, true);
        } else {
            motion.position = Vim::matchingBracketPosition(document, position);
            motion.inclusive = true;
            motion.toNumberedRegister = true;
        }
        motion.jump = true;
        return motion;
    }

    //
    // Строки по номеру и на экране
    //
    if (character == 'G') {
        motion = lineMotion(_hasCount ? _count : Vim::navigableBlocksCount(document), true);
        motion.jump = true;
        return motion;
    }
    if (character == 'H' || character == 'M' || character == 'L') {
        const auto viewport = m_editor->viewport()->rect();
        const int x = viewport.left() + viewport.width() / 2;
        int y = viewport.center().y();
        if (character == 'H') {
            y = viewport.top() + 1;
        } else if (character == 'L') {
            y = viewport.bottom() - 1;
        }
        auto block
            = Vim::navigableBlock(document, m_editor->cursorForPosition({ x, y }).position());
        if (character != 'M') {
            for (int index = 1; index < _count; ++index) {
                const auto next = character == 'H' ? Vim::nextNavigableBlock(block)
                                                   : Vim::previousNavigableBlock(block);
                if (!next.isValid()) {
                    break;
                }
                block = next;
            }
        }
        motion.position = Vim::firstNonBlankPosition(block);
        motion.linewise = true;
        motion.jump = true;
        return motion;
    }

    //
    // Поиск
    //
    if (character == 'n' || character == 'N') {
        const bool forward = character == 'n' ? shared().searchForward : !shared().searchForward;
        return searchMotion(shared().searchPattern, forward, _count, position,
                            shared().searchOffset);
    }
    if (character == '*' || character == '#') {
        searchWordUnderCursor(character == '*', true, _count);
        motion.position = -2;
        return motion;
    }

    //
    // Повтор поиска символа в строке
    //
    if (character == ';' || character == ',') {
        if (shared().findCharacter.isEmpty()) {
            return motion;
        }
        const auto command = shared().findCommand;
        const bool forward = (command == 'f' || command == 't') == (character == ';');
        return findCharacterMotion(shared().findCharacter, forward,
                                   command == 't' || command == 'T', _count, true);
    }

    return motion;
}

VimTextEditController::Motion VimTextEditController::verticalMotion(int _count, bool _down,
                                                                    bool _visualLines)
{
    Motion motion;
    motion.keepPreferredX = true;
    motion.linewise = true;
    const int position = currentPosition();
    auto document = this->document();

    //
    // Операторы и построчное выделение работают с абзацами целиком
    //
    if (!_visualLines) {
        auto block = Vim::navigableBlock(document, position);
        const int column = position - block.position();
        int moved = 0;
        for (; moved < _count; ++moved) {
            const auto next
                = _down ? Vim::nextNavigableBlock(block) : Vim::previousNavigableBlock(block);
            if (!next.isValid()) {
                break;
            }
            block = next;
        }
        if (moved == 0) {
            return motion;
        }
        motion.position = m_preferLineEnd
            ? Vim::lineEndPosition(block)
            : block.position() + qMin(column, qMax(0, block.length() - 1));
        return motion;
    }

    //
    // ... а простое перемещение идёт по строкам на экране, сохраняя горизонтальное положение
    //
    QTextCursor cursor(document);
    cursor.setPosition(position);
    if (m_preferredX < 0) {
        m_preferredX
            = m_preferLineEnd ? std::numeric_limits<int>::max() / 4 : cursor.verticalMovementX();
    }
    int moved = 0;
    for (; moved < _count; ++moved) {
        const int previousPosition = cursor.position();
        cursor.setVerticalMovementX(m_preferredX);
        bool success = cursor.movePosition(_down ? QTextCursor::Down : QTextCursor::Up);
        while (success && !Vim::isNavigableBlock(cursor.block())) {
            success = cursor.movePosition(_down ? QTextCursor::Down : QTextCursor::Up);
        }
        if (!success || cursor.position() == previousPosition) {
            cursor.setPosition(previousPosition);
            break;
        }
    }
    if (moved == 0) {
        return motion;
    }

    motion.position = m_preferLineEnd ? Vim::lineEndPosition(cursor.block()) : cursor.position();
    return motion;
}

VimTextEditController::Motion VimTextEditController::lineMotion(int _lineNumber,
                                                                bool _firstNonBlank) const
{
    Motion motion;
    const auto block = Vim::navigableBlockByNumber(document(), _lineNumber);
    motion.position = _firstNonBlank ? Vim::firstNonBlankPosition(block) : block.position();
    motion.linewise = true;
    return motion;
}

VimTextEditController::Motion VimTextEditController::findCharacterMotion(const QString& _character,
                                                                         bool _forward, bool _till,
                                                                         int _count,
                                                                         bool _repeat) const
{
    Motion motion;
    motion.inclusive = _forward;

    const Vim::Cursor cursor(document(), currentPosition());
    const auto block = cursor.block();
    const auto text = Vim::displayedText(block);
    const int lineStart = block.position();
    const int lineEnd = Vim::lineEndPosition(block);
    int position = cursor.position();
    //
    // ... при повторе поиска до символа стоящий рядом искомый символ пропускается
    //
    bool stop = !(_repeat && _till && _count == 1);
    for (int count = _count; count > 0; --count) {
        forever
        {
            if (_forward) {
                position = Vim::nextCharacterPosition(block, position);
                if (position >= lineEnd) {
                    return motion;
                }
            } else {
                if (position <= lineStart) {
                    return motion;
                }
                position = Vim::previousCharacterPosition(block, position);
            }
            const int characterEnd = Vim::nextCharacterPosition(block, position);
            if (stop && text.mid(position - lineStart, characterEnd - position) == _character) {
                break;
            }
            stop = true;
        }
    }

    if (_till) {
        position = _forward ? Vim::previousCharacterPosition(block, position)
                            : Vim::nextCharacterPosition(block, position);
    }
    motion.position = position;
    return motion;
}

VimTextEditController::Motion VimTextEditController::markMotion(QChar _name, bool _exact)
{
    Motion motion;
    const auto name = _name == '\'' ? QChar('`') : _name;
    if (!m_marks.contains(name) || m_marks.value(name).isNull()
        || m_marks.value(name).document() != document()) {
        fail(QCoreApplication::translate("Ui::VimTextEditController", "E20: Mark not set"));
        return motion;
    }

    const int position = m_marks.value(name).position();
    motion.jump = true;
    if (_exact) {
        motion.position = Vim::Cursor(document(), position).position();
    } else {
        motion.position = Vim::firstNonBlankPosition(Vim::navigableBlock(document(), position));
        motion.linewise = true;
    }
    motion.toNumberedRegister = _exact;
    return motion;
}

VimTextEditController::Motion VimTextEditController::searchMotion(const QString& _pattern,
                                                                  bool _forward, int _count,
                                                                  int _fromPosition,
                                                                  const QString& _offset)
{
    Motion motion;
    motion.jump = true;
    motion.toNumberedRegister = true;
    if (_pattern.isEmpty()) {
        fail(QCoreApplication::translate("Ui::VimTextEditController",
                                         "E35: No previous regular expression"));
        return motion;
    }

    //
    // Разбираем смещение: число строк, или сдвиг от конца (e), или от начала (s, b) совпадения
    //
    QChar offsetType;
    int offset = 0;
    {
        auto offsetText = _offset.trimmed();
        if (!offsetText.isEmpty() && QStringLiteral("esb").contains(offsetText.at(0))) {
            offsetType = offsetText.at(0);
            offsetText = offsetText.mid(1);
        }
        if (offsetText == '+' || offsetText == '-') {
            offset = offsetText == '+' ? 1 : -1;
        } else if (!offsetText.isEmpty()) {
            bool ok = false;
            offset = offsetText.toInt(&ok);
            if (!ok) {
                offset = 0;
            }
        }
        if (offsetType.isNull() && !_offset.trimmed().isEmpty()) {
            offsetType = 'l';
        }
    }

    //
    // ... как и vim, перед поиском отменяем сдвиг в символах, чтобы повторный поиск не находил
    //     то же самое совпадение
    //
    int position = _fromPosition;
    if (offsetType != 'l' && offset != 0) {
        Vim::Cursor start(document(), position);
        for (int step = 0; step < qAbs(offset); ++step) {
            if ((offset > 0 ? start.decl() : start.incl()) == -1) {
                break;
            }
        }
        position = start.position();
    }
    int matchEnd = -1;
    for (int count = 0; count < _count; ++count) {
        position = searchPosition(_pattern, position, _forward, &matchEnd);
        if (position < 0) {
            if (m_message.isEmpty()) {
                fail(QCoreApplication::translate("Ui::VimTextEditController",
                                                 "E486: Pattern not found: %1")
                         .arg(_pattern));
            } else {
                fail();
            }
            return motion;
        }
    }

    if (offsetType == 'l') {
        auto block = Vim::navigableBlock(document(), position);
        for (int step = 0; step < qAbs(offset); ++step) {
            const auto next
                = offset > 0 ? Vim::nextNavigableBlock(block) : Vim::previousNavigableBlock(block);
            if (!next.isValid()) {
                break;
            }
            block = next;
        }
        motion.position = block.position();
        motion.linewise = true;
        return motion;
    }

    Vim::Cursor cursor(document(), position);
    if (offsetType == 'e') {
        //
        // ... конец совпадения - последний его символ, и он входит в диапазон оператора
        //
        cursor.setPosition(qMax(position, matchEnd));
        if (matchEnd > position) {
            cursor.decl();
        }
        motion.inclusive = true;
    }
    for (int step = 0; step < qAbs(offset); ++step) {
        if ((offset > 0 ? cursor.incl() : cursor.decl()) == -1) {
            break;
        }
    }
    motion.position = cursor.position();
    return motion;
}

void VimTextEditController::applyMotion(const Motion& _motion)
{
    //
    // ... специальное значение означает, что перемещение уже выполнено
    //
    if (_motion.position == -2) {
        return;
    }

    if (_motion.position < 0) {
        resetPendingCommand();
        fail();
        return;
    }

    //
    // ... не полностью выполненное перемещение без оператора всё же сдвигает курсор
    //
    if (_motion.failed && m_pendingOperator == Operator::None) {
        fail();
    }

    if (_motion.jump) {
        rememberJumpPosition();
    }

    //
    // ... после оператора горизонтальное положение курсора определяется заново
    //
    if (m_pendingOperator != Operator::None) {
        m_preferredX = -1;
        m_preferLineEnd = false;
        applyOperatorToMotion(_motion);
        return;
    }

    if (!_motion.keepPreferredX) {
        m_preferredX = -1;
        m_preferLineEnd = false;
    }
    if (_motion.toLineEnd) {
        m_preferredX = -1;
        m_preferLineEnd = true;
    }

    //
    // ... в визуальном режиме курсор может стоять и в конце строки, тогда в выделение попадает
    //     перевод строки
    //
    if (isVisualMode()) {
        m_visualCursor = _motion.position;
        updateVisualSelection();
    } else {
        setCursorPosition(characterPosition(_motion.position));
    }
    resetPendingCommand();
}

void VimTextEditController::applyOperatorToMotion(const Motion& _motion)
{
    const auto pendingOperator = m_pendingOperator;
    auto document = this->document();
    const int position = currentPosition();
    const int start = qMin(position, _motion.position);
    const int end = qMax(position, _motion.position);

    //
    // Построчные перемещения захватывают строки целиком
    //
    if (_motion.linewise) {
        applyOperator(pendingOperator, linesRange(start, end), start, _motion.toNumberedRegister);
        return;
    }

    //
    // Если конец невключающего перемещения попадает в начало строки, то он переносится в конец
    // предыдущей, а если начало при этом в отступе, то оператор и вовсе работает построчно
    //
    const Vim::Cursor startCursor(document, start);
    const Vim::Cursor endCursor(document, end);
    int rangeEnd = end;
    bool inclusive = _motion.inclusive;
    if (!inclusive && !_motion.keepOperatorEnd && endCursor.column() == 0
        && endCursor.block() != startCursor.block()) {
        const auto previous = Vim::previousNavigableBlock(endCursor.block());
        if (startCursor.inIndent(0)) {
            applyOperator(pendingOperator, linesRange(start, previous.position()), start,
                          _motion.toNumberedRegister);
            return;
        }
        rangeEnd = Vim::lineEndPosition(previous);
    } else if (inclusive && !endCursor.atLineEnd()) {
        Vim::Cursor next = endCursor;
        next.inc();
        rangeEnd = next.block() == endCursor.block() ? next.position()
                                                     : Vim::lineEndPosition(endCursor.block());
    }

    //
    // ... многострочное удаление, после которого в строке остаются только пробелы, тоже
    //     становится построчным
    //
    const Vim::Cursor rangeEndCursor(document, rangeEnd);
    if (pendingOperator == Operator::Delete && rangeEndCursor.block() != startCursor.block()
        && startCursor.inIndent(0)
        && rangeEndCursor.block().text().mid(rangeEndCursor.column()).trimmed().isEmpty()) {
        applyOperator(pendingOperator, linesRange(start, rangeEnd), start,
                      _motion.toNumberedRegister);
        return;
    }

    applyOperator(pendingOperator, { start, rangeEnd, false }, start, _motion.toNumberedRegister);
}

void VimTextEditController::applyOperatorToObject(const Vim::ObjectRange& _object)
{
    Motion motion;
    motion.position = _object.cursor;
    motion.inclusive = _object.inclusive;
    motion.linewise = _object.linewise;

    //
    // ... объект задаёт и начало диапазона, поэтому ставим туда курсор
    //
    setCursorPosition(_object.start);
    applyOperatorToMotion(motion);
}


// ****


QTextDocument* VimTextEditController::document() const
{
    return m_editor->document();
}

int VimTextEditController::currentPosition() const
{
    if (isVisualMode() && m_visualCursor >= 0) {
        return Vim::Cursor(document(), m_visualCursor).position();
    }

    return characterPosition(m_editor->textCursor().position());
}

QTextBlock VimTextEditController::currentBlock() const
{
    return Vim::navigableBlock(document(), currentPosition());
}

int VimTextEditController::characterPosition(int _position) const
{
    const Vim::Cursor cursor(document(), _position);
    if (cursor.atLineEnd() && !cursor.isLineEmpty()) {
        Vim::Cursor previous = cursor;
        previous.setColumn(cursor.column() - 1);
        return previous.position();
    }
    return cursor.position();
}

void VimTextEditController::setCursorPosition(int _position)
{
    QTextCursor cursor(document());
    cursor.setPosition(Vim::Cursor(document(), _position).position());
    m_editor->setTextCursor(cursor);
}

Vim::TextRange VimTextEditController::linesRange(int _fromPosition, int _toPosition) const
{
    const auto firstBlock = Vim::navigableBlock(document(), qMin(_fromPosition, _toPosition));
    const auto lastBlock = Vim::navigableBlock(document(), qMax(_fromPosition, _toPosition));
    return { firstBlock.position(), Vim::lineEndPosition(lastBlock), true };
}


// ****


void VimTextEditController::applyOperator(Operator _operator, const Vim::TextRange& _range,
                                          int _cursorPosition, bool _toNumberedRegister)
{
    const auto registerName = m_pendingRegister;
    resetPendingCommand();

    if (!_range.isValid()) {
        return;
    }

    switch (_operator) {
    case Operator::Yank: {
        yankRange(_range, registerName, _cursorPosition);
        break;
    }
    case Operator::Delete: {
        deleteRange(_range, registerName, _toNumberedRegister);
        break;
    }
    case Operator::Change: {
        changeRange(_range, registerName, _toNumberedRegister);
        break;
    }
    case Operator::SwapCase:
    case Operator::Lowercase:
    case Operator::Uppercase: {
        changeCaseRange(_range, _operator, _cursorPosition);
        break;
    }
    default: {
        break;
    }
    }
}

void VimTextEditController::yankRange(const Vim::TextRange& _range, QChar _registerName,
                                      int _cursorPosition)
{
    if (!_range.isValid()) {
        return;
    }

    shared().registers.setYankedContent(_registerName, registerContent(_range));

    //
    // ... курсор встаёт в начало скопированного фрагмента, как это делает vim
    //
    setCursorPosition(characterPosition(_cursorPosition));
}

void VimTextEditController::deleteRange(const Vim::TextRange& _range, QChar _registerName,
                                        bool _toNumberedRegister)
{
    if (!canEdit() || !_range.isValid() || (!_range.linewise && _range.start == _range.end)) {
        return;
    }

    if (!_range.linewise) {
        if (!isEditableRange(_range.start, _range.end)) {
            fail();
            return;
        }
        shared().registers.setDeletedContent(_registerName, registerContent(_range),
                                             _toNumberedRegister);
        const bool removed = removeText(_range.start, _range.end);
        setCursorPosition(characterPosition(removed ? _range.start : _range.end));
        return;
    }

    shared().registers.setDeletedContent(_registerName, registerContent(_range),
                                         _toNumberedRegister);
    const int cursorPosition = deleteLines(lineRuns(_range), 0);
    setCursorPosition(Vim::firstNonBlankPosition(Vim::navigableBlock(document(), cursorPosition)));
}

QVector<Vim::TextRange> VimTextEditController::lineRuns(const Vim::TextRange& _range) const
{
    //
    // Строки диапазона делим на части, между которыми есть скрытый текст, или граница ячейки
    // таблицы, ведь удалять их нужно по отдельности, чтобы не задеть то, что между ними
    //
    QVector<Vim::TextRange> runs;
    QTextBlock runStart;
    QTextBlock runEnd;
    auto finishRun = [&] {
        if (runStart.isValid()) {
            runs.append({ runStart.position(), Vim::lineEndPosition(runEnd), true });
        }
    };
    for (auto block = Vim::navigableBlock(document(), _range.start);
         block.isValid() && block.position() <= _range.end;
         block = Vim::nextNavigableBlock(block)) {
        bool isConnected = runEnd.isValid() && isSameContainer(runEnd, block);
        for (auto between = runEnd.next(); isConnected && between.isValid() && between != block;
             between = between.next()) {
            isConnected = between.isVisible();
        }
        if (!isConnected) {
            finishRun();
            runStart = block;
        }
        runEnd = block;
    }
    finishRun();
    return runs;
}

int VimTextEditController::deleteLines(const QVector<Vim::TextRange>& _runs, int _fromRun)
{
    //
    // Чтобы строки действительно исчезли, а не превратились в пустые, захватываем ещё и
    // разделитель абзацев - следующий, а если за строками идёт скрытый текст, или они в конце
    // ячейки таблицы, то предыдущий, если же ни тот, ни другой удалить нельзя, то очищаем строки,
    // не трогая соседние абзацы
    // @note Удаляем с конца, чтобы не сдвигать позиции ещё не обработанных частей, а блоки
    //       получаем заново на каждом шаге, т.к. после правки документа они становятся невалидными
    //
    auto document = this->document();
    auto removeLines = [this, document](int _firstPosition, int _lastPosition) {
        const auto firstBlock = document->findBlock(_firstPosition);
        const auto lastBlock = document->findBlock(_lastPosition);
        const auto blockAfter = lastBlock.next();
        const auto blockBefore = firstBlock.previous();
        const int firstPosition = firstBlock.position();
        if (canMergeWith(lastBlock, blockAfter)) {
            //
            // ... а если строки только помечены удалёнными, то курсор встаёт за ними
            //
            const int afterPosition = blockAfter.position();
            return removeText(firstPosition, afterPosition) ? firstPosition : afterPosition;
        }
        if (canMergeWith(firstBlock, blockBefore)) {
            const int cursorPosition = blockBefore.position();
            removeText(Vim::lineEndPosition(blockBefore), Vim::lineEndPosition(lastBlock));
            return cursorPosition;
        }
        removeText(firstPosition, Vim::lineEndPosition(lastBlock));
        return firstPosition;
    };

    int cursorPosition = currentPosition();
    for (int index = _runs.size() - 1; index >= _fromRun; --index) {
        const auto& run = _runs.at(index);

        //
        // ... в ячейке таблицы удаляем строки по одной, т.к. редактор плохо справляется с
        //     выделением нескольких абзацев внутри ячейки
        //
        if (QTextCursor(document->findBlock(run.start)).currentTable() != nullptr) {
            QVector<int> lines;
            for (auto block = document->findBlock(run.start);
                 block.isValid() && block.position() <= run.end; block = block.next()) {
                lines.append(block.position());
            }
            for (int line = lines.size() - 1; line >= 0; --line) {
                cursorPosition = removeLines(lines.at(line), lines.at(line));
            }
            continue;
        }

        cursorPosition = removeLines(run.start, run.end);
    }
    return cursorPosition;
}

void VimTextEditController::changeRange(const Vim::TextRange& _range, QChar _registerName,
                                        bool _toNumberedRegister)
{
    if (!canEdit() || !_range.isValid()) {
        return;
    }

    //
    // ... в отличие от удаления, при построчном изменении первая строка остаётся на месте вместе
    //     с отступом, лишь очищается её содержимое, а строки за скрытым текстом удаляются целиком
    //
    int start = _range.start;
    int end = _range.end;
    QVector<Vim::TextRange> runs;
    if (_range.linewise) {
        runs = lineRuns(_range);
        if (runs.isEmpty()) {
            return;
        }
        const auto firstBlock = document()->findBlock(runs.constFirst().start);
        start = qMin(Vim::firstNonBlankPosition(firstBlock), Vim::lineEndPosition(firstBlock));
        end = runs.constFirst().end;
    } else if (!isEditableRange(start, end)) {
        fail();
        return;
    }

    //
    // ... пустой фрагмент, например ci( в (), как и в vim, регистры не трогает
    //
    if (_range.linewise || _range.start < _range.end) {
        shared().registers.setDeletedContent(_registerName, registerContent(_range),
                                             _toNumberedRegister);
    }
    if (runs.size() > 1) {
        deleteLines(runs, 1);
    }
    removeText(start, end);
    setCursorPosition(start);
    enterInsertMode(1, 'c');

    //
    // ... если в строке остался только отступ и ничего так и не будет введено, то при выходе
    //     из режима вставки отступ удаляется, как это делает vim
    //
    if (_range.linewise && start > _range.start) {
        m_autoIndentLine = QTextCursor(document());
        m_autoIndentLine.setPosition(start);
        m_autoIndentRevision = document()->revision();
    }
}

void VimTextEditController::changeCaseRange(const Vim::TextRange& _range, Operator _operator,
                                            int _cursorPosition)
{
    if (!canEdit() || !_range.isValid()) {
        return;
    }

    for (const auto& lineRange : lineRanges(_range)) {
        const int from = lineRange.start;
        const int to = lineRange.end;
        const auto block = document()->findBlock(from);
        const auto originalText = rangeText({ from, to, false });
        auto text = originalText;
        for (int index = 0; index < text.size(); ++index) {
            //
            // ... текст, который форматирование и так показывает в одном регистре, не трогаем,
            //     ведь изменение не будет видно, и модель документа его не сохранит
            //
            if (Vim::hasForcedCase(block, from + index)) {
                continue;
            }
            const auto character = text.at(index);
            switch (_operator) {
            case Operator::Lowercase: {
                text[index] = character.toLower();
                break;
            }
            case Operator::Uppercase: {
                text[index] = character.toUpper();
                break;
            }
            default: {
                text[index] = character.isUpper() ? character.toLower() : character.toUpper();
                break;
            }
            }
        }
        if (text != originalText) {
            replaceText(from, to, text);
        }
    }

    //
    // ... курсор встаёт в начало диапазона, а для построчного - туда, где начиналось
    //     перемещение, сохраняя столбец, как это делает vim
    //
    setCursorPosition(characterPosition(_cursorPosition >= 0 ? _cursorPosition : _range.start));
}

Vim::RegisterContent VimTextEditController::registerContent(const Vim::TextRange& _range) const
{
    //
    // В регистр попадает текст строк, в которых может находиться курсор, таким, каким его видно
    // на экране, ведь модель документа всё равно сохраняет его так, а если диапазон захватывает
    // перевод строки в конце, то и он тоже
    //
    Vim::RegisterContent content;
    content.linewise = _range.linewise;
    bool hasLine = false;
    for (auto block = document()->findBlock(_range.start);
         block.isValid() && block.position() <= _range.end; block = block.next()) {
        if (!Vim::isNavigableBlock(block)) {
            continue;
        }
        if (hasLine) {
            content.text += '\n';
        }
        hasLine = true;

        const int from = qMax(_range.start, block.position());
        const int to = qMin(_range.end, Vim::lineEndPosition(block));
        if (to <= from) {
            continue;
        }

        //
        // ... вместе с текстом запоминаем и начертание, отличающееся от стиля абзаца, чтобы
        //     вставленный текст выглядел так же
        //
        const int offset = content.text.size();
        const auto blockFormat = block.charFormat();
        for (auto iter = block.begin(); !iter.atEnd(); ++iter) {
            const auto fragment = iter.fragment();
            const int fragmentFrom = qMax(from, fragment.position());
            const int fragmentTo = qMin(to, fragment.position() + fragment.length());
            if (!fragment.isValid() || fragmentTo <= fragmentFrom) {
                continue;
            }
            const auto fragmentFont = fragment.charFormat().font();
            const auto blockFont = blockFormat.font();
            QTextCharFormat format;
            if (fragmentFont.bold() != blockFont.bold()) {
                format.setFontWeight(fragmentFont.bold() ? QFont::Bold : QFont::Normal);
            }
            if (fragmentFont.italic() != blockFont.italic()) {
                format.setFontItalic(fragmentFont.italic());
            }
            if (fragmentFont.underline() != blockFont.underline()) {
                format.setFontUnderline(fragmentFont.underline());
            }
            if (fragmentFont.strikeOut() != blockFont.strikeOut()) {
                format.setFontStrikeOut(fragmentFont.strikeOut());
            }
            if (format.propertyCount() > 0) {
                content.formats.append(
                    { offset + fragmentFrom - from, fragmentTo - fragmentFrom, format });
            }
        }
        content.text += Vim::displayedText(block).mid(from - block.position(), to - from);
    }

    if (!_range.linewise) {
        return content;
    }

    //
    // ... построчный фрагмент всегда заканчивается переводом строки, даже если последняя
    //     строка пуста
    //
    content.text += '\n';

    //
    // ... а чтобы при вставке строки сохранили свои типы, запоминаем и их
    //
    content.documentKind = documentKind();
    if (!content.documentKind.isEmpty()) {
        for (auto block = document()->findBlock(_range.start);
             block.isValid() && block.position() <= _range.end; block = block.next()) {
            if (Vim::isNavigableBlock(block)) {
                content.paragraphTypes.append(
                    static_cast<int>(BusinessLayer::TextBlockStyle::forBlock(block)));
            }
        }
    }
    return content;
}

Vim::RegisterContent VimTextEditController::registerValue(QChar _name) const
{
    //
    // ... регистры только для чтения хранят то, что знает только контроллер
    //
    if (_name == '.') {
        return { shared().lastInsertedText, false };
    }
    if (_name == ':') {
        return { shared().lastExCommand, false };
    }
    if (_name == '/') {
        return { shared().searchPattern, false };
    }
    return shared().registers.content(_name);
}

QString VimTextEditController::documentKind() const
{
    const auto textDocument = qobject_cast<BusinessLayer::TextDocument*>(document());
    if (textDocument == nullptr || textDocument->model() == nullptr) {
        return {};
    }

    //
    // ... один и тот же документ показывают разные редакторы, например сценарий и поэпизодник,
    //     и типы абзацев у них разные, поэтому учитываем и редактор
    //
    return textDocument->model()->mimeTypes().value(0) + '/'
        + QString::fromLatin1(m_editor->metaObject()->className());
}

void VimTextEditController::restoreParagraphTypes(int _position,
                                                  const Vim::RegisterContent& _content, int _count)
{
    const auto textDocument = qobject_cast<BusinessLayer::TextDocument*>(document());
    if (textDocument == nullptr || _content.paragraphTypes.isEmpty()
        || _content.documentKind != documentKind()
        || _content.paragraphTypes.size() != _content.text.count('\n')) {
        return;
    }

    //
    // Восстанавливаем только типы обычных абзацев: заголовки и окончания папок, а так же
    // служебные блоки так не создать, не сломав структуру документа
    //
    using BusinessLayer::TextParagraphType;
    auto canRestore = [](TextParagraphType _type) {
        switch (_type) {
        case TextParagraphType::UnformattedText:
        case TextParagraphType::InlineNote:
        case TextParagraphType::SceneHeading:
        case TextParagraphType::Character:
        case TextParagraphType::Dialogue:
        case TextParagraphType::SceneCharacters:
        case TextParagraphType::BeatHeading:
        case TextParagraphType::Action:
        case TextParagraphType::Parenthetical:
        case TextParagraphType::Lyrics:
        case TextParagraphType::Transition:
        case TextParagraphType::Shot:
        case TextParagraphType::Sound:
        case TextParagraphType::Music:
        case TextParagraphType::Cue:
        case TextParagraphType::PageHeading:
        case TextParagraphType::PanelHeading:
        case TextParagraphType::Description:
        case TextParagraphType::ChapterHeading1:
        case TextParagraphType::ChapterHeading2:
        case TextParagraphType::ChapterHeading3:
        case TextParagraphType::ChapterHeading4:
        case TextParagraphType::ChapterHeading5:
        case TextParagraphType::ChapterHeading6:
        case TextParagraphType::Text: {
            return true;
        }
        default: {
            return false;
        }
        }
    };

    //
    // ... позиции вставленных строк отслеживаем курсорами, т.к. смена типа меняет документ
    //
    QVector<QTextCursor> lines;
    const int linesCount = _content.paragraphTypes.size() * _count;
    for (auto block = document()->findBlock(_position);
         block.isValid() && lines.size() < linesCount; block = Vim::nextNavigableBlock(block)) {
        QTextCursor cursor(document());
        cursor.setPosition(block.position());
        lines.append(cursor);
    }

    for (int index = 0; index < lines.size(); ++index) {
        const auto type = static_cast<TextParagraphType>(
            _content.paragraphTypes.at(index % _content.paragraphTypes.size()));
        BusinessLayer::TextCursor cursor(lines.at(index));
        if (!canRestore(type) || BusinessLayer::TextBlockStyle::forBlock(cursor.block()) == type) {
            continue;
        }

        //
        // ... в ячейках таблиц не бывает заголовков, ведь они начинают новые сцены и группы
        //
        if (cursor.inTable() && isGroupHeading(type)) {
            continue;
        }
        textDocument->setParagraphType(type, cursor);
    }
}

void VimTextEditController::applyTextFormats(
    int _position, const QString& _text, const QVector<Vim::RegisterContent::TextFormat>& _formats)
{
    //
    // Вставленный текст идёт с заданной позиции, а каждая следующая его строка - с начала
    // очередной строки документа, при этом начертание применяем только там, где текст в документе
    // совпадает со вставленным, на случай, если редактор вставил его как-то иначе
    //
    auto block = document()->findBlock(_position);
    int lineStart = 0;
    int column = _position - block.position();
    for (const auto& format : _formats) {
        for (int lineEnd = _text.indexOf('\n', lineStart);
             lineEnd >= 0 && lineEnd < format.from && block.isValid();
             lineEnd = _text.indexOf('\n', lineStart)) {
            lineStart = lineEnd + 1;
            block = Vim::nextNavigableBlock(block);
            column = 0;
        }
        if (!block.isValid()) {
            return;
        }

        const int from = column + format.from - lineStart;
        if (block.text().mid(from, format.length) != _text.mid(format.from, format.length)) {
            continue;
        }
        QTextCursor cursor(document());
        cursor.setPosition(block.position() + from);
        cursor.setPosition(block.position() + from + format.length, QTextCursor::KeepAnchor);
        cursor.mergeCharFormat(format.format);
    }
}

QVector<Vim::TextRange> VimTextEditController::lineRanges(const Vim::TextRange& _range) const
{
    //
    // Части диапазона в строках, в которых может находиться курсор, без переводов строк
    // @note Идут в обратном порядке, чтобы правки в одной части не сдвигали предыдущие
    //
    QVector<Vim::TextRange> ranges;
    for (auto block = document()->findBlock(_range.start);
         block.isValid() && block.position() <= _range.end; block = block.next()) {
        if (!Vim::isNavigableBlock(block)) {
            continue;
        }
        const int from = qMax(_range.start, block.position());
        const int to = qMin(_range.end, Vim::lineEndPosition(block));
        if (to > from) {
            ranges.prepend({ from, to, false });
        }
    }
    return ranges;
}

QString VimTextEditController::rangeText(const Vim::TextRange& _range) const
{
    //
    // В регистр попадает только текст строк, в которых может находиться курсор, а если
    // диапазон захватывает перевод строки в конце, то и он тоже
    //
    QString text;
    bool hasLine = false;
    for (auto block = document()->findBlock(_range.start);
         block.isValid() && block.position() <= _range.end; block = block.next()) {
        if (!Vim::isNavigableBlock(block)) {
            continue;
        }
        if (hasLine) {
            text += '\n';
        }
        const int from = qMax(_range.start, block.position());
        const int to = qMin(_range.end, Vim::lineEndPosition(block));
        if (to > from) {
            text += block.text().mid(from - block.position(), to - from);
        }
        hasLine = true;
    }
    return text;
}


// ****


bool VimTextEditController::canEdit() const
{
    //
    // ... если в документе нет ни одной строки, по которой может перемещаться курсор, то любое
    //     изменение затронет скрытый, или служебный текст
    //
    return !m_editor->isReadOnly() && Vim::firstNavigableBlock(document()).isValid();
}

bool VimTextEditController::isEditableRange(int _from, int _to) const
{
    //
    // Изменение не должно затрагивать скрытый текст и выходить за пределы ячейки таблицы
    //
    const auto firstBlock = document()->findBlock(_from);
    const auto lastBlock = document()->findBlock(_to);
    for (auto block = firstBlock; block.isValid(); block = block.next()) {
        if (!block.isVisible() || !isSameContainer(firstBlock, block)) {
            return false;
        }
        if (block == lastBlock) {
            break;
        }
    }
    return true;
}

bool VimTextEditController::isProtectedSelectionEdit(QKeyEvent* _event) const
{
    //
    // Сочетания редактора, удаляющие выделение, в визуальном режиме не должны затрагивать
    // скрытый текст и выходить за пределы ячейки таблицы, как и команды vim
    //
    if (!isVisualMode()
        || (!_event->matches(QKeySequence::Cut) && !_event->matches(QKeySequence::Paste)
            && !_event->matches(QKeySequence::DeleteStartOfWord)
            && !_event->matches(QKeySequence::DeleteEndOfWord))) {
        return false;
    }

    const auto cursor = m_editor->textCursor();
    return cursor.hasSelection()
        && !isEditableRange(cursor.selectionStart(), cursor.selectionEnd());
}

bool VimTextEditController::removeText(int _from, int _to)
{
    if (_from >= _to) {
        return true;
    }

    //
    // Удаление отдаём самому редактору, чтобы он корректно обработал границы абзацев, таблиц
    // и режим отслеживания изменений
    // @note Выделяем от конца к началу, т.к. в режиме отслеживания изменений редактор проверяет,
    //       не добавленный ли это текст, начиная с блока, в котором находится курсор
    //
    const int charactersCount = document()->characterCount();
    QTextCursor cursor(document());
    cursor.setPosition(_to);
    cursor.setPosition(_from, QTextCursor::KeepAnchor);
    m_editor->setTextCursor(cursor);
    sendKeyPressToEditor(Qt::Key_Delete, {});

    //
    // ... в режиме отслеживания изменений текст лишь помечается удалённым
    //
    return !m_editor->isTrackChangesEnabled() || document()->characterCount() < charactersCount;
}

bool VimTextEditController::insertText(int _position, const QString& _text)
{
    if (_text.isEmpty()) {
        return true;
    }

    QTextCursor cursor(document());
    cursor.setPosition(_position);
    if (_text.contains('\n') && isFolderBoundary(cursor.block())) {
        fail();
        return false;
    }
    cursor.beginEditBlock();

    //
    // Если вставка с переводами строк идёт в начало абзаца, то его данные должны остаться
    // с его же текстом, который окажется в последнем из вставленных абзацев
    //
    auto block = cursor.block();
    BusinessLayer::TextBlockData* movedData = nullptr;
    if (cursor.atBlockStart() && block.length() > 1 && _text.contains('\n')) {
        if (const auto data = dynamic_cast<BusinessLayer::TextBlockData*>(block.userData())) {
            movedData = new BusinessLayer::TextBlockData(data);
            block.setUserData(nullptr);
        }
    }

    cursor.insertText(_text);
    if (movedData != nullptr) {
        cursor.block().setUserData(movedData);
    }
    cursor.endEditBlock();
    m_editor->markInsertedText(_position, _position + _text.size());
    return true;
}

bool VimTextEditController::replaceText(int _from, int _to, const QString& _text,
                                        int* _textPosition)
{
    if (_textPosition != nullptr) {
        *_textPosition = _from;
    }

    QTextCursor cursor(document());
    if (_text.contains('\n') && isFolderBoundary(document()->findBlock(_from))) {
        fail();
        return false;
    }

    //
    // В режиме отслеживания изменений заменяемый текст помечается удалённым, а новый
    // вставляется за ним и помечается добавленным, с форматированием первого заменяемого символа
    //
    if (m_editor->isTrackChangesEnabled() && _to > _from) {
        cursor.setPosition(_from + 1);
        const auto format = cursor.charFormat();
        const int position = removeText(_from, _to) ? _from : _to;
        if (_textPosition != nullptr) {
            *_textPosition = position;
        }
        if (_text.contains('\n')) {
            return insertText(position, _text);
        }
        cursor.setPosition(position);
        cursor.insertText(_text, format);
        m_editor->markInsertedText(position, position + _text.size());
        return true;
    }

    cursor.beginEditBlock();

    //
    // Заменяем текст частями с одинаковым форматированием, чтобы его не потерять
    // @note Части собираем заранее, т.к. правка блока может объединить его фрагменты
    //
    if (_text.size() == _to - _from) {
        struct Part {
            int from = 0;
            int to = 0;
            QTextCharFormat format;
        };
        //
        // ... формат каждого символа узнаём курсором, а не перебором фрагментов блока, которых в
        //     длинном абзаце после множества правок может быть очень много
        //
        QVector<Part> parts;
        QTextCursor probe(document());
        for (int position = _from; position < _to; ++position) {
            probe.setPosition(position + 1);
            const auto format = probe.charFormat();
            if (!parts.isEmpty() && parts.constLast().to == position
                && parts.constLast().format == format) {
                parts.last().to = position + 1;
            } else {
                parts.append({ position, position + 1, format });
            }
        }
        for (const auto& part : parts) {
            cursor.setPosition(part.from);
            cursor.setPosition(part.to, QTextCursor::KeepAnchor);
            cursor.insertText(_text.mid(part.from - _from, part.to - part.from), part.format);
        }
        cursor.endEditBlock();
        m_editor->markInsertedText(_from, _to);
        return true;
    }

    //
    // ... а новый текст получает форматирование первого заменяемого символа
    //
    cursor.setPosition(_from);
    if (_to > _from) {
        cursor.setPosition(_from + 1);
    }
    const auto format = cursor.charFormat();
    cursor.setPosition(_from);
    cursor.setPosition(_to, QTextCursor::KeepAnchor);
    if (_text.contains('\n')) {
        cursor.removeSelectedText();
        cursor.endEditBlock();
        return insertText(_from, _text);
    }
    cursor.insertText(_text, format);
    cursor.endEditBlock();
    m_editor->markInsertedText(_from, _from + _text.size());
    return true;
}

bool VimTextEditController::openLine(bool _below)
{
    if (!canEdit()) {
        return false;
    }

    //
    // Строка выше открывается как строка под предыдущей, чтобы тип нового абзаца выбрал
    // редактор, как при нажатии Enter в конце строки, а абзац, разорванный переносом на другую
    // страницу, обрабатываем целиком
    //
    using BusinessLayer::TextBlockStyle;
    auto document = this->document();
    auto block = currentBlock();
    bool below = _below;
    if (below) {
        while (block.blockFormat().boolProperty(TextBlockStyle::PropertyIsBreakCorrectionStart)) {
            const auto next = Vim::nextNavigableBlock(block);
            if (!next.isValid()) {
                break;
            }
            block = next;
        }
    } else {
        while (block.blockFormat().boolProperty(TextBlockStyle::PropertyIsBreakCorrectionEnd)) {
            const auto previous = Vim::previousNavigableBlock(block);
            if (!previous.isValid()) {
                break;
            }
            block = previous;
        }
        const auto previous = block.previous();
        if (previous.isValid() && Vim::isNavigableBlock(previous)
            && isSameContainer(previous, block)) {
            block = previous;
            below = true;
        }
    }

    const int blocksCount = document->blockCount();
    const int blockStart = block.position();
    const int blockEnd = Vim::lineEndPosition(block);

    //
    // Под окончанием папки строка открывается за самой папкой, перед следующим абзацем, ведь
    // редактор по Enter в окончании папки создаёт новую папку
    //
    if (below && isFolderFooter(block)) {
        const auto next = block.next();
        if (!next.isValid() || !next.isVisible() || isFolderBoundary(next)
            || !isSameContainer(block, next)) {
            return false;
        }
        const int nextPosition = next.position();
        if (!insertText(nextPosition, QStringLiteral("\n"))) {
            return false;
        }
        QTextCursor cursor(document);
        cursor.setPosition(nextPosition);
        m_editor->setTextCursor(cursor);
        return true;
    }

    //
    // Новый абзац создаёт сам редактор, но в пустом абзаце нажатие Enter меняет его тип,
    // поэтому там вставляем абзац сами
    //
    if (block.length() > 1) {
        const int revision = document->revision();
        QTextCursor original(document);
        original.setPosition(blockStart);
        QTextCursor cursor(document);
        cursor.setPosition(below ? blockEnd : blockStart);
        m_editor->setTextCursor(cursor);
        sendKeyPressToEditor(Qt::Key_Return, QStringLiteral("\r"));

        auto newBlock = m_editor->textCursor().block();
        if (!below && newBlock.length() > 1) {
            newBlock = newBlock.previous();
        }
        //
        // ... новой строкой считаем только пустой абзац в той же ячейке, а не, например,
        //     разделитель перед таблицей
        //
        if (newBlock.isValid() && newBlock.length() == 1 && newBlock != original.block()
            && Vim::isNavigableBlock(newBlock) && isSameContainer(newBlock, original.block())) {
            QTextCursor newCursor(document);
            newCursor.setPosition(newBlock.position());
            m_editor->setTextCursor(newCursor);
            return true;
        }

        //
        // ... если редактор изменил документ как-то иначе, то сами ничего не добавляем
        //
        if (document->revision() != revision) {
            return false;
        }
    }

    const int position = below ? blockEnd : blockStart;
    if (!insertText(position, QStringLiteral("\n"))) {
        return false;
    }
    QTextCursor cursor(document);
    cursor.setPosition(below ? position + 1 : blockStart);
    m_editor->setTextCursor(cursor);
    return document->blockCount() > blocksCount;
}

void VimTextEditController::deleteCharacters(int _count, bool _previous)
{
    if (!canEdit()) {
        return;
    }

    const auto block = currentBlock();
    const int position = currentPosition();
    const int lineStart = block.position();
    const int lineEnd = Vim::lineEndPosition(block);
    if (lineEnd <= lineStart) {
        return;
    }

    const int start = _previous ? movePositionByCharacters(block, position, -_count) : position;
    const int end = _previous ? position : movePositionByCharacters(block, position, _count);
    if (end <= start) {
        //
        // ... X в начале строки ничего не удаляет, о чём и сообщаем, а x в пустой строке - нет
        //
        if (_previous) {
            fail();
        }
        return;
    }

    deleteRange({ start, end, false }, m_pendingRegister, false);
}

void VimTextEditController::replaceCharacters(const QString& _character, int _count)
{
    if (!canEdit()) {
        return;
    }

    //
    // ... если в строке не хватает символов, то замена не выполняется вовсе
    //
    const int position = currentPosition();
    int moved = 0;
    const int end = movePositionByCharacters(currentBlock(), position, _count, &moved);
    if (moved < _count) {
        fail();
        return;
    }

    int textPosition = position;
    replaceText(position, end, _character.repeated(_count), &textPosition);

    //
    // ... курсор остаётся на последнем заменённом символе
    //
    setCursorPosition(textPosition + (_count - 1) * _character.size());
}

void VimTextEditController::incrementNumber(int _delta)
{
    if (!canEdit()) {
        return;
    }

    //
    // Ищем число под курсором, или первое число правее него, как это делает vim: десятичное,
    // шестнадцатеричное с префиксом 0x, или двоичное с префиксом 0b
    //
    const auto block = currentBlock();
    const auto text = block.text();
    const int column = currentPosition() - block.position();
    auto isDigit = [](QChar _character, int _base) {
        const auto code = _character.unicode();
        if (_base == 16) {
            return (code >= '0' && code <= '9') || (code >= 'a' && code <= 'f')
                || (code >= 'A' && code <= 'F');
        }
        return code >= '0' && code < '0' + qMin(_base, 10);
    };
    auto prefixedNumberAt = [&text, &isDigit](int _index, int* _base) {
        if (_index + 2 < text.size() && text.at(_index) == '0') {
            const auto prefix = text.at(_index + 1).toLower();
            const int base = prefix == 'x' ? 16 : (prefix == 'b' ? 2 : 0);
            if (base > 0 && isDigit(text.at(_index + 2), base)) {
                *_base = base;
                return true;
            }
        }
        return false;
    };

    int start = -1;
    int base = 10;
    //
    // ... курсор может стоять внутри числа с префиксом, поэтому ищем префикс от начала слова
    //
    int wordStart = qMin(column, static_cast<int>(text.size()));
    while (wordStart > 0 && text.at(wordStart - 1).unicode() < 0x80
           && text.at(wordStart - 1).isLetterOrNumber()) {
        --wordStart;
    }
    for (int index = wordStart; index <= column && start < 0; ++index) {
        int prefixBase = 0;
        if (prefixedNumberAt(index, &prefixBase)) {
            int numberEnd = index + 2;
            while (numberEnd < text.size() && isDigit(text.at(numberEnd), prefixBase)) {
                ++numberEnd;
            }
            if (column < numberEnd) {
                start = index;
                base = prefixBase;
            }
        }
    }
    if (start < 0 && column < text.size() && isDigit(text.at(column), 10)) {
        start = column;
        while (start > 0 && isDigit(text.at(start - 1), 10)) {
            --start;
        }
    }
    if (start < 0) {
        for (int index = column; index < text.size(); ++index) {
            int prefixBase = 0;
            if (prefixedNumberAt(index, &prefixBase)) {
                start = index;
                base = prefixBase;
                break;
            }
            if (isDigit(text.at(index), 10)) {
                start = index;
                break;
            }
        }
    }
    if (start < 0) {
        fail();
        return;
    }

    const int digitsStart = base == 10 ? start : start + 2;
    int end = digitsStart;
    while (end < text.size() && isDigit(text.at(end), base)) {
        ++end;
    }
    const auto digits = text.mid(digitsStart, end - digitsStart);

    QString result;
    int replaceStart = start;
    if (base == 10) {
        //
        // ... десятичное число может быть отрицательным, а ведущие нули сохраняются
        //
        const bool negative = start > 0 && text.at(start - 1) == '-';
        if (negative) {
            replaceStart = start - 1;
        }
        bool ok = false;
        const qint64 number = digits.toLongLong(&ok);
        if (!ok || number > std::numeric_limits<qint64>::max() - qAbs(_delta)) {
            fail();
            return;
        }
        const qint64 value = (negative ? -1 : 1) * number + _delta;
        result = QString::number(qAbs(value));
        if (digits.size() > 1 && digits.startsWith('0') && result.size() < digits.size()) {
            result = result.rightJustified(digits.size(), '0');
        }
        if (value < 0) {
            result.prepend('-');
        }
    } else {
        //
        // ... а шестнадцатеричное и двоичное - беззнаковые, сохраняют ширину и регистр букв
        //
        bool ok = false;
        const quint64 value = digits.toULongLong(&ok, base) + static_cast<quint64>(_delta);
        if (!ok) {
            fail();
            return;
        }
        result = QString::number(value, base);
        if (result.size() < digits.size()) {
            result = result.rightJustified(digits.size(), '0');
        }
        if (digits != digits.toLower()) {
            result = result.toUpper();
        }
        result.prepend(text.mid(start, 2));
    }

    const int position = block.position() + replaceStart;
    int textPosition = position;
    if (replaceText(position, block.position() + end, result, &textPosition)) {
        setCursorPosition(textPosition + result.size() - 1);
    }
}

void VimTextEditController::toggleCase(int _count)
{
    if (!canEdit()) {
        return;
    }

    const int position = currentPosition();
    const int end = movePositionByCharacters(currentBlock(), position, _count);
    if (end <= position) {
        return;
    }

    //
    // ... в режиме отслеживания изменений заменённые символы остаются в тексте, и курсор встаёт
    //     за добавленными вместо них
    //
    const int charactersCount = document()->characterCount();
    changeCaseRange({ position, end, false }, Operator::SwapCase);
    setCursorPosition(characterPosition(end + document()->characterCount() - charactersCount));
}

void VimTextEditController::joinLines(int _count, bool _withSpaces)
{
    if (!canEdit()) {
        return;
    }

    auto document = this->document();
    auto block = currentBlock();
    int joinPosition = -1;
    QElapsedTimer timer;
    timer.start();
    for (int step = 0; step < qMax(1, _count - 1); ++step) {
        if (timer.elapsed() > kSearchTimeoutMs) {
            fail(QCoreApplication::translate("Ui::VimTextEditController", "Interrupted"));
            break;
        }

        const auto next = Vim::nextNavigableBlock(block);
        if (!next.isValid()) {
            if (step == 0) {
                fail();
            }
            break;
        }

        //
        // ... не объединяем со скрытым текстом, с продолжением абзаца на следующей странице, с
        //     другой ячейкой таблицы и с заголовками и окончаниями папок, ведь редактор при этом
        //     удалит саму папку
        //
        using BusinessLayer::TextBlockStyle;
        bool canJoin
            = !block.blockFormat().boolProperty(TextBlockStyle::PropertyIsBreakCorrectionStart)
            && isSameContainer(block, next) && !isFolderBoundary(block) && !isFolderBoundary(next);
        for (auto between = block.next(); canJoin && between.isValid() && between != next;
             between = between.next()) {
            canJoin = between.isVisible();
        }
        if (!canJoin) {
            if (step == 0) {
                fail();
            }
            break;
        }

        const auto currentText = block.text();
        const auto nextText = next.text();
        const int lineEnd = Vim::lineEndPosition(block);

        //
        // ... само объединение делает редактор, чтобы корректно обработать декорации, а получилось
        //     ли оно, определяем по тому, оказался ли текст следующей строки в том же абзаце
        //
        QTextCursor currentEnd(document);
        currentEnd.setPosition(lineEnd);
        QTextCursor nextStart(document);
        nextStart.setPosition(next.position());
        QTextCursor nextEnd(document);
        nextEnd.setPosition(Vim::lineEndPosition(next));
        QTextCursor cursor(document);
        cursor.setPosition(lineEnd);
        m_editor->setTextCursor(cursor);
        sendKeyPressToEditor(Qt::Key_Delete, {});
        if (!isSameParagraph(currentEnd.block(), nextStart.block())) {
            if (step == 0) {
                fail();
            }
            break;
        }
        joinPosition = nextStart.position();
        block = nextEnd.block();

        if (!_withSpaces) {
            continue;
        }

        //
        // ... как в vim, убираем отступ присоединённой строки и вставляем один пробел, если
        //     строка не пуста, не начинается с закрывающей скобки и предыдущая не заканчивается
        //     пробелом
        //
        const int leadingWhite = leadingWhiteCount(nextText);
        if (leadingWhite > 0) {
            removeText(nextStart.position(), nextStart.position() + leadingWhite);
        }
        const auto rest = nextText.mid(leadingWhite);
        const bool insertSpace = !rest.isEmpty() && !rest.startsWith(')') && !currentText.isEmpty()
            && !currentText.endsWith(' ') && !currentText.endsWith('\t');
        if (insertSpace) {
            joinPosition = nextStart.position();
            QTextCursor spaceCursor(document);
            spaceCursor.setPosition(joinPosition);
            spaceCursor.insertText(QStringLiteral(" "));
            m_editor->markInsertedText(joinPosition, joinPosition + 1);
        }
        block = nextEnd.block();
    }

    if (joinPosition >= 0) {
        setCursorPosition(characterPosition(joinPosition));
    } else {
        setCursorPosition(currentPosition());
    }
}

void VimTextEditController::paste(QChar _registerName, bool _before, int _count, bool _cursorAfter)
{
    const auto content = registerValue(_registerName);
    if (content.isEmpty()) {
        fail(
            QCoreApplication::translate("Ui::VimTextEditController", "E353: Nothing in register %1")
                .arg(_registerName.isNull() ? QChar('"') : _registerName));
        return;
    }
    pasteContent(content, _before, _count, _cursorAfter);
}

void VimTextEditController::pasteContent(const Vim::RegisterContent& _content, bool _before,
                                         int _count, bool _cursorAfter)
{
    if (!canEdit()) {
        return;
    }
    if (_content.isEmpty()) {
        fail();
        return;
    }

    if (_content.linewise) {
        pasteLines(_content, _before, _count, _cursorAfter);
        return;
    }

    const auto block = currentBlock();
    int position = currentPosition();
    if (!_before && block.length() > 1) {
        position = Vim::nextCharacterPosition(block, position);
    }

    if (static_cast<qint64>(_content.text.size()) * _count > kMaxPastedTextLength) {
        fail(QCoreApplication::translate("Ui::VimTextEditController",
                                         "E1240: Resulting text too long"));
        return;
    }

    const auto text = _content.text.repeated(_count);
    if (!insertText(position, text)) {
        return;
    }
    applyTextFormats(position, text, repeatedFormats(_content, _count, _content.text.size()));

    //
    // ... курсор встаёт на последний вставленный символ, а при многострочной вставке - на первый
    //
    if (_cursorAfter) {
        setCursorPosition(position + text.size());
        return;
    }
    setCursorPosition(text.contains('\n') ? position
                                          : characterPosition(position + text.size() - 1));
}

void VimTextEditController::pasteLines(const Vim::RegisterContent& _content, bool _before,
                                       int _count, bool _cursorAfter)
{
    auto text = _content.text;
    if (text.endsWith('\n')) {
        text.chop(1);
    }
    if (static_cast<qint64>(text.size() + 1) * _count > kMaxPastedTextLength
        || static_cast<qint64>(text.count('\n') + 1) * _count > kMaxPastedLines) {
        fail(QCoreApplication::translate("Ui::VimTextEditController",
                                         "E1240: Resulting text too long"));
        return;
    }

    QStringList lines;
    for (int index = 0; index < _count; ++index) {
        lines.append(text);
    }

    if (!openLine(!_before)) {
        fail();
        return;
    }

    const int position = m_editor->textCursor().position();
    const auto linesText = lines.join('\n');
    if (!insertText(position, linesText)) {
        return;
    }
    restoreParagraphTypes(position, _content, _count);
    applyTextFormats(position, linesText, repeatedFormats(_content, _count, text.size() + 1));

    if (_cursorAfter) {
        //
        // ... курсор встаёт в начало строки, следующей за вставленными
        //
        auto lastBlock = Vim::navigableBlock(document(), position);
        for (int index = 1; index < lines.size() * (text.count('\n') + 1); ++index) {
            const auto next = Vim::nextNavigableBlock(lastBlock);
            if (!next.isValid()) {
                break;
            }
            lastBlock = next;
        }
        const auto next = Vim::nextNavigableBlock(lastBlock);
        setCursorPosition(next.isValid() ? next.position() : lastBlock.position());
        return;
    }
    setCursorPosition(Vim::firstNonBlankPosition(document()->findBlock(position)));
}

void VimTextEditController::replaceVisualWithRegister(QChar _registerName, bool _keepRegisters,
                                                      int _count)
{
    const auto content = registerValue(_registerName);
    const auto range = visualRange();
    const bool wasLinewise = m_mode == Mode::VisualLine;
    leaveVisualMode();

    if (!range.isValid() || content.isEmpty() || !canEdit()) {
        return;
    }

    //
    // Удаляем выделение, при этом удалённый текст попадает в регистр по умолчанию, если это не
    // команда P, которая сохраняет регистры как есть
    //
    auto document = this->document();
    deleteRange(range, _keepRegisters ? QChar('_') : QChar(), false);

    if (wasLinewise) {
        //
        // ... на место строк вставляем строки, даже если в регистре был фрагмент строки, а если
        //     строки были удалены в конце документа, то курсор встал выше, и вставлять нужно под
        //     ним
        //
        pasteLines(content, currentBlock().position() >= range.start, _count);
        return;
    }

    const int position = currentPosition();
    if (content.linewise) {
        //
        // ... построчный фрагмент разрывает строку
        //
        auto text = content.text;
        if (text.endsWith('\n')) {
            text.chop(1);
        }
        QStringList lines;
        for (int index = 0; index < _count; ++index) {
            lines.append(text);
        }
        const int splitPosition = position < range.start ? range.start : position;
        if (!insertText(splitPosition,
                        QStringLiteral("\n") + lines.join('\n') + QStringLiteral("\n"))) {
            return;
        }
        restoreParagraphTypes(splitPosition + 1, content, _count);
        setCursorPosition(Vim::firstNonBlankPosition(document->findBlock(splitPosition + 1)));
        return;
    }

    //
    // ... если удаление было в конце строки, то курсор встал левее, и вставлять нужно за ним
    //
    pasteContent(content, position >= range.start, _count);
}

void VimTextEditController::undo(int _count)
{
    if (isVisualMode()) {
        enterNormalMode();
    }
    m_commandIsChange = false;
    for (int index = 0; index < _count; ++index) {
        triggerEditorShortcut(QKeySequence::Undo);
    }
    setCursorPosition(characterPosition(m_editor->textCursor().position()));
}

void VimTextEditController::redo(int _count)
{
    if (isVisualMode()) {
        enterNormalMode();
    }
    m_commandIsChange = false;
    for (int index = 0; index < _count; ++index) {
        triggerEditorShortcut(QKeySequence::Redo);
    }
    setCursorPosition(characterPosition(m_editor->textCursor().position()));
}

void VimTextEditController::triggerEditorShortcut(QKeySequence::StandardKey _key)
{
    if (!canEdit()) {
        return;
    }

    //
    // Отмена и повтор действия реализованы в самих редакторах, поэтому имитируем нажатие
    // системного сочетания клавиш, чтобы редактор обработал его сам
    //
    const auto keyBindings = QKeySequence::keyBindings(_key);
    if (keyBindings.isEmpty() || keyBindings.constFirst().count() == 0) {
        return;
    }

#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
    const int keyCombination = keyBindings.constFirst()[0].toCombined();
#else
    const int keyCombination = keyBindings.constFirst()[0];
#endif
    const auto key = keyCombination & ~Qt::KeyboardModifierMask;
    const auto modifiers = Qt::KeyboardModifiers(keyCombination & Qt::KeyboardModifierMask);

    QScopedValueRollback<int> passThroughRollback(m_passThroughDepth, m_passThroughDepth + 1);
    QKeyEvent event(QEvent::KeyPress, key, modifiers);
    QGuiApplication::sendEvent(m_editor, &event);
}

void VimTextEditController::sendKeyPressToEditor(int _key, const QString& _text,
                                                 Qt::KeyboardModifiers _modifiers)
{
    QScopedValueRollback<int> passThroughRollback(m_passThroughDepth, m_passThroughDepth + 1);
    QKeyEvent event(QEvent::KeyPress, _key, _modifiers, _text);
    QGuiApplication::sendEvent(m_editor, &event);
}

void VimTextEditController::commitUndoStep()
{
    m_hasUncommittedChange = false;

    //
    // Отмена в редакторах работает через модель, которая сохраняет изменения с задержкой,
    // поэтому сохраняем их сразу, чтобы каждая команда отменялась отдельно, как в vim
    //
    const auto textDocument = qobject_cast<BusinessLayer::TextDocument*>(document());
    if (textDocument == nullptr || textDocument->model() == nullptr) {
        return;
    }

    textDocument->model()->saveChanges();
}


// ****


void VimTextEditController::setMark(QChar _name, int _position)
{
    QTextCursor cursor(document());
    cursor.setPosition(Vim::Cursor(document(), _position).position());
    m_marks[_name] = cursor;
}

void VimTextEditController::rememberJumpPosition()
{
    setMark('`', currentPosition());
}

int VimTextEditController::searchPosition(const QString& _pattern, int _fromPosition, bool _forward,
                                          int* _matchEnd)
{
    if (_pattern.isEmpty()) {
        return -1;
    }

    const auto expression = Vim::regularExpression(_pattern);
    auto document = this->document();
    const Vim::Cursor start(document, _fromPosition);
    const auto startBlock = start.block();
    QElapsedTimer timer;
    timer.start();
    bool interrupted = false;

    //
    // Ищем в строках, по которым может перемещаться курсор, продолжая с другого конца документа
    //
    int foundEnd = -1;
    auto findInBlock
        = [&expression, &timer, &interrupted, &foundEnd](const QTextBlock& _block, int _fromColumn,
                                                         int _toColumn, bool _forward) {
              if (interrupted || timer.elapsed() > kSearchTimeoutMs) {
                  interrupted = true;
                  return -1;
              }

              //
              // ... совпадения ищем с начала строки, т.к. с \zs начало совпадения может оказаться
              //     дальше того места, откуда шаблон начинает сопоставляться
              //
              const auto text = Vim::displayedText(_block);
              const auto options = matchOptions(text);
              int found = -1;
              int column = 0;
              while (column <= text.size()) {
                  const auto match
                      = expression.match(text, column, QRegularExpression::NormalMatch, options);
                  if (!match.hasMatch()) {
                      break;
                  }
                  const int matchStart = static_cast<int>(match.capturedStart());
                  if (matchStart > _toColumn) {
                      break;
                  }
                  if (matchStart >= _fromColumn) {
                      found = matchStart;
                      foundEnd = _block.position() + static_cast<int>(match.capturedEnd());
                      if (_forward) {
                          break;
                      }
                  }
                  //
                  // ... следующую попытку начинаем со следующего символа, не разрывая суррогатную
                  // пару
                  //
                  column = qMax(column + 1, matchStart + 1);
                  if (column < text.size() && text.at(column).isLowSurrogate()
                      && text.at(column - 1).isHighSurrogate()) {
                      ++column;
                  }
              }
              return found >= 0 ? _block.position() + found : -1;
          };
    auto result = [&interrupted, &foundEnd, _matchEnd, this](int _found) {
        if (interrupted) {
            fail(QCoreApplication::translate("Ui::VimTextEditController", "Interrupted"));
            return -1;
        }
        if (_matchEnd != nullptr) {
            *_matchEnd = foundEnd;
        }
        return _found;
    };

    if (_forward) {
        int found = findInBlock(startBlock, start.column() + 1, startBlock.length(), true);
        if (found >= 0) {
            return result(found);
        }
        for (auto block = Vim::nextNavigableBlock(startBlock); block.isValid() && !interrupted;
             block = Vim::nextNavigableBlock(block)) {
            found = findInBlock(block, 0, block.length(), true);
            if (found >= 0) {
                return result(found);
            }
        }
        for (auto block = Vim::firstNavigableBlock(document);
             block.isValid() && !interrupted && block.blockNumber() <= startBlock.blockNumber();
             block = Vim::nextNavigableBlock(block)) {
            found = findInBlock(block, 0, block == startBlock ? start.column() : block.length(),
                                true);
            if (found >= 0) {
                return result(found);
            }
        }
        return result(-1);
    }

    int found = findInBlock(startBlock, 0, start.column() - 1, false);
    if (found >= 0) {
        return result(found);
    }
    for (auto block = Vim::previousNavigableBlock(startBlock); block.isValid() && !interrupted;
         block = Vim::previousNavigableBlock(block)) {
        found = findInBlock(block, 0, block.length(), false);
        if (found >= 0) {
            return result(found);
        }
    }
    for (auto block = Vim::lastNavigableBlock(document);
         block.isValid() && !interrupted && block.blockNumber() >= startBlock.blockNumber();
         block = Vim::previousNavigableBlock(block)) {
        found = findInBlock(block, block == startBlock ? start.column() : 0, block.length(), false);
        if (found >= 0) {
            return result(found);
        }
    }
    return result(-1);
}

Vim::TextRange VimTextEditController::searchMatchRange(bool _forward)
{
    const auto pattern = shared().searchPattern;
    if (pattern.isEmpty()) {
        fail(QCoreApplication::translate("Ui::VimTextEditController",
                                         "E35: No previous regular expression"));
        return {};
    }

    //
    // Сперва проверяем, не стоит ли курсор на совпадении, а затем ищем ближайшее
    //
    const int position = currentPosition();
    const auto block = currentBlock();
    const auto text = Vim::displayedText(block);
    const auto expression = Vim::regularExpression(pattern);
    const auto options = matchOptions(text);
    int column = 0;
    while (column <= text.size()) {
        const auto match = expression.match(text, column, QRegularExpression::NormalMatch, options);
        if (!match.hasMatch()) {
            break;
        }
        const int start = block.position() + static_cast<int>(match.capturedStart());
        const int end = block.position() + static_cast<int>(match.capturedEnd());
        if (start > position) {
            break;
        }
        if (position < qMax(end, start + 1)) {
            return { start, end, false };
        }
        column = qMax(column + 1, static_cast<int>(match.capturedEnd()));
        if (column < text.size() && text.at(column).isLowSurrogate()
            && text.at(column - 1).isHighSurrogate()) {
            ++column;
        }
    }

    int end = -1;
    const int start = searchPosition(pattern, position, _forward, &end);
    if (start < 0) {
        if (m_message.isEmpty()) {
            fail(QCoreApplication::translate("Ui::VimTextEditController",
                                             "E486: Pattern not found: %1")
                     .arg(pattern));
        }
        return {};
    }
    return { start, qMax(start, end), false };
}

void VimTextEditController::searchWordUnderCursor(bool _forward, bool _wholeWord, int _count)
{
    bool isKeyword = false;
    int wordStart = -1;
    const auto word = Vim::wordUnderCursor(document(), currentPosition(), &isKeyword, &wordStart);
    if (word.isEmpty()) {
        resetPendingCommand();
        return;
    }

    auto pattern = escapedPattern(word);
    if (_wholeWord && isKeyword) {
        pattern = QStringLiteral("\\<%1\\>").arg(pattern);
    }
    shared().searchPattern = pattern;
    shared().searchOffset.clear();
    shared().searchForward = _forward;

    const auto motion = searchMotion(pattern, _forward, _count, wordStart);
    applyMotion(motion);
}


// ****


void VimTextEditController::scrollHalfPage(bool _down, int _count)
{
    //
    // Курсор перемещается на заданное количество строк, или на половину экрана, а документ
    // прокручивается вместе с ним
    //
    const auto cursorRect = m_editor->cursorRect();
    const int lineHeight = qMax(1, cursorRect.height());
    const int lines
        = _count > 0 ? _count : qMax(1, m_editor->viewport()->height() / lineHeight / 2);

    QTextCursor cursor(document());
    cursor.setPosition(currentPosition());
    for (int step = 0; step < lines; ++step) {
        if (!cursor.movePosition(_down ? QTextCursor::Down : QTextCursor::Up)) {
            break;
        }
    }

    if (isVisualMode()) {
        m_visualCursor = cursor.position();
        updateVisualSelection();
    } else {
        setCursorPosition(characterPosition(cursor.position()));
    }
    const auto newCursorRect = m_editor->cursorRect();
    m_editor->scrollVerticallyBy(newCursorRect.top() - cursorRect.top());
}

void VimTextEditController::scrollCursorTo(int _alignment)
{
    m_editor->ensureCursorVisible();

    const auto cursorRect = m_editor->cursorRect();
    const int viewportHeight = m_editor->viewport()->height();
    int delta = 0;
    switch (_alignment) {
    case ScrollToCenter: {
        delta = cursorRect.top() - (viewportHeight - cursorRect.height()) / 2;
        break;
    }
    case ScrollToTop: {
        delta = cursorRect.top();
        break;
    }
    case ScrollToBottom:
    default: {
        delta = cursorRect.bottom() - viewportHeight;
        break;
    }
    }

    m_editor->scrollVerticallyBy(delta);
}


// ****


void VimTextEditController::openCommandLine(const QString& _prefix)
{
    //
    // ... поиск может быть целью оператора, поэтому запоминаем набранную команду
    //
    m_commandLineOperator = m_pendingOperator;
    m_commandLineOperatorCount = takeOptionalCount();
    m_commandLineRegister = m_pendingRegister;
    resetPendingCommand();
    m_commandLineActive = true;
    m_commandLine = _prefix;
}

void VimTextEditController::closeCommandLine()
{
    m_commandLineActive = false;
    m_commandLineAwaitingRegister = false;
    m_commandLine.clear();
    m_commandLineOperator = Operator::None;
    m_commandLineOperatorCount = 0;
    m_commandLineRegister = QChar();
}

void VimTextEditController::handleCommandLine(QKeyEvent* _event)
{
    if (isEnter(_event)) {
        const auto command = m_commandLine;
        const auto pendingOperator = m_commandLineOperator;
        const int count = m_commandLineOperatorCount;
        const auto registerName = m_commandLineRegister;
        closeCommandLine();
        m_pendingOperator = pendingOperator;
        m_count = count > 0 ? QString::number(count) : QString();
        m_pendingRegister = registerName;
        executeCommandLine(command);
        return;
    }

    //
    // ... Ctrl-R вставляет содержимое регистра, а Ctrl-R Ctrl-W - слово под курсором
    //
    if (m_commandLineAwaitingRegister) {
        m_commandLineAwaitingRegister = false;
        QString text;
        if (hasVimControl(_event) && _event->key() == Qt::Key_W) {
            text = Vim::wordUnderCursor(document(), currentPosition(), nullptr, nullptr);
        } else if (Vim::Registers::isValidName(commandCharacter(_event))) {
            text = registerValue(commandCharacter(_event)).text;
            while (text.endsWith('\n')) {
                text.chop(1);
            }
        }
        m_commandLine.append(text);
        return;
    }
    if (hasVimControl(_event) && _event->key() == Qt::Key_R) {
        m_commandLineAwaitingRegister = true;
        return;
    }

    if (_event->key() == Qt::Key_Backspace) {
        const int size = m_commandLine.size();
        const bool isSurrogatePair = size > 2 && m_commandLine.at(size - 1).isLowSurrogate()
            && m_commandLine.at(size - 2).isHighSurrogate();
        m_commandLine.chop(isSurrogatePair ? 2 : 1);
        if (m_commandLine.isEmpty()) {
            closeCommandLine();
        }
        return;
    }

    if (hasVimControl(_event) && _event->key() == Qt::Key_U) {
        m_commandLine = m_commandLine.left(1);
        return;
    }

    //
    // ... Ctrl-W удаляет слово, или знаки препинания перед курсором вместе с пробелами за ними
    //
    if (hasVimControl(_event) && _event->key() == Qt::Key_W) {
        auto isWordCharacter
            = [](QChar _character) { return _character.isLetterOrNumber() || _character == '_'; };
        int end = m_commandLine.size();
        while (end > 1 && m_commandLine.at(end - 1).isSpace()) {
            --end;
        }
        if (end > 1) {
            const bool isWord = isWordCharacter(m_commandLine.at(end - 1));
            while (end > 1 && !m_commandLine.at(end - 1).isSpace()
                   && isWordCharacter(m_commandLine.at(end - 1)) == isWord) {
                --end;
            }
        }
        m_commandLine.truncate(end);
        return;
    }

    const auto text = _event->text();
    if (!text.isEmpty() && QChar::isPrint(firstCodePoint(text))) {
        m_commandLine.append(text);
    }
}

void VimTextEditController::executeCommandLine(const QString& _command)
{
    if (_command.isEmpty()) {
        resetPendingCommand();
        return;
    }

    const auto prefix = _command.at(0);
    const auto body = _command.mid(1);
    if (prefix == '/' || prefix == '?') {
        //
        // ... после шаблона через разделитель может идти смещение: /шаблон/e+1
        //
        QString pattern;
        QString offset;
        bool hasOffset = false;
        for (int index = 0; index < body.size(); ++index) {
            const auto character = body.at(index);
            if (character == '\\' && index + 1 < body.size()) {
                pattern += character;
                pattern += body.at(++index);
                continue;
            }
            if (character == prefix) {
                offset = body.mid(index + 1);
                hasOffset = true;
                break;
            }
            pattern += character;
        }

        //
        // ... пустой шаблон означает повтор последнего поиска, а если не задано и смещение,
        //     то и с последним смещением
        //
        if (pattern.isEmpty()) {
            pattern = shared().searchPattern;
            if (!hasOffset) {
                offset = shared().searchOffset;
            }
        }
        shared().searchPattern = pattern;
        shared().searchOffset = offset;
        shared().searchForward = prefix == '/';
        const int count = takeCount();
        auto motion = searchMotion(pattern, prefix == '/', count, currentPosition(), offset);
        applyMotion(motion);
        return;
    }

    resetPendingCommand();
    if (prefix == ':') {
        shared().lastExCommand = body;
        executeExCommands(body);
    }
}

void VimTextEditController::executeExCommands(const QString& _commands)
{
    //
    // Команды разделяются чертой, если она не экранирована, но у :normal черта - часть аргумента,
    // а у замены она может быть в шаблоне, поэтому замена отделяет следующую команду сама
    //
    static const QRegularExpression wholeArgumentCommand(
        QStringLiteral("^[^A-Za-z&]*(norm|&|s(u(b(s(t(i(t(u(te?)?)?)?)?)?)?)?)?(?![A-Za-z]))"));
    QStringList commands;
    QString command;
    for (int index = 0; index < _commands.size(); ++index) {
        const auto character = _commands.at(index);
        if (character == '\\' && index + 1 < _commands.size()) {
            command += character;
            command += _commands.at(++index);
            continue;
        }
        if (character == '|' && !wholeArgumentCommand.match(command).hasMatch()) {
            commands.append(command);
            command.clear();
            continue;
        }
        command += character;
    }
    commands.append(command);

    for (const auto& item : commands) {
        executeExCommand(item);
        if (m_commandFailed) {
            break;
        }
    }
}

void VimTextEditController::executeExCommand(const QString& _command)
{
    auto document = this->document();
    const auto command = _command.trimmed();
    int index = 0;

    //
    // Разбираем диапазон строк: %, номера, ., $, метки, шаблоны и смещения
    //
    auto currentLine = Vim::navigableBlockNumber(currentBlock());
    const int lineCount = Vim::navigableBlocksCount(document);
    bool addressFailed = false;
    auto parseAddress = [&](int* _line) {
        bool found = false;
        int line = currentLine;
        if (index < command.size()) {
            const auto character = command.at(index);
            if (character.isDigit()) {
                int end = index;
                while (end < command.size() && command.at(end).isDigit()) {
                    ++end;
                }
                line = command.mid(index, end - index).toInt();
                index = end;
                found = true;
            } else if (character == '.') {
                ++index;
                found = true;
            } else if (character == '$') {
                line = lineCount;
                ++index;
                found = true;
            } else if (character == '\'' && index + 1 < command.size()) {
                const auto mark = command.at(index + 1);
                const auto name = mark == '\'' ? QChar('`') : mark;
                index += 2;
                if (!m_marks.contains(name) || m_marks.value(name).document() != document) {
                    fail(QCoreApplication::translate("Ui::VimTextEditController",
                                                     "E20: Mark not set"));
                    addressFailed = true;
                    return false;
                }
                line = Vim::navigableBlockNumber(
                    Vim::navigableBlock(document, m_marks.value(name).position()));
                found = true;
            } else if (character == '/' || character == '?') {
                //
                // ... строка, в которой найден шаблон, ищем со следующей строки, или с предыдущей
                //
                QString pattern;
                int end = index + 1;
                while (end < command.size() && command.at(end) != character) {
                    if (command.at(end) == '\\' && end + 1 < command.size()) {
                        pattern += command.at(end);
                        ++end;
                    }
                    pattern += command.at(end);
                    ++end;
                }
                index = end < command.size() ? end + 1 : end;
                if (pattern.isEmpty()) {
                    pattern = shared().searchPattern;
                } else {
                    shared().searchPattern = pattern;
                }
                const bool forward = character == '/';
                const auto block = Vim::navigableBlockByNumber(document, line);
                const int position = searchPosition(
                    pattern, forward ? Vim::lineEndPosition(block) : block.position(), forward);
                if (position < 0) {
                    if (m_message.isEmpty()) {
                        fail(QCoreApplication::translate("Ui::VimTextEditController",
                                                         "E486: Pattern not found: %1")
                                 .arg(pattern));
                    }
                    addressFailed = true;
                    return false;
                }
                line = Vim::navigableBlockNumber(Vim::navigableBlock(document, position));
                found = true;
            }
        }
        while (index < command.size() && (command.at(index) == '+' || command.at(index) == '-')) {
            const int sign = command.at(index) == '+' ? 1 : -1;
            ++index;
            int end = index;
            while (end < command.size() && command.at(end).isDigit()) {
                ++end;
            }
            line += sign * (end > index ? command.mid(index, end - index).toInt() : 1);
            index = end;
            found = true;
        }
        if (found) {
            *_line = line;
        }
        return found;
    };

    int firstLine = currentLine;
    int lastLine = currentLine;
    int addressCount = 0;
    if (command.startsWith('%')) {
        firstLine = 1;
        lastLine = lineCount;
        index = 1;
        addressCount = 2;
    } else {
        int line = currentLine;
        if (parseAddress(&line)) {
            firstLine = lastLine = line;
            addressCount = 1;
        }
        if (!addressFailed && index < command.size()
            && (command.at(index) == ',' || command.at(index) == ';')) {
            if (command.at(index) == ';') {
                currentLine = firstLine;
            }
            ++index;
            if (parseAddress(&line)) {
                lastLine = line;
            }
            addressCount = 2;
        }
    }
    const bool hasRange = addressCount > 0;
    if (addressFailed) {
        return;
    }
    if (firstLine > lastLine) {
        std::swap(firstLine, lastLine);
    }

    const auto rest = command.mid(index).trimmed();

    //
    // ... переход к строке по её номеру, номера за пределами документа ограничиваются им
    //
    if (rest.isEmpty()) {
        if (hasRange && lastLine < 0) {
            fail(QCoreApplication::translate("Ui::VimTextEditController", "E16: Invalid range"));
            return;
        }
        if (hasRange) {
            rememberJumpPosition();
            setCursorPosition(Vim::firstNonBlankPosition(
                Vim::navigableBlockByNumber(document, qBound(1, lastLine, lineCount))));
        }
        return;
    }

    //
    // ... а для команд диапазон должен быть внутри документа, нулевая строка означает первую
    //
    if (firstLine < 0 || lastLine > lineCount) {
        fail(QCoreApplication::translate("Ui::VimTextEditController", "E16: Invalid range"));
        return;
    }
    firstLine = qMax(1, firstLine);
    lastLine = qMax(1, lastLine);
    auto firstBlock = Vim::navigableBlockByNumber(document, firstLine);
    auto lastBlock = Vim::navigableBlockByNumber(document, lastLine);

    //
    // ... замена текста
    //
    if (rest.startsWith('&')) {
        substitute(firstBlock, lastBlock, rest);
        return;
    }
    if (rest.startsWith('s')) {
        //
        // ... команда может быть сокращена до любого начала слова substitute
        //
        const QString substituteCommand = QStringLiteral("substitute");
        int length = 1;
        while (length < rest.size() && length < substituteCommand.size()
               && rest.at(length) == substituteCommand.at(length)) {
            ++length;
        }
        const auto arguments = rest.mid(length);
        if (arguments.isEmpty() || !arguments.at(0).isLetter()) {
            substitute(firstBlock, lastBlock, arguments);
            return;
        }
    }

    //
    // ... остальные команды: имя, восклицательный знак и аргументы
    //
    int nameEnd = 0;
    while (nameEnd < rest.size() && rest.at(nameEnd).isLetter()) {
        ++nameEnd;
    }
    const auto name = rest.left(nameEnd);
    const bool bang = nameEnd < rest.size() && rest.at(nameEnd) == '!';
    auto arguments = rest.mid(nameEnd + (bang ? 1 : 0)).trimmed();
    auto isCommand = [&name](const QString& _fullName, int _minLength) {
        return name.size() >= _minLength && _fullName.startsWith(name);
    };

    //
    // ... регистр и количество строк, которое отсчитывается от конца диапазона
    //
    auto takeRegisterAndCount = [&](bool _withRegister, QChar* _register) {
        if (_withRegister && !arguments.isEmpty() && !arguments.at(0).isDigit()) {
            *_register = arguments.at(0);
            arguments = arguments.mid(1).trimmed();
        }
        if (!arguments.isEmpty() && arguments.at(0).isDigit()) {
            int end = 0;
            while (end < arguments.size() && arguments.at(end).isDigit()) {
                ++end;
            }
            const int count = arguments.left(end).toInt();
            arguments = arguments.mid(end).trimmed();
            if (count <= 0) {
                fail(QCoreApplication::translate("Ui::VimTextEditController",
                                                 "E939: Positive count required"));
                return false;
            }
            firstLine = lastLine;
            lastLine = qMin(lineCount, lastLine + count - 1);
            firstBlock = Vim::navigableBlockByNumber(document, firstLine);
            lastBlock = Vim::navigableBlockByNumber(document, lastLine);
        }
        if (!arguments.isEmpty()) {
            fail(QCoreApplication::translate("Ui::VimTextEditController",
                                             "E488: Trailing characters: %1")
                     .arg(arguments));
            return false;
        }
        return true;
    };

    if (isCommand(QStringLiteral("delete"), 1)) {
        QChar registerName;
        if (takeRegisterAndCount(true, &registerName)) {
            deleteRange(linesRange(firstBlock.position(), lastBlock.position()), registerName,
                        true);
        }
    } else if (isCommand(QStringLiteral("yank"), 1)) {
        QChar registerName;
        if (takeRegisterAndCount(true, &registerName)) {
            yankRange(linesRange(firstBlock.position(), lastBlock.position()), registerName,
                      currentPosition());
        }
    } else if (isCommand(QStringLiteral("join"), 1)) {
        QChar registerName;
        if (!arguments.isEmpty() && arguments.at(0).isDigit()) {
            ++addressCount;
        }
        if (!takeRegisterAndCount(false, &registerName)) {
            return;
        }
        //
        // ... как и в vim, без диапазона объединяются текущая и следующая строки, а диапазон
        //     из одной строки ничего не объединяет
        //
        if (firstLine == lastLine) {
            if (addressCount >= 2) {
                return;
            }
            if (lastLine == lineCount) {
                fail();
                return;
            }
            ++lastLine;
        }
        setCursorPosition(firstBlock.position());
        joinLines(lastLine - firstLine + 1, !bang);
        setCursorPosition(Vim::firstNonBlankPosition(currentBlock()));
    } else if (isCommand(QStringLiteral("normal"), 4)) {
        executeNormalCommand(firstLine, hasRange ? lastLine : firstLine, arguments);
    } else if (isCommand(QStringLiteral("nohlsearch"), 3)) {
        //
        // ... найденный текст не подсвечивается, так что и убирать нечего
        //
    } else {
        fail(QCoreApplication::translate("Ui::VimTextEditController",
                                         "E492: Not an editor command: %1")
                 .arg(_command.trimmed()));
    }
}

void VimTextEditController::executeNormalCommand(int _firstLine, int _lastLine,
                                                 const QString& _keys)
{
    if (_keys.isEmpty() || m_replayDepth >= kMaxReplayDepth) {
        return;
    }

    //
    // Набираем команду, как будто её ввели с клавиатуры, отдельно для каждой строки диапазона
    //
    QVector<RecordedKey> keys;
    for (int index = 0; index < _keys.size(); ++index) {
        auto text = QString(_keys.at(index));
        if (_keys.at(index).isHighSurrogate() && index + 1 < _keys.size()) {
            text += _keys.at(++index);
        }
        const auto modifiers = text.at(0).isUpper() ? Qt::ShiftModifier : Qt::NoModifier;
        keys.append({ keyForCharacter(text.at(0)), modifiers, text });
    }

    //
    // ... строки перебираем по номерам, как и vim, ведь команды могут добавлять и удалять строки
    //
    for (int lineNumber = _firstLine; lineNumber <= _lastLine; ++lineNumber) {
        //
        // ... если строк стало меньше, то, как и vim, остаёмся на последней
        //
        auto block = Vim::firstNavigableBlock(document());
        for (int number = 1; number < lineNumber; ++number) {
            const auto next = Vim::nextNavigableBlock(block);
            if (!next.isValid()) {
                break;
            }
            block = next;
        }
        if (!block.isValid()) {
            break;
        }

        m_commandFailed = false;
        setCursorPosition(block.position());
        replayKeys(keys);

        //
        // ... незавершённая команда отменяется, как будто нажали Esc
        //
        if (m_mode == Mode::Insert) {
            leaveInsertMode();
        } else if (isVisualMode()) {
            enterNormalMode();
        }
        closeCommandLine();
        resetPendingCommand();
    }
}

void VimTextEditController::substitute(QTextBlock _firstLine, QTextBlock _lastLine,
                                       const QString& _arguments)
{
    if (!canEdit()) {
        return;
    }

    auto& state = shared();
    QString pattern;
    QString replacement;
    QString flags;
    QString nextCommand;

    //
    // Без аргументов повторяем последнюю замену, а с & - ещё и с прежними флагами
    //
    if (_arguments.isEmpty() || _arguments.startsWith('&')) {
        if (state.substitutePattern.isEmpty()) {
            return;
        }
        pattern = state.substitutePattern;
        replacement = state.substituteReplacement;
        flags = _arguments.startsWith(QStringLiteral("&&")) ? state.substituteFlags : QString();
    } else {
        const auto separator = _arguments.at(0);
        if (separator.isLetterOrNumber() || separator == '\\' || separator == '"'
            || separator == '|') {
            return;
        }

        //
        // ... разбираем шаблон, замену и флаги, учитывая экранированный разделитель
        //
        QStringList parts;
        QString part;
        for (int index = 1; index < _arguments.size(); ++index) {
            const auto character = _arguments.at(index);
            if (character == '\\' && index + 1 < _arguments.size()) {
                const auto next = _arguments.at(index + 1);
                if (next == separator && parts.size() < 2) {
                    part += next;
                } else {
                    part += character;
                    part += next;
                }
                ++index;
                continue;
            }
            if (character == separator && parts.size() < 2) {
                parts.append(part);
                part.clear();
                continue;
            }
            part += character;
        }
        parts.append(part);

        pattern = parts.value(0);
        replacement = parts.value(1);
        flags = parts.value(2).trimmed();

        //
        // ... ~ в замене означает предыдущую замену
        //
        QString expandedReplacement;
        for (int index = 0; index < replacement.size(); ++index) {
            const auto character = replacement.at(index);
            if (character == '\\' && index + 1 < replacement.size()) {
                expandedReplacement += character;
                expandedReplacement += replacement.at(++index);
            } else if (character == '~') {
                expandedReplacement += state.substituteReplacement;
            } else {
                expandedReplacement += character;
            }
        }
        replacement = expandedReplacement;

        //
        // ... за флагами через черту может идти следующая команда
        //
        const int nextCommandStart = flags.indexOf('|');
        if (nextCommandStart >= 0) {
            nextCommand = flags.mid(nextCommandStart + 1);
            flags = flags.left(nextCommandStart).trimmed();
        }
        if (flags.startsWith('&')) {
            flags = state.substituteFlags + flags.mid(1);
        }

        //
        // ... за флагами может идти количество строк, отсчитываемое от конца диапазона
        //
        int flagsEnd = 0;
        while (flagsEnd < flags.size() && !flags.at(flagsEnd).isDigit()) {
            ++flagsEnd;
        }
        if (flagsEnd < flags.size()) {
            const int count = flags.mid(flagsEnd).trimmed().toInt();
            flags = flags.left(flagsEnd).trimmed();
            if (count <= 0) {
                fail(QCoreApplication::translate("Ui::VimTextEditController",
                                                 "E939: Positive count required"));
                return;
            }
            _firstLine = _lastLine;
            for (int line = 1; line < count; ++line) {
                const auto next = Vim::nextNavigableBlock(_lastLine);
                if (!next.isValid()) {
                    break;
                }
                _lastLine = next;
            }
        }

        //
        // ... пустой шаблон - последний поиск
        //
        if (pattern.isEmpty()) {
            pattern = state.searchPattern;
        }
        if (pattern.isEmpty()) {
            return;
        }
        state.substitutePattern = pattern;
        state.substituteReplacement = replacement;
        state.substituteFlags = flags;
        state.searchPattern = pattern;
    }

    auto expression = Vim::regularExpression(pattern);
    if (flags.contains('i')) {
        expression.setPatternOptions(expression.patternOptions()
                                     | QRegularExpression::CaseInsensitiveOption);
    } else if (flags.contains('I')) {
        expression.setPatternOptions(expression.patternOptions()
                                     & ~QRegularExpression::CaseInsensitiveOption);
    }
    const bool replaceAll = flags.contains('g');

    //
    // Сперва находим все совпадения, а затем заменяем их с конца. Обычно все замены делаются
    // одним изменением документа, во время которого он не перестраивается, и позиции ещё не
    // заменённых совпадений не сдвигаются, но в режиме отслеживания изменений удалением
    // занимается сам редактор, который может перестроить разрывы страниц, поэтому тогда
    // отслеживаем позиции курсорами
    // @note Курсоры используем только тогда, т.к. каждый из них обновляется при любой правке
    //       документа, а совпадений могут быть тысячи
    //
    const bool isTrackChanges = m_editor->isTrackChangesEnabled();
    struct Replacement {
        int from = 0;
        int to = 0;
        QTextCursor range;
        QString text;
    };
    QVector<Replacement> replacements;
    int changedLines = 0;
    QElapsedTimer timer;
    timer.start();
    for (auto block = _firstLine; block.isValid(); block = Vim::nextNavigableBlock(block)) {
        if (timer.elapsed() > kSearchTimeoutMs) {
            fail(QCoreApplication::translate("Ui::VimTextEditController", "Interrupted"));
            return;
        }

        const auto text = Vim::displayedText(block);
        const auto options = matchOptions(text);
        int column = 0;
        int previousMatchEnd = -1;
        bool isFirst = true;
        bool isChanged = false;
        forever
        {
            //
            // ... как и в vim, после совпадения, дошедшего до конца строки, поиск прекращается
            //
            if (!isFirst && column >= text.size()) {
                break;
            }
            const auto match
                = expression.match(text, column, QRegularExpression::NormalMatch, options);
            if (!match.hasMatch()) {
                break;
            }
            const int matchStart = static_cast<int>(match.capturedStart());
            const int matchEnd = static_cast<int>(match.capturedEnd());

            //
            // ... пустое совпадение сразу за предыдущим не считается
            //
            if (matchStart == matchEnd && matchStart == previousMatchEnd) {
                if (matchStart >= text.size()) {
                    break;
                }
                column = Vim::nextCharacterPosition(block, block.position() + matchStart)
                    - block.position();
                continue;
            }

            Replacement item;
            item.from = block.position() + matchStart;
            item.to = block.position() + matchEnd;
            item.text = Vim::substitution(match, replacement);
            if (isTrackChanges) {
                //
                // ... конец совпадения не должен сдвигаться заменой следующего совпадения,
                //     которая начинается сразу за ним
                //
                item.range = QTextCursor(document());
                item.range.setPosition(item.from);
                item.range.setPosition(item.to, QTextCursor::KeepAnchor);
                item.range.setKeepPositionOnInsert(true);
            }
            replacements.append(item);
            isChanged = true;
            isFirst = false;
            previousMatchEnd = matchEnd;
            if (!replaceAll) {
                break;
            }
            column = matchStart == matchEnd
                ? Vim::nextCharacterPosition(block, block.position() + matchEnd) - block.position()
                : matchEnd;
        }
        if (isChanged) {
            ++changedLines;
        }

        if (block == _lastLine) {
            break;
        }
    }

    if (replacements.isEmpty()) {
        fail(QCoreApplication::translate("Ui::VimTextEditController", "E486: Pattern not found: %1")
                 .arg(pattern));
        return;
    }

    //
    // ... с флагом n совпадения только подсчитываются, поэтому текст не трогаем
    //
    if (flags.contains('n')) {
        m_message
            = QCoreApplication::translate("Ui::VimTextEditController", "Matches: %1, lines: %2")
                  .arg(replacements.size())
                  .arg(changedLines);
        if (!nextCommand.isEmpty()) {
            executeExCommands(nextCommand);
        }
        return;
    }

    //
    // ... все замены делаем одним изменением документа, чтобы модель обновилась один раз
    //
    QTextCursor editCursor(document());
    if (!isTrackChanges) {
        editCursor.beginEditBlock();
    }
    QTextCursor lastChangedLine;
    for (int index = replacements.size() - 1; index >= 0; --index) {
        const auto& item = replacements.at(index);
        const int from = isTrackChanges ? item.range.selectionStart() : item.from;
        const int to = isTrackChanges ? item.range.selectionEnd() : item.to;
        replaceText(from, to, item.text);

        //
        // ... запоминаем конец последней замены, ведь замена с переводами строк делит строку
        //
        if (lastChangedLine.isNull()) {
            lastChangedLine = QTextCursor(document());
            lastChangedLine.setPosition(
                qMin(from + static_cast<int>(item.text.size()), document()->characterCount() - 1));
        }
    }
    if (!isTrackChanges) {
        editCursor.endEditBlock();
    }

    //
    // ... курсор встаёт в начало последней изменённой строки
    //
    setCursorPosition(
        Vim::firstNonBlankPosition(Vim::navigableBlock(document(), lastChangedLine.position())));
    if (changedLines > 2) {
        m_message = QCoreApplication::translate("Ui::VimTextEditController",
                                                "Substitutions: %1, lines: %2")
                        .arg(replacements.size())
                        .arg(changedLines);
    }
    if (!nextCommand.isEmpty() && !m_commandFailed) {
        executeExCommands(nextCommand);
    }
}


// ****


void VimTextEditController::trackKeyPress(const RecordedKey& _key)
{
    //
    // ... записываем макрос
    //
    if (!m_recordingRegister.isNull()) {
        if (m_skipMacroRecordKey) {
            m_skipMacroRecordKey = false;
        } else if (m_recordedKeys.size() < kMaxRecordedKeys) {
            m_recordedKeys.append(_key);
        }
    }

    //
    // ... и собираем команду, которую можно будет повторить, причём текст, вставленный из
    //     регистра по Ctrl-R, запоминается как набранный
    //
    if (!m_insertedRegisterText.isEmpty()) {
        if (!m_commandKeys.isEmpty()) {
            m_commandKeys.removeLast();
        }
        if (!m_insertKeys.isEmpty()) {
            m_insertKeys.removeLast();
            if (!m_insertKeys.isEmpty()) {
                m_insertKeys.removeLast();
            }
            m_insertKeys += keysFromText(m_insertedRegisterText);
        }
        m_commandKeys += keysFromText(m_insertedRegisterText);
        m_insertedRegisterText.clear();
    } else if (m_commandKeys.size() < kMaxRecordedKeys) {
        m_commandKeys.append(_key);
    } else {
        m_commandKeysOverflow = true;
    }

    const bool isCommandFinished = m_mode == Mode::Normal && !hasPendingInput();
    if (!isCommandFinished) {
        return;
    }

    const bool isChanged = document()->revision() != m_commandRevision;
    if (isChanged) {
        //
        // ... слишком длинную команду целиком не повторить, поэтому лучше не повторять ничего
        //
        if (m_commandIsChange && m_commandKeysOverflow) {
            m_lastChangeKeys.clear();
        } else if (m_commandIsChange) {
            m_lastChangeKeys = m_commandKeys;

            //
            // ... вставка, прерванная по Ctrl-O, повторяется так, будто её завершили по Esc
            //
            if (m_insertOneCommand && !m_lastChangeKeys.isEmpty()) {
                m_lastChangeKeys.last()
                    = { Qt::Key_Escape, Qt::NoModifier, QStringLiteral("\x1b") };
            }
        }
        setMark('.', currentPosition());
        if (m_isAutoRepeat) {
            m_hasUncommittedChange = true;
        } else {
            commitUndoStep();
        }
    }
    m_commandKeys.clear();
    m_commandKeysOverflow = false;
    m_commandRevision = document()->revision();
    m_commandIsChange = true;
}

void VimTextEditController::toggleMacroRecording(QChar _name)
{
    m_commandIsChange = false;

    if (!m_recordingRegister.isNull()) {
        const auto name = m_recordingRegister.toLower();
        shared().macros[name] = m_recordedKeys;
        shared().registers.setRecordedContent(name, keysText(m_recordedKeys));
        m_recordedKeys.clear();
        m_recordingRegister = QChar();
        return;
    }

    //
    // ... заглавная буква дописывает нажатия к уже записанному макросу
    //
    m_recordingRegister = _name;
    m_recordedKeys = _name.isUpper() ? macroKeys(_name.toLower()) : QVector<RecordedKey>();
    m_skipMacroRecordKey = true;
}

QString VimTextEditController::keysText(const QVector<RecordedKey>& _keys)
{
    QString text;
    for (const auto& key : _keys) {
        if (!key.text.isEmpty()) {
            text += key.text;
            continue;
        }

        //
        // ... клавиши, которые не вводят текст, записываем символами из области для личного
        //     использования, как это делает macOS
        //
        switch (key.key) {
        case Qt::Key_Up: {
            text += QChar(0xF700);
            break;
        }
        case Qt::Key_Down: {
            text += QChar(0xF701);
            break;
        }
        case Qt::Key_Left: {
            text += QChar(0xF702);
            break;
        }
        case Qt::Key_Right: {
            text += QChar(0xF703);
            break;
        }
        case Qt::Key_Home: {
            text += QChar(0xF729);
            break;
        }
        case Qt::Key_End: {
            text += QChar(0xF72B);
            break;
        }
        case Qt::Key_PageUp: {
            text += QChar(0xF72C);
            break;
        }
        case Qt::Key_PageDown: {
            text += QChar(0xF72D);
            break;
        }
        default: {
            break;
        }
        }
    }
    return text;
}

QVector<VimTextEditController::RecordedKey> VimTextEditController::keysFromText(
    const QString& _text)
{
    QVector<RecordedKey> keys;
    for (int index = 0; index < _text.size(); ++index) {
        const auto character = _text.at(index);
        const auto code = character.unicode();
        switch (code) {
        case 0x1b: {
            keys.append({ Qt::Key_Escape, Qt::NoModifier, QString(character) });
            break;
        }
        case '\r':
        case '\n': {
            keys.append({ Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r") });
            break;
        }
        case '\t': {
            keys.append({ Qt::Key_Tab, Qt::NoModifier, QString(character) });
            break;
        }
        case '\b': {
            keys.append({ Qt::Key_Backspace, Qt::NoModifier, QString(character) });
            break;
        }
        case 0x7f: {
            keys.append({ Qt::Key_Delete, Qt::NoModifier, QString(character) });
            break;
        }
        case 0xF700:
        case 0xF701:
        case 0xF702:
        case 0xF703:
        case 0xF729:
        case 0xF72B:
        case 0xF72C:
        case 0xF72D: {
            const QHash<ushort, int> functionKeys = {
                { 0xF700, Qt::Key_Up },     { 0xF701, Qt::Key_Down },     { 0xF702, Qt::Key_Left },
                { 0xF703, Qt::Key_Right },  { 0xF729, Qt::Key_Home },     { 0xF72B, Qt::Key_End },
                { 0xF72C, Qt::Key_PageUp }, { 0xF72D, Qt::Key_PageDown },
            };
            keys.append({ functionKeys.value(code), Qt::NoModifier, {} });
            break;
        }
        default: {
            //
            // ... управляющие символы - это сочетания с Ctrl
            //
            if (code >= 0x01 && code <= 0x1a) {
                keys.append({ Qt::Key_A + code - 1, vimControlModifier(), QString(character) });
                break;
            }
            auto text = QString(character);
            if (character.isHighSurrogate() && index + 1 < _text.size()) {
                text += _text.at(++index);
            }
            keys.append({ keyForCharacter(character),
                          character.isUpper() ? Qt::ShiftModifier : Qt::NoModifier, text });
            break;
        }
        }
    }
    return keys;
}

QVector<VimTextEditController::RecordedKey> VimTextEditController::macroKeys(QChar _name) const
{
    //
    // ... если текст в регистре не менялся, то воспроизводим записанные нажатия как есть
    //
    const auto content = registerValue(_name);
    const auto recordedKeys = shared().macros.value(_name.toLower());
    if (!recordedKeys.isEmpty() && keysText(recordedKeys) == content.text) {
        return recordedKeys;
    }
    return keysFromText(content.text);
}

void VimTextEditController::playMacro(QChar _name, int _count)
{
    const auto name = _name.toLower();
    const auto keys = macroKeys(name);
    if (keys.isEmpty()) {
        return;
    }

    shared().lastPlayedMacro = name;
    for (int step = 0; step < _count && !m_commandFailed; ++step) {
        replayKeys(keys);
    }
}

void VimTextEditController::repeatLastChange(int _count)
{
    m_commandIsChange = false;
    if (m_lastChangeKeys.isEmpty()) {
        return;
    }

    //
    // Повтор вставки из нумерованного регистра берёт следующий регистр, как в vim: "1p...
    //
    for (int index = 0; index + 2 < m_lastChangeKeys.size(); ++index) {
        const auto& key = m_lastChangeKeys.at(index);
        if (key.text.size() == 1 && key.text.at(0).isDigit()) {
            continue;
        }
        if (key.text == '"') {
            auto& registerKey = m_lastChangeKeys[index + 1];
            const auto command = m_lastChangeKeys.at(index + 2).text;
            const auto name = registerKey.text.size() == 1 ? registerKey.text.at(0) : QChar();
            if (name >= '1' && name <= '8'
                && (command == 'p' || command == 'P' || command == 'g')) {
                const QChar nextName(name.unicode() + 1);
                registerKey = { keyForCharacter(nextName), Qt::NoModifier, QString(nextName) };
            }
        }
        break;
    }

    //
    // Количество в команде повтора заменяет количество в самой повторяемой команде
    //
    auto keys = m_lastChangeKeys;
    if (_count > 0) {
        auto character = [&keys](int _index) {
            const auto& text = keys.at(_index).text;
            return text.size() == 1 ? text.at(0) : QChar();
        };
        auto isCountStart = [&character](int _index) {
            const auto value = character(_index);
            return value >= '1' && value <= '9';
        };
        auto skipCount = [&keys, &character, &isCountStart](int _index) {
            if (_index >= keys.size() || !isCountStart(_index)) {
                return _index;
            }
            while (_index < keys.size() && character(_index).isDigit()) {
                ++_index;
            }
            return _index;
        };

        QVector<RecordedKey> result;
        int index = 0;
        forever
        {
            if (index + 1 < keys.size() && character(index) == '"') {
                result.append(keys.at(index));
                result.append(keys.at(index + 1));
                index += 2;
                continue;
            }
            const int afterCount = skipCount(index);
            if (afterCount != index) {
                index = afterCount;
                continue;
            }
            break;
        }

        if (index < keys.size() && character(index) != 'v' && character(index) != 'V') {
            const auto command = character(index);
            result.append(keys.at(index));
            ++index;
            if (command == 'g' && index < keys.size()) {
                result.append(keys.at(index));
                ++index;
            }
            const bool isOperator = command == 'd' || command == 'c' || command == 'y'
                || (command == 'g' && QStringLiteral("uU~").contains(character(index - 1)));
            if (isOperator) {
                index = skipCount(index);
            }
            const auto countText = QString::number(_count);
            for (int digit = countText.size() - 1; digit >= 0; --digit) {
                result.prepend({ keyForCharacter(countText.at(digit)), Qt::NoModifier,
                                 QString(countText.at(digit)) });
            }
            for (; index < keys.size(); ++index) {
                result.append(keys.at(index));
            }
            keys = result;
        }
    }

    QScopedValueRollback<bool> repeatRollback(m_repeatingChange, true);
    replayKeys(keys);
}

void VimTextEditController::replayKeys(const QVector<RecordedKey>& _keys)
{
    if (_keys.isEmpty() || m_replayDepth >= kMaxReplayDepth) {
        return;
    }

    ++m_replayDepth;
    for (const auto& key : _keys) {
        //
        // ... как и в vim, ошибка в команде прерывает воспроизведение
        //
        //
        // ... но при повторе изменения невыполненное перемещение, которым строилось выделение,
        //     не страшно, ведь vim повторяет сам оператор над тем же объёмом текста
        //
        if (m_commandFailed && m_repeatingChange && isVisualMode()) {
            m_commandFailed = false;
        }
        if (m_replayedKeys >= kMaxReplayedKeys) {
            fail(QCoreApplication::translate("Ui::VimTextEditController", "Interrupted"));
            break;
        }
        if (m_commandFailed) {
            break;
        }
        ++m_replayedKeys;
        QKeyEvent event(QEvent::KeyPress, key.key, key.modifiers, key.text);
        QGuiApplication::sendEvent(m_editor, &event);
    }
    --m_replayDepth;
}

void VimTextEditController::finishHandledEvent(QKeyEvent* _event)
{
    //
    // ... подсказки редактора нужны только при наборе текста
    //
    if (!isInsertMode() && m_editor->isCompleterVisible()) {
        m_editor->closeCompleter();
    }
    m_editor->ensureCursorVisible();
    _event->accept();
}

void VimTextEditController::fail(const QString& _message)
{
    m_commandFailed = true;
    if (!_message.isEmpty()) {
        m_message = _message;
    }
}

} // namespace Ui
