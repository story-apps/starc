#include "vim_text_edit_controller.h"

#include "script_text_edit.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_document.h>
#include <business_layer/model/text/text_model.h>
#include <business_layer/templates/text_template.h>

#include <QGuiApplication>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextDocument>

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
const int kMaxRecordedKeys = 10000;

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
    return _event->modifiers().testFlag(vimControlModifier());
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
 * @brief Вводит ли нажатие символ, с учётом раскладок, в которых символы набираются с Alt
 */
bool isCharacterInput(QKeyEvent* _event)
{
    const auto text = _event->text();
    if (text.isEmpty() || !text.at(0).isPrint()) {
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
    bool searchForward = true;

    QChar findCharacter;
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
    case Mode::Normal:
        return QStringLiteral("NORMAL");
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

    if (!m_recordingRegister.isNull()) {
        return QStringLiteral("recording @%1").arg(m_recordingRegister.toLower());
    }

    return {};
}

bool VimTextEditController::usesBlockCursor() const
{
    return m_enabled && m_mode != Mode::Insert;
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

    if (m_mode == Mode::Insert || !hasVimControl(_event)) {
        return false;
    }

    return _event->key() == Qt::Key_R || _event->key() == Qt::Key_D || _event->key() == Qt::Key_U;
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
        if (m_mode == Mode::Normal && !hasPendingInput()) {
            m_commandKeys.clear();
            m_commandRevision = document()->revision();
            m_commandIsChange = true;
        } else if (isVisualMode() && m_commandKeys.isEmpty()) {
            //
            // ... выделение мышью не повторить, поэтому и команду над ним тоже
            //
            m_commandIsChange = false;
        }
    }

    const bool handled = handleKeyPressImpl(_event);

    if (isTopLevel) {
        trackKeyPress(recordedKey);
    }
    return handled;
}

bool VimTextEditController::handleInputMethodEvent(QInputMethodEvent* _event)
{
    if (!m_enabled || m_passThroughDepth > 0 || m_mode == Mode::Insert || _event == nullptr) {
        return false;
    }

    //
    // В командных режимах символы, набранные через метод ввода (например с мёртвыми клавишами),
    // обрабатываем так же, как обычные нажатия
    //
    const auto commitString = _event->commitString();
    for (const auto& character : commitString) {
        QKeyEvent keyEvent(QEvent::KeyPress, keyForCharacter(character), Qt::NoModifier,
                           QString(character));
        handleKeyPress(&keyEvent);
    }
    _event->accept();
    return true;
}

void VimTextEditController::handleSelectionChange()
{
    if (!m_enabled || m_editingDepth > 0 || m_passThroughDepth > 0 || m_mode == Mode::Insert) {
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

void VimTextEditController::enterInsertMode(int _repeat, QChar _command)
{
    m_autoIndentLine = QTextCursor();
    if (isVisualMode()) {
        leaveVisualMode();
    }
    resetPendingCommand();
    m_mode = Mode::Insert;
    m_insertRepeat = qBound(1, _repeat, kMaxCount);
    m_insertCommand = _command;
    m_insertKeys.clear();
    m_preferredX = -1;
}

void VimTextEditController::leaveInsertMode()
{
    //
    // Убираем отступ, оставшийся от изменения строк, если ничего не было введено
    //
    if (!m_autoIndentLine.isNull() && m_autoIndentRevision == document()->revision()) {
        const auto block = m_autoIndentLine.block();
        if (!block.text().isEmpty() && block.text().trimmed().isEmpty()) {
            QTextCursor cursor(document());
            cursor.setPosition(block.position());
            cursor.setPosition(Vim::lineEndPosition(block), QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            m_editor->setTextCursor(cursor);
        }
    }
    m_autoIndentLine = QTextCursor();

    //
    // Повторяем набранный текст, если команда вставки была с количеством
    //
    if (m_insertRepeat > 1 && !m_insertKeys.isEmpty() && canEdit()) {
        const auto keys = m_insertKeys;
        QScopedValueRollback<bool> replayRollback(m_replayingInsert, true);
        for (int index = 1; index < m_insertRepeat; ++index) {
            if (m_insertCommand == 'o' || m_insertCommand == 'O') {
                openLine(true);
            }
            replayKeys(keys);
        }
    }
    m_insertRepeat = 1;
    m_insertCommand = QChar();
    m_insertKeys.clear();

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
}

void VimTextEditController::enterNormalMode()
{
    if (isVisualMode()) {
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
    //
    // ... запоминаем выделение, чтобы его можно было восстановить командой gv
    //
    if (m_visualAnchor >= 0 && m_visualCursor >= 0) {
        m_lastVisualAnchor = QTextCursor(document());
        m_lastVisualAnchor.setPosition(m_visualAnchor);
        m_lastVisualCursor = QTextCursor(document());
        m_lastVisualCursor.setPosition(m_visualCursor);
        m_lastVisualMode = isVisualMode() ? m_mode : m_lastVisualMode;
        setMark('<', qMin(m_visualAnchor, m_visualCursor));
        setMark('>', qMax(m_visualAnchor, m_visualCursor));
    }

    m_visualAnchor = -1;
    m_visualCursor = -1;
    m_mode = Mode::Normal;
    resetPendingCommand();

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

void VimTextEditController::updateVisualSelection()
{
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
        end = next.isValid() && next.isVisible() ? next.position() : last.position();
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
        } else if (m_mode == Mode::Insert) {
            if (m_editor->isCompleterVisible()) {
                m_editor->closeCompleter();
            } else {
                leaveInsertMode();
            }
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

    if (m_mode == Mode::Insert) {
        //
        // ... запоминаем набранное, чтобы повторить его для команд вставки с количеством
        //
        if (!m_replayingInsert && m_insertRepeat > 1 && m_insertKeys.size() < kMaxRecordedKeys) {
            m_insertKeys.append({ _event->key(), _event->modifiers(), _event->text() });
        }
        return false;
    }

    if (hasVimControl(_event)) {
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
        //
        // Служебные клавиши, вроде листания страниц, отдаём на откуп самому редактору,
        // а всё, что вводит текст, поглощаем, чтобы не портить документ
        //
        resetPendingCommand();
        return !_event->text().isEmpty();
    }

    switch (character.unicode()) {
    case 'i':
    case 'a':
    case 'I':
    case 'A': {
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
            setCursorPosition(qMin(currentPosition() + 1, Vim::lineEndPosition(block)));
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
        openLine(character == 'o');
        enterInsertMode(count, character);
        return true;
    }

    case 'v':
    case 'V': {
        const int position = currentPosition();
        enterVisualMode(character == 'V' ? Mode::VisualLine : Mode::Visual, position, position);
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
        const int end = qMin(Vim::lineEndPosition(currentBlock()), position + count);
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
                    return true;
                }
                break;
            }
            lastBlock = next;
        }
        Motion motion;
        motion.position = lastBlock == currentBlock() ? currentPosition() : lastBlock.position();
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
        if (character.isNull()) {
            resetPendingCommand();
            return true;
        }
        shared().findCharacter = character;
        shared().findCommand = m_findCommand;
        const int count = takeCount();
        applyMotion(findCharacterMotion(character, m_findCommand == 'f' || m_findCommand == 't',
                                        m_findCommand == 't' || m_findCommand == 'T', count,
                                        false));
        return true;
    }

    case Awaiting::ReplaceCharacter: {
        const bool isLineBreak = isEnter(_event);
        if (character.isNull() && !isLineBreak) {
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
                    replaceText(lineRange.start, lineRange.end,
                                QString(lineRange.end - lineRange.start, character));
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
            const int lineEnd = Vim::lineEndPosition(currentBlock());
            if (!canEdit() || position + count > lineEnd) {
                return true;
            }
            removeText(position, position + count);
            QTextCursor cursor(document());
            cursor.setPosition(position);
            m_editor->setTextCursor(cursor);
            sendKeyPressToEditor(Qt::Key_Return, QStringLiteral("\r"));
            setCursorPosition(characterPosition(m_editor->textCursor().position()));
            return true;
        }
        replaceCharacters(character, count);
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
        const auto name = character == '@' ? shared().lastPlayedMacro : character;
        if (!name.isNull()) {
            playMacro(name, count);
        }
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
            Vim::Cursor cursor(document(), currentPosition());
            if (Vim::backwardEndOfWord(cursor, count, character == 'E', false)) {
                motion.position = cursor.position();
            }
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
        for (int count = _count; count > 0; --count) {
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
        for (int count = _count; count > 0; --count) {
            const bool canMove
                = pastLine ? !cursor.atLineEnd() : cursor.column() + 1 < cursor.lineLength();
            if (canMove) {
                cursor.setColumn(cursor.column() + 1);
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
        motion.position = isVisualMode() || lineEnd == block.position() ? lineEnd : lineEnd - 1;
        motion.inclusive = true;
        motion.toLineEnd = true;
        motion.keepPreferredX = true;
        return motion;
    }
    if (character == '|') {
        const auto block = Vim::navigableBlock(document, position);
        motion.position = block.position() + qMin(_count - 1, qMax(0, block.length() - 2));
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

        if (!success && !forOperator) {
            return motion;
        }
        motion.position = cursor.position();
        return motion;
    }
    if (character == 'b' || character == 'B') {
        Vim::Cursor cursor(document, position);
        if (Vim::backwardWord(cursor, _count, character == 'B', false)) {
            motion.position = cursor.position();
        }
        return motion;
    }

    //
    // Предложения
    //
    if (character == '(' || character == ')') {
        Vim::Cursor cursor(document, position);
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
        return searchMotion(shared().searchPattern, forward, _count, position);
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
        if (shared().findCharacter.isNull()) {
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

VimTextEditController::Motion VimTextEditController::findCharacterMotion(QChar _character,
                                                                         bool _forward, bool _till,
                                                                         int _count,
                                                                         bool _repeat) const
{
    Motion motion;
    motion.inclusive = _forward;

    const Vim::Cursor cursor(document(), currentPosition());
    const auto text = cursor.block().text();
    int column = cursor.column();
    //
    // ... при повторе поиска до символа стоящий рядом искомый символ пропускается
    //
    bool stop = !(_repeat && _till && _count == 1);
    for (int count = _count; count > 0; --count) {
        forever
        {
            if (_forward) {
                ++column;
                if (column >= text.size()) {
                    return motion;
                }
            } else {
                if (column == 0) {
                    return motion;
                }
                --column;
            }
            if (text.at(column) == _character && stop) {
                break;
            }
            stop = true;
        }
    }

    if (_till) {
        column += _forward ? -1 : 1;
    }
    motion.position = cursor.block().position() + column;
    return motion;
}

VimTextEditController::Motion VimTextEditController::markMotion(QChar _name, bool _exact) const
{
    Motion motion;
    const auto name = _name == '\'' ? QChar('`') : _name;
    if (!m_marks.contains(name) || m_marks.value(name).isNull()) {
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
                                                                  int _fromPosition)
{
    Motion motion;
    motion.jump = true;
    motion.toNumberedRegister = true;
    if (_pattern.isEmpty()) {
        return motion;
    }

    int position = _fromPosition;
    for (int count = 0; count < _count; ++count) {
        position = searchPosition(_pattern, position, _forward);
        if (position < 0) {
            return motion;
        }
    }
    motion.position = position;
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
        return;
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
        changeCaseRange(_range, _operator);
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

    shared().registers.setDeletedContent(_registerName, registerContent(_range),
                                         _toNumberedRegister);

    if (!_range.linewise) {
        removeText(_range.start, _range.end);
        setCursorPosition(characterPosition(_range.start));
        return;
    }

    //
    // Чтобы строки действительно исчезли, а не превратились в пустые, захватываем ещё и
    // разделитель абзацев - следующий, а если за строками идёт скрытый текст, то предыдущий,
    // если же скрыт текст с обеих сторон, то очищаем строки, не трогая соседние абзацы
    // @note Позиции вычисляем заранее, т.к. после правки документа блоки становятся невалидными
    //
    auto document = this->document();
    const auto firstBlock = document->findBlock(_range.start);
    const auto lastBlock = document->findBlock(_range.end);
    const auto blockAfter = lastBlock.next();
    const auto blockBefore = firstBlock.previous();
    int cursorPosition = firstBlock.position();
    if (blockAfter.isValid() && blockAfter.isVisible()) {
        removeText(firstBlock.position(), blockAfter.position());
    } else if (blockBefore.isValid() && blockBefore.isVisible()) {
        cursorPosition = blockBefore.position();
        removeText(Vim::lineEndPosition(blockBefore), Vim::lineEndPosition(lastBlock));
    } else {
        removeText(firstBlock.position(), Vim::lineEndPosition(lastBlock));
    }

    //
    // ... курсор ставим на первый значимый символ строки, оказавшейся на месте удалённых
    //
    setCursorPosition(Vim::firstNonBlankPosition(Vim::navigableBlock(document, cursorPosition)));
}

void VimTextEditController::changeRange(const Vim::TextRange& _range, QChar _registerName,
                                        bool _toNumberedRegister)
{
    if (!canEdit() || !_range.isValid()) {
        return;
    }

    shared().registers.setDeletedContent(_registerName, registerContent(_range),
                                         _toNumberedRegister);

    //
    // ... в отличие от удаления, при построчном изменении первая строка остаётся на месте вместе
    //     с отступом, лишь очищается её содержимое
    //
    int start = _range.start;
    if (_range.linewise) {
        start = Vim::firstNonBlankPosition(document()->findBlock(_range.start));
        start = qMin(start, Vim::lineEndPosition(document()->findBlock(_range.start)));
    }
    removeText(start, _range.end);
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

void VimTextEditController::changeCaseRange(const Vim::TextRange& _range, Operator _operator)
{
    if (!canEdit() || !_range.isValid()) {
        return;
    }

    for (const auto& lineRange : lineRanges(_range)) {
        const int from = lineRange.start;
        const int to = lineRange.end;
        auto text = rangeText({ from, to, false });
        for (auto& character : text) {
            switch (_operator) {
            case Operator::Lowercase: {
                character = character.toLower();
                break;
            }
            case Operator::Uppercase: {
                character = character.toUpper();
                break;
            }
            default: {
                character = character.isUpper() ? character.toLower() : character.toUpper();
                break;
            }
            }
        }
        replaceText(from, to, text);
    }

    setCursorPosition(characterPosition(_range.start));
}

Vim::RegisterContent VimTextEditController::registerContent(const Vim::TextRange& _range) const
{
    //
    // ... построчный фрагмент всегда заканчивается переводом строки, даже если последняя
    //     строка пуста
    //
    auto text = rangeText(_range);
    if (_range.linewise) {
        text += '\n';
    }
    return { text, _range.linewise };
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
    return !m_editor->isReadOnly();
}

void VimTextEditController::removeText(int _from, int _to)
{
    if (_from >= _to) {
        return;
    }

    //
    // Удаление отдаём самому редактору, чтобы он корректно обработал границы абзацев, таблиц
    // и режим отслеживания изменений
    //
    QTextCursor cursor(document());
    cursor.setPosition(_from);
    cursor.setPosition(_to, QTextCursor::KeepAnchor);
    m_editor->setTextCursor(cursor);
    sendKeyPressToEditor(Qt::Key_Delete, {});
}

void VimTextEditController::insertText(int _position, const QString& _text)
{
    if (_text.isEmpty()) {
        return;
    }

    QTextCursor cursor(document());
    cursor.setPosition(_position);
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
}

void VimTextEditController::replaceText(int _from, int _to, const QString& _text)
{
    QTextCursor cursor(document());
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
        QVector<Part> parts;
        const auto block = document()->findBlock(_from);
        for (auto iter = block.begin(); !iter.atEnd(); ++iter) {
            const auto fragment = iter.fragment();
            const int from = qMax(_from, fragment.position());
            const int to = qMin(_to, fragment.position() + fragment.length());
            if (to > from) {
                parts.append({ from, to, fragment.charFormat() });
            }
        }
        for (const auto& part : parts) {
            cursor.setPosition(part.from);
            cursor.setPosition(part.to, QTextCursor::KeepAnchor);
            cursor.insertText(_text.mid(part.from - _from, part.to - part.from), part.format);
        }
        cursor.endEditBlock();
        return;
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
        insertText(_from, _text);
        return;
    }
    cursor.insertText(_text, format);
    cursor.endEditBlock();
}

bool VimTextEditController::openLine(bool _below)
{
    if (!canEdit()) {
        return false;
    }

    //
    // Строка выше открывается как строка под предыдущей, чтобы тип нового абзаца выбрал
    // редактор, как при нажатии Enter в конце строки
    //
    auto document = this->document();
    auto block = currentBlock();
    bool below = _below;
    if (!below) {
        const auto previous = block.previous();
        if (previous.isValid() && Vim::isNavigableBlock(previous)) {
            block = previous;
            below = true;
        }
    }

    const int blocksCount = document->blockCount();
    const int blockStart = block.position();
    const int blockEnd = Vim::lineEndPosition(block);

    //
    // Новый абзац создаёт сам редактор, но в пустом абзаце нажатие Enter меняет его тип,
    // поэтому там вставляем абзац сами
    //
    if (block.length() > 1) {
        QTextCursor cursor(document);
        cursor.setPosition(below ? blockEnd : blockStart);
        m_editor->setTextCursor(cursor);
        sendKeyPressToEditor(Qt::Key_Return, QStringLiteral("\r"));

        if (document->blockCount() > blocksCount) {
            auto newBlock = m_editor->textCursor().block();
            if (!below && newBlock.length() > 1) {
                newBlock = newBlock.previous();
            }
            if (newBlock.isValid() && newBlock.length() == 1) {
                QTextCursor newCursor(document);
                newCursor.setPosition(newBlock.position());
                m_editor->setTextCursor(newCursor);
                return true;
            }
        }
    }

    const int position = below ? blockEnd : blockStart;
    insertText(position, QStringLiteral("\n"));
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

    const int start = _previous ? qMax(lineStart, position - _count) : position;
    const int end = _previous ? position : qMin(lineEnd, position + _count);
    if (end <= start) {
        return;
    }

    deleteRange({ start, end, false }, m_pendingRegister, false);
}

void VimTextEditController::replaceCharacters(QChar _character, int _count)
{
    if (!canEdit()) {
        return;
    }

    const int position = currentPosition();
    const int lineEnd = Vim::lineEndPosition(currentBlock());

    //
    // ... если в строке не хватает символов, то замена не выполняется вовсе
    //
    if (position + _count > lineEnd) {
        return;
    }

    replaceText(position, position + _count, QString(_count, _character));

    //
    // ... курсор остаётся на последнем заменённом символе
    //
    setCursorPosition(position + _count - 1);
}

void VimTextEditController::toggleCase(int _count)
{
    if (!canEdit()) {
        return;
    }

    const int position = currentPosition();
    const int lineEnd = Vim::lineEndPosition(currentBlock());
    const int end = qMin(lineEnd, position + _count);
    if (end <= position) {
        return;
    }

    changeCaseRange({ position, end, false }, Operator::SwapCase);
    setCursorPosition(characterPosition(end));
}

void VimTextEditController::joinLines(int _count, bool _withSpaces)
{
    if (!canEdit()) {
        return;
    }

    auto document = this->document();
    auto block = currentBlock();
    int joinPosition = -1;
    for (int step = 0; step < qMax(1, _count - 1); ++step) {
        const auto next = Vim::nextNavigableBlock(block);
        if (!next.isValid()) {
            break;
        }

        //
        // ... не объединяем со скрытым текстом и с продолжением абзаца на следующей странице
        //
        bool canJoin = !block.blockFormat().boolProperty(
            BusinessLayer::TextBlockStyle::PropertyIsBreakCorrectionStart);
        for (auto between = block.next(); canJoin && between.isValid() && between != next;
             between = between.next()) {
            canJoin = between.isVisible();
        }
        if (!canJoin) {
            break;
        }

        const auto currentText = block.text();
        const auto nextText = next.text();
        const int blocksCount = document->blockCount();
        const int lineEnd = Vim::lineEndPosition(block);

        //
        // ... само объединение делает редактор, чтобы корректно обработать декорации
        //
        QTextCursor cursor(document);
        cursor.setPosition(lineEnd);
        m_editor->setTextCursor(cursor);
        sendKeyPressToEditor(Qt::Key_Delete, {});
        if (document->blockCount() >= blocksCount) {
            break;
        }
        block = document->findBlock(lineEnd);
        joinPosition = lineEnd;

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
            QTextCursor whiteCursor(document);
            whiteCursor.setPosition(lineEnd);
            whiteCursor.setPosition(lineEnd + leadingWhite, QTextCursor::KeepAnchor);
            whiteCursor.removeSelectedText();
        }
        const auto rest = nextText.mid(leadingWhite);
        const bool insertSpace = !rest.isEmpty() && !rest.startsWith(')') && !currentText.isEmpty()
            && !currentText.endsWith(' ') && !currentText.endsWith('\t');
        if (insertSpace) {
            QTextCursor spaceCursor(document);
            spaceCursor.setPosition(lineEnd);
            spaceCursor.insertText(QStringLiteral(" "));
        }
        block = document->findBlock(lineEnd);
    }

    if (joinPosition >= 0) {
        setCursorPosition(characterPosition(joinPosition));
    } else {
        setCursorPosition(currentPosition());
    }
}

void VimTextEditController::paste(QChar _registerName, bool _before, int _count, bool _cursorAfter)
{
    pasteContent(shared().registers.content(_registerName), _before, _count, _cursorAfter);
}

void VimTextEditController::pasteContent(const Vim::RegisterContent& _content, bool _before,
                                         int _count, bool _cursorAfter)
{
    if (!canEdit() || _content.isEmpty()) {
        return;
    }

    if (_content.linewise) {
        auto text = _content.text;
        if (text.endsWith('\n')) {
            text.chop(1);
        }
        pasteLines(text, _before, _count, _cursorAfter);
        return;
    }

    const auto block = currentBlock();
    int position = currentPosition();
    if (!_before && block.length() > 1) {
        position = qMin(position + 1, Vim::lineEndPosition(block));
    }

    QString text;
    for (int index = 0; index < _count; ++index) {
        text += _content.text;
    }
    insertText(position, text);

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

void VimTextEditController::pasteLines(const QString& _text, bool _before, int _count,
                                       bool _cursorAfter)
{
    QStringList lines;
    for (int index = 0; index < _count; ++index) {
        lines.append(_text);
    }

    if (!openLine(!_before)) {
        return;
    }

    const int position = m_editor->textCursor().position();
    const auto text = lines.join('\n');
    insertText(position, text);
    if (_cursorAfter) {
        //
        // ... курсор встаёт в начало строки, следующей за вставленными
        //
        const auto lastBlock = document()->findBlock(position + text.size());
        const auto next = Vim::nextNavigableBlock(lastBlock);
        setCursorPosition(next.isValid() ? next.position() : lastBlock.position());
        return;
    }
    setCursorPosition(Vim::firstNonBlankPosition(document()->findBlock(position)));
}

void VimTextEditController::replaceVisualWithRegister(QChar _registerName, bool _keepRegisters,
                                                      int _count)
{
    const auto content = shared().registers.content(_registerName);
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
        auto text = content.text;
        if (text.endsWith('\n')) {
            text.chop(1);
        }
        pasteLines(text, currentBlock().position() >= range.start, _count);
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
        insertText(splitPosition, QStringLiteral("\n") + lines.join('\n') + QStringLiteral("\n"));
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

void VimTextEditController::sendKeyPressToEditor(int _key, const QString& _text)
{
    QScopedValueRollback<int> passThroughRollback(m_passThroughDepth, m_passThroughDepth + 1);
    QKeyEvent event(QEvent::KeyPress, _key, Qt::NoModifier, _text);
    QGuiApplication::sendEvent(m_editor, &event);
}

void VimTextEditController::commitUndoStep()
{
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

int VimTextEditController::searchPosition(const QString& _pattern, int _fromPosition,
                                          bool _forward) const
{
    if (_pattern.isEmpty()) {
        return -1;
    }

    const auto expression = Vim::regularExpression(_pattern);
    auto document = this->document();
    const Vim::Cursor start(document, _fromPosition);
    const auto startBlock = start.block();

    //
    // Ищем в строках, по которым может перемещаться курсор, продолжая с другого конца документа
    //
    auto findInBlock
        = [&expression](const QTextBlock& _block, int _fromColumn, int _toColumn, bool _forward) {
              //
              // ... совпадения ищем с начала строки, т.к. с \zs начало совпадения может оказаться
              //     дальше того места, откуда шаблон начинает сопоставляться
              //
              const auto text = _block.text();
              int found = -1;
              int column = 0;
              while (column <= text.size()) {
                  const auto match = expression.match(text, column);
                  if (!match.hasMatch()) {
                      break;
                  }
                  const int matchStart = static_cast<int>(match.capturedStart());
                  if (matchStart > _toColumn) {
                      break;
                  }
                  if (matchStart >= _fromColumn) {
                      found = matchStart;
                      if (_forward) {
                          break;
                      }
                  }
                  column = qMax(column + 1, matchStart + 1);
              }
              return found >= 0 ? _block.position() + found : -1;
          };

    if (_forward) {
        int found = findInBlock(startBlock, start.column() + 1, startBlock.length(), true);
        if (found >= 0) {
            return found;
        }
        for (auto block = Vim::nextNavigableBlock(startBlock); block.isValid();
             block = Vim::nextNavigableBlock(block)) {
            found = findInBlock(block, 0, block.length(), true);
            if (found >= 0) {
                return found;
            }
        }
        for (auto block = Vim::firstNavigableBlock(document);
             block.isValid() && block.blockNumber() <= startBlock.blockNumber();
             block = Vim::nextNavigableBlock(block)) {
            found = findInBlock(block, 0, block == startBlock ? start.column() : block.length(),
                                true);
            if (found >= 0) {
                return found;
            }
        }
        return -1;
    }

    int found = findInBlock(startBlock, 0, start.column() - 1, false);
    if (found >= 0) {
        return found;
    }
    for (auto block = Vim::previousNavigableBlock(startBlock); block.isValid();
         block = Vim::previousNavigableBlock(block)) {
        found = findInBlock(block, 0, block.length(), false);
        if (found >= 0) {
            return found;
        }
    }
    for (auto block = Vim::lastNavigableBlock(document);
         block.isValid() && block.blockNumber() >= startBlock.blockNumber();
         block = Vim::previousNavigableBlock(block)) {
        found = findInBlock(block, block == startBlock ? start.column() : 0, block.length(), false);
        if (found >= 0) {
            return found;
        }
    }
    return -1;
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

    if (_event->key() == Qt::Key_Backspace) {
        m_commandLine.chop(1);
        if (m_commandLine.isEmpty()) {
            closeCommandLine();
        }
        return;
    }

    if (hasVimControl(_event) && _event->key() == Qt::Key_U) {
        m_commandLine = m_commandLine.left(1);
        return;
    }

    const auto text = _event->text();
    if (!text.isEmpty() && text.at(0).isPrint()) {
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
    auto body = _command.mid(1);
    if (prefix == '/' || prefix == '?') {
        //
        // ... пустой шаблон означает повтор последнего поиска
        //
        if (body.isEmpty()) {
            body = shared().searchPattern;
        }
        shared().searchPattern = body;
        shared().searchForward = prefix == '/';
        const int count = takeCount();
        auto motion = searchMotion(body, prefix == '/', count, currentPosition());
        applyMotion(motion);
        return;
    }

    resetPendingCommand();
    if (prefix == ':') {
        executeExCommand(body);
    }
}

void VimTextEditController::executeExCommand(const QString& _command)
{
    auto document = this->document();
    const auto command = _command.trimmed();
    int index = 0;

    //
    // Разбираем диапазон строк: %, номера, ., $, метки и смещения
    //
    auto currentLine = Vim::navigableBlockNumber(currentBlock());
    const int lineCount = Vim::navigableBlocksCount(document);
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
                if (!m_marks.contains(name)) {
                    return false;
                }
                line = Vim::navigableBlockNumber(
                    Vim::navigableBlock(document, m_marks.value(name).position()));
                index += 2;
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
    bool hasRange = false;
    if (command.startsWith('%')) {
        firstLine = 1;
        lastLine = lineCount;
        index = 1;
        hasRange = true;
    } else {
        int line = currentLine;
        if (parseAddress(&line)) {
            firstLine = lastLine = line;
            hasRange = true;
        }
        if (index < command.size() && (command.at(index) == ',' || command.at(index) == ';')) {
            if (command.at(index) == ';') {
                currentLine = firstLine;
            }
            ++index;
            if (parseAddress(&line)) {
                lastLine = line;
            }
            hasRange = true;
        }
    }
    firstLine = qBound(1, firstLine, lineCount);
    lastLine = qBound(1, lastLine, lineCount);
    if (firstLine > lastLine) {
        std::swap(firstLine, lastLine);
    }

    const auto name = command.mid(index).trimmed();
    const auto firstBlock = Vim::navigableBlockByNumber(document, firstLine);
    const auto lastBlock = Vim::navigableBlockByNumber(document, lastLine);

    //
    // ... переход к строке по её номеру
    //
    if (name.isEmpty()) {
        if (hasRange) {
            rememberJumpPosition();
            setCursorPosition(Vim::firstNonBlankPosition(lastBlock));
        }
        return;
    }

    //
    // ... замена текста
    //
    if (name.startsWith('&')) {
        substitute(firstBlock, lastBlock, name);
        return;
    }
    if (name.startsWith('s')) {
        //
        // ... команда может быть сокращена до любого начала слова substitute
        //
        const QString substituteCommand = QStringLiteral("substitute");
        int length = 1;
        while (length < name.size() && length < substituteCommand.size()
               && name.at(length) == substituteCommand.at(length)) {
            ++length;
        }
        const auto arguments = name.mid(length);
        if (!arguments.isEmpty() && arguments.at(0).isLetterOrNumber()) {
            return;
        }
        substitute(firstBlock, lastBlock, arguments);
        return;
    }

    //
    // ... удаление, копирование и объединение строк
    //
    const auto words = name.split(' ', Qt::SkipEmptyParts);
    const auto commandName = words.value(0);
    const auto registerName = words.value(1).isEmpty() ? QChar() : words.value(1).at(0);
    if (QStringLiteral("delete").startsWith(commandName)) {
        deleteRange(linesRange(firstBlock.position(), lastBlock.position()), registerName, true);
    } else if (QStringLiteral("yank").startsWith(commandName)) {
        yankRange(linesRange(firstBlock.position(), lastBlock.position()), registerName,
                  currentPosition());
    } else if (QStringLiteral("join").startsWith(commandName)) {
        setCursorPosition(firstBlock.position());
        joinLines(qMax(2, lastLine - firstLine + 1), true);
        setCursorPosition(Vim::firstNonBlankPosition(currentBlock()));
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
        if (flags.startsWith('&')) {
            flags = state.substituteFlags + flags.mid(1);
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
    // ... с флагом n совпадения только подсчитываются, поэтому текст не трогаем
    //
    if (flags.contains('n')) {
        return;
    }

    //
    // Идём по строкам снизу вверх, чтобы замены с переводами строк не сбивали позиции
    //
    QVector<int> lines;
    for (auto block = _firstLine; block.isValid(); block = Vim::nextNavigableBlock(block)) {
        lines.append(block.position());
        if (block == _lastLine) {
            break;
        }
    }

    QTextCursor lastChangedLine;
    for (int lineIndex = lines.size() - 1; lineIndex >= 0; --lineIndex) {
        const auto block = document()->findBlock(lines.at(lineIndex));
        const int blockPosition = block.position();
        const auto text = block.text();

        struct Replacement {
            int from = 0;
            int to = 0;
            QString text;
        };
        QVector<Replacement> replacements;
        int column = 0;
        bool isFirst = true;
        forever
        {
            //
            // ... как и в vim, после совпадения, дошедшего до конца строки, поиск прекращается
            //
            if (!isFirst && column >= text.size()) {
                break;
            }
            const auto match = expression.match(text, column);
            if (!match.hasMatch()) {
                break;
            }
            replacements.append({ static_cast<int>(match.capturedStart()),
                                  static_cast<int>(match.capturedEnd()),
                                  Vim::substitution(match, replacement) });
            isFirst = false;
            if (!replaceAll) {
                break;
            }
            column = match.capturedLength() == 0 ? match.capturedEnd() + 1 : match.capturedEnd();
        }

        if (replacements.isEmpty()) {
            continue;
        }

        for (int index = replacements.size() - 1; index >= 0; --index) {
            const auto& item = replacements.at(index);
            replaceText(blockPosition + item.from, blockPosition + item.to, item.text);

            //
            // ... запоминаем конец последней замены, ведь замена с переводами строк делит строку
            //
            if (lastChangedLine.isNull()) {
                lastChangedLine = QTextCursor(document());
                lastChangedLine.setPosition(blockPosition + item.from + item.text.size());
            }
        }
    }

    //
    // ... курсор встаёт в начало последней изменённой строки
    //
    if (!lastChangedLine.isNull()) {
        setCursorPosition(Vim::firstNonBlankPosition(
            Vim::navigableBlock(document(), lastChangedLine.position())));
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
    // ... и собираем команду, которую можно будет повторить
    //
    if (m_commandKeys.size() < kMaxRecordedKeys) {
        m_commandKeys.append(_key);
    }

    const bool isCommandFinished = m_mode == Mode::Normal && !hasPendingInput();
    if (!isCommandFinished) {
        return;
    }

    const bool isChanged = document()->revision() != m_commandRevision;
    if (isChanged) {
        if (m_commandIsChange) {
            m_lastChangeKeys = m_commandKeys;
        }
        setMark('.', currentPosition());
        commitUndoStep();
    }
    m_commandKeys.clear();
    m_commandRevision = document()->revision();
    m_commandIsChange = true;
}

void VimTextEditController::toggleMacroRecording(QChar _name)
{
    m_commandIsChange = false;

    if (!m_recordingRegister.isNull()) {
        shared().macros[m_recordingRegister.toLower()] = m_recordedKeys;
        m_recordedKeys.clear();
        m_recordingRegister = QChar();
        return;
    }

    //
    // ... заглавная буква дописывает нажатия к уже записанному макросу
    //
    m_recordingRegister = _name;
    m_recordedKeys
        = _name.isUpper() ? shared().macros.value(_name.toLower()) : QVector<RecordedKey>();
    m_skipMacroRecordKey = true;
}

void VimTextEditController::playMacro(QChar _name, int _count)
{
    const auto name = _name.toLower();
    const auto keys = shared().macros.value(name);
    if (keys.isEmpty()) {
        return;
    }

    shared().lastPlayedMacro = name;
    for (int step = 0; step < _count; ++step) {
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

    replayKeys(keys);
}

void VimTextEditController::replayKeys(const QVector<RecordedKey>& _keys)
{
    if (_keys.isEmpty() || m_replayDepth >= kMaxReplayDepth) {
        return;
    }

    ++m_replayDepth;
    for (const auto& key : _keys) {
        if (m_replayedKeys >= kMaxReplayedKeys) {
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
    m_editor->ensureCursorVisible();
    _event->accept();
}

} // namespace Ui
