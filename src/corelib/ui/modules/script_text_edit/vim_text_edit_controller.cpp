#include "vim_text_edit_controller.h"

#include "script_text_edit.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QScopedValueRollback>
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

bool hasCommandModifier(QKeyEvent* _event)
{
    return _event->modifiers().testFlag(Qt::AltModifier)
        || _event->modifiers().testFlag(Qt::MetaModifier)
        || _event->modifiers().testFlag(Qt::ControlModifier);
}

bool isEscape(QKeyEvent* _event)
{
    return _event->key() == Qt::Key_Escape
        || (_event->key() == Qt::Key_BracketLeft
            && _event->modifiers().testFlag(Qt::ControlModifier));
}

bool isDigit(const QString& _text)
{
    return _text.size() == 1 && _text.at(0).isDigit();
}

} // namespace

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

    m_enabled = _enabled;
    resetPendingCommand();
    resetVisualSelection();
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

bool VimTextEditController::usesBlockCursor() const
{
    return m_enabled && m_mode != Mode::Insert;
}

int VimTextEditController::blockCursorPosition() const
{
    if (isVisualMode() && m_visualCursorPosition >= 0) {
        return boundedPosition(m_visualCursorPosition);
    }

    return boundedPosition(m_editor->textCursor().position());
}

bool VimTextEditController::handleShortcutOverride(QKeyEvent* _event) const
{
    if (!m_enabled || _event == nullptr) {
        return false;
    }

    if (isEscape(_event)) {
        return true;
    }

    if (!canEdit()) {
        return false;
    }

    if (_event->matches(QKeySequence::Undo) || _event->matches(QKeySequence::Redo)) {
        return true;
    }

    return m_mode != Mode::Insert && _event->modifiers().testFlag(Qt::ControlModifier)
        && _event->key() == Qt::Key_R;
}

bool VimTextEditController::handleKeyPress(QKeyEvent* _event)
{
    if (!m_enabled || _event == nullptr) {
        return false;
    }

    if (isEscape(_event)) {
        //
        // Сперва закрываем подстановщика, если он показан, чтобы не оставлять его висеть
        // поверх текста, и только следующим нажатием выходим из режима вставки
        //
        if (m_editor->isCompleterVisible()) {
            m_editor->closeCompleter();
        } else {
            enterNormalMode(m_mode == Mode::Insert);
        }
        finishHandledEvent(_event);
        return true;
    }

    if (m_mode == Mode::Insert) {
        return false;
    }

    if (_event->modifiers().testFlag(Qt::ControlModifier) && _event->key() == Qt::Key_R) {
        if (canEdit()) {
            redo();
        }
        finishHandledEvent(_event);
        return true;
    }

    if (hasCommandModifier(_event)) {
        return false;
    }

    const bool handled = isVisualMode() ? handleVisualMode(_event) : handleNormalMode(_event);
    if (handled) {
        finishHandledEvent(_event);
    }
    return handled;
}

bool VimTextEditController::isVisualMode() const
{
    return m_mode == Mode::Visual || m_mode == Mode::VisualLine;
}

bool VimTextEditController::isInclusiveMotion(Motion _motion) const
{
    return _motion == Motion::EndOfWord;
}

void VimTextEditController::resetPendingCommand()
{
    m_pendingOperator = Operator::None;
    m_pendingOperatorCount = 0;
    m_count.clear();
}

void VimTextEditController::enterInsertMode()
{
    resetPendingCommand();
    resetVisualSelection();
    m_mode = Mode::Insert;
}

void VimTextEditController::enterNormalMode(bool _fromInsert)
{
    resetPendingCommand();

    auto cursor = m_editor->textCursor();
    if (isVisualMode() && m_visualCursorPosition >= 0) {
        cursor.clearSelection();
        cursor.setPosition(boundedPosition(m_visualCursorPosition));
    } else if (_fromInsert && !cursor.hasSelection() && !cursor.atBlockStart()) {
        cursor.movePosition(QTextCursor::PreviousCharacter);
    } else if (cursor.hasSelection()) {
        cursor.clearSelection();
    }
    m_editor->setTextCursor(cursor);

    resetVisualSelection();
    m_mode = Mode::Normal;
}

void VimTextEditController::enterVisualMode(Mode _mode)
{
    resetPendingCommand();

    //
    // Если переключаемся между VISUAL и VISUAL LINE, то сохраняем текущее выделение
    //
    if (!isVisualMode() || m_visualAnchorPosition < 0 || m_visualCursorPosition < 0) {
        auto cursor = m_editor->textCursor();
        cursor.clearSelection();
        m_visualAnchorPosition = visualCursorPosition(cursor.position());
        m_visualCursorPosition = m_visualAnchorPosition;
    }

    m_mode = _mode;
    updateVisualSelection();
}

void VimTextEditController::leaveVisualMode()
{
    resetPendingCommand();
    resetVisualSelection();
    m_mode = Mode::Normal;
}

void VimTextEditController::resetVisualSelection()
{
    m_visualAnchorPosition = -1;
    m_visualCursorPosition = -1;
}

void VimTextEditController::updateVisualSelection()
{
    if (!isVisualMode() || m_visualAnchorPosition < 0 || m_visualCursorPosition < 0) {
        return;
    }

    const int anchorPosition = boundedPosition(m_visualAnchorPosition);
    const int cursorPosition = boundedPosition(m_visualCursorPosition);
    int selectionStart = qMin(anchorPosition, cursorPosition);
    int selectionEnd = qMax(anchorPosition, cursorPosition);
    if (m_mode == Mode::VisualLine) {
        const auto lineCursor = linesCursor(selectionStart, selectionEnd, false);
        selectionStart = lineCursor.anchor();
        selectionEnd = lineCursor.position();
    } else {
        selectionEnd = visualSelectionEndPosition(selectionEnd);
    }

    QTextCursor cursor(m_editor->document());
    if (cursorPosition < anchorPosition) {
        cursor.setPosition(selectionEnd);
        cursor.setPosition(selectionStart, QTextCursor::KeepAnchor);
    } else {
        cursor.setPosition(selectionStart);
        cursor.setPosition(selectionEnd, QTextCursor::KeepAnchor);
    }
    m_editor->setTextCursor(cursor);
}

void VimTextEditController::appendCount(const QString& _digit)
{
    if (m_count.size() >= kMaxCountDigits) {
        return;
    }

    m_count.append(_digit);
}

int VimTextEditController::takeCount(int _default)
{
    const auto count = m_count.isEmpty() ? _default : m_count.toInt();
    m_count.clear();
    return qBound(1, count, kMaxCount);
}

int VimTextEditController::takeOptionalCount()
{
    if (m_count.isEmpty()) {
        return 0;
    }

    return takeCount();
}

int VimTextEditController::combinedOperatorCount()
{
    return qBound(1, qMax(1, m_pendingOperatorCount) * takeCount(1), kMaxCount);
}

bool VimTextEditController::handleNormalMode(QKeyEvent* _event)
{
    const bool canEditText = canEdit();

    if (m_pendingOperator == Operator::Replace) {
        return handlePendingReplace(_event);
    }

    if (m_pendingOperator != Operator::None) {
        return handlePendingOperator(_event);
    }

    const auto text = _event->text();
    if (isDigit(text) && (text != "0" || !m_count.isEmpty())) {
        appendCount(text);
        return true;
    }

    const auto key = _event->key();
    const auto motion = motionFromKey(_event);
    if (motion != Motion::None) {
        move(motion, takeCount());
        return true;
    }

    switch (key) {
    case Qt::Key_I: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        if (text == "I") {
            move(Motion::FirstNonBlank, 1);
        }
        enterInsertMode();
        return true;
    }

    case Qt::Key_A: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        if (text == "A") {
            move(Motion::EndOfLine, 1);
        } else {
            auto cursor = m_editor->textCursor();
            cursor.clearSelection();
            if (!cursor.atBlockEnd()) {
                cursor.movePosition(QTextCursor::NextCharacter);
            }
            m_editor->setTextCursor(cursor);
        }
        enterInsertMode();
        return true;
    }

    case Qt::Key_O: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        if (text == "O") {
            openLineAbove();
        } else {
            openLineBelow();
        }
        enterInsertMode();
        return true;
    }

    case Qt::Key_V: {
        enterVisualMode(text == "V" ? Mode::VisualLine : Mode::Visual);
        return true;
    }

    case Qt::Key_U: {
        if (text != "u" || !canEditText) {
            resetPendingCommand();
            return true;
        }
        undo();
        return true;
    }

    case Qt::Key_X: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        deleteCharacters(takeCount(), text == "X");
        return true;
    }

    case Qt::Key_D: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        if (text == "D") {
            deleteToEndOfLine();
        } else {
            m_pendingOperator = Operator::Delete;
            m_pendingOperatorCount = takeOptionalCount();
        }
        return true;
    }

    case Qt::Key_C: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        if (text == "C") {
            deleteToEndOfLine();
            enterInsertMode();
        } else {
            m_pendingOperator = Operator::Change;
            m_pendingOperatorCount = takeOptionalCount();
        }
        return true;
    }

    case Qt::Key_Y: {
        if (text == "Y") {
            const int fromPosition = m_editor->textCursor().position();
            yankLines(fromPosition, lastLinePosition(fromPosition, takeCount()));
        } else {
            m_pendingOperator = Operator::Yank;
            m_pendingOperatorCount = takeOptionalCount();
        }
        return true;
    }

    case Qt::Key_G: {
        if (text == "G") {
            const int lineNumber = takeOptionalCount();
            goToLine(lineNumber > 0 ? lineNumber : m_editor->document()->blockCount());
        } else {
            m_pendingOperator = Operator::Go;
            m_pendingOperatorCount = takeOptionalCount();
        }
        return true;
    }

    case Qt::Key_P: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        paste(text == "P", takeCount());
        return true;
    }

    case Qt::Key_R: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        m_pendingOperator = Operator::Replace;
        m_pendingOperatorCount = takeOptionalCount();
        return true;
    }

    case Qt::Key_S: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        if (text == "S") {
            const int fromPosition = m_editor->textCursor().position();
            const int count = takeCount();
            changeLines(fromPosition, lastLinePosition(fromPosition, count), true);
        } else {
            deleteCharacters(takeCount());
            enterInsertMode();
        }
        return true;
    }

    default: {
        resetPendingCommand();
        //
        // Служебные клавиши, вроде листания страниц, отдаём на откуп самому редактору,
        // а всё, что вводит текст, поглощаем, чтобы не портить документ
        //
        return !_event->text().isEmpty();
    }
    }
}

bool VimTextEditController::handleVisualMode(QKeyEvent* _event)
{
    const bool canEditText = canEdit();
    const auto text = _event->text();

    if (isDigit(text) && (text != "0" || !m_count.isEmpty())) {
        appendCount(text);
        return true;
    }

    if (m_pendingOperator == Operator::Go) {
        return handlePendingGo(_event);
    }

    const auto motion = motionFromKey(_event);
    if (motion != Motion::None) {
        moveVisual(motion, takeCount());
        return true;
    }

    switch (_event->key()) {
    case Qt::Key_V: {
        const auto targetMode = text == "V" ? Mode::VisualLine : Mode::Visual;
        if (m_mode == targetMode) {
            enterNormalMode();
        } else {
            enterVisualMode(targetMode);
        }
        return true;
    }

    case Qt::Key_Y: {
        const int anchorPosition = m_visualAnchorPosition;
        const int cursorPosition = m_visualCursorPosition;
        if (m_mode == Mode::VisualLine) {
            yankLines(anchorPosition, cursorPosition);
        } else {
            yankSelection();
        }
        //
        // ... курсор ставим в начало скопированного фрагмента, как это делает vim
        //
        m_visualCursorPosition = qMin(anchorPosition, cursorPosition);
        enterNormalMode();
        return true;
    }

    case Qt::Key_U: {
        if (text == "u" && canEditText) {
            undo();
        } else {
            resetPendingCommand();
        }
        return true;
    }

    case Qt::Key_G: {
        if (text == "G") {
            const int lineNumber = takeOptionalCount();
            goToLine(lineNumber > 0 ? lineNumber : m_editor->document()->blockCount());
        } else {
            m_pendingOperator = Operator::Go;
            m_pendingOperatorCount = takeOptionalCount();
        }
        return true;
    }

    case Qt::Key_D:
    case Qt::Key_X: {
        if (!canEditText) {
            enterNormalMode();
            return true;
        }
        const int anchorPosition = m_visualAnchorPosition;
        const int cursorPosition = m_visualCursorPosition;
        const bool linewise = m_mode == Mode::VisualLine;
        leaveVisualMode();
        if (linewise) {
            deleteLines(anchorPosition, cursorPosition, true);
        } else {
            deleteSelection();
        }
        return true;
    }

    case Qt::Key_C:
    case Qt::Key_S: {
        if (!canEditText) {
            enterNormalMode();
            return true;
        }
        const int anchorPosition = m_visualAnchorPosition;
        const int cursorPosition = m_visualCursorPosition;
        const bool linewise = m_mode == Mode::VisualLine;
        leaveVisualMode();
        if (linewise) {
            changeLines(anchorPosition, cursorPosition, true);
        } else {
            changeSelection();
        }
        return true;
    }

    case Qt::Key_P: {
        if (!canEditText) {
            enterNormalMode();
            return true;
        }
        replaceSelectionWithRegister();
        return true;
    }

    default: {
        resetPendingCommand();
        return true;
    }
    }
}

bool VimTextEditController::handlePendingOperator(QKeyEvent* _event)
{
    const auto text = _event->text();
    if (isDigit(text) && (text != "0" || !m_count.isEmpty())) {
        appendCount(text);
        return true;
    }

    if (m_pendingOperator == Operator::Go) {
        return handlePendingGo(_event);
    }

    if ((m_pendingOperator == Operator::Delete || m_pendingOperator == Operator::Change)
        && !canEdit()) {
        resetPendingCommand();
        return true;
    }

    //
    // Оператор, повторённый дважды, работает построчно: dd, cc, yy
    //
    const bool linewise = (_event->key() == Qt::Key_D && m_pendingOperator == Operator::Delete)
        || (_event->key() == Qt::Key_C && m_pendingOperator == Operator::Change)
        || (_event->key() == Qt::Key_Y && m_pendingOperator == Operator::Yank);
    if (linewise) {
        const auto pendingOperator = m_pendingOperator;
        const int fromPosition = m_editor->textCursor().position();
        const int toPosition = lastLinePosition(fromPosition, combinedOperatorCount());
        resetPendingCommand();
        switch (pendingOperator) {
        case Operator::Yank: {
            yankLines(fromPosition, toPosition);
            break;
        }
        case Operator::Delete: {
            deleteLines(fromPosition, toPosition, true);
            break;
        }
        case Operator::Change: {
            changeLines(fromPosition, toPosition, true);
            break;
        }
        default: {
            break;
        }
        }
        return true;
    }

    const auto motion = motionFromKey(_event);
    if (motion == Motion::None) {
        resetPendingCommand();
        return true;
    }

    const auto pendingOperator = m_pendingOperator;
    const auto count = combinedOperatorCount();
    const auto sourcePosition = m_editor->textCursor().position();
    resetPendingCommand();

    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    if (!moveCursor(cursor, motion, count) || cursor.position() == sourcePosition) {
        return true;
    }

    //
    // ... перемещения, включающие в себя символ под курсором, захватывают его в выделение
    //
    auto targetPosition = cursor.position();
    if (isInclusiveMotion(motion) && targetPosition > sourcePosition) {
        cursor.setPosition(targetPosition);
        if (!cursor.atBlockEnd()) {
            cursor.movePosition(QTextCursor::NextCharacter);
        }
        targetPosition = cursor.position();
    }

    cursor.setPosition(sourcePosition);
    cursor.setPosition(targetPosition, QTextCursor::KeepAnchor);
    m_editor->setTextCursor(cursor);

    switch (pendingOperator) {
    case Operator::Yank: {
        yankSelection();
        cursor.setPosition(qMin(sourcePosition, targetPosition));
        m_editor->setTextCursor(cursor);
        break;
    }
    case Operator::Delete: {
        deleteSelection();
        break;
    }
    case Operator::Change: {
        changeSelection();
        break;
    }
    default: {
        break;
    }
    }

    return true;
}

bool VimTextEditController::handlePendingGo(QKeyEvent* _event)
{
    const bool isGoToLine = _event->key() == Qt::Key_G && _event->text() == "g";
    const int lineNumber
        = m_pendingOperatorCount > 0 ? m_pendingOperatorCount : takeOptionalCount();
    resetPendingCommand();

    if (isGoToLine) {
        goToLine(lineNumber > 0 ? lineNumber : 1);
    }
    return true;
}

bool VimTextEditController::handlePendingReplace(QKeyEvent* _event)
{
    const auto text = _event->text();
    const int count = combinedOperatorCount();
    resetPendingCommand();

    if (canEdit() && !text.isEmpty() && text.at(0).isPrint()) {
        replaceCharacters(text, count);
    }
    return true;
}

VimTextEditController::Motion VimTextEditController::motionFromKey(QKeyEvent* _event) const
{
    const auto text = _event->text();
    switch (_event->key()) {
    case Qt::Key_H:
    case Qt::Key_Left:
    case Qt::Key_Backspace:
        return Motion::Left;
    case Qt::Key_J:
    case Qt::Key_Down:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return Motion::Down;
    case Qt::Key_K:
    case Qt::Key_Up:
        return Motion::Up;
    case Qt::Key_L:
    case Qt::Key_Right:
    case Qt::Key_Space:
        return Motion::Right;
    case Qt::Key_0:
    case Qt::Key_Home:
        return Motion::StartOfLine;
    case Qt::Key_AsciiCircum:
        return Motion::FirstNonBlank;
    case Qt::Key_Dollar:
    case Qt::Key_End:
        return Motion::EndOfLine;
    case Qt::Key_W:
        return text == "W" ? Motion::None : Motion::NextWord;
    case Qt::Key_B:
        return text == "B" ? Motion::None : Motion::PreviousWord;
    case Qt::Key_E:
        return text == "E" ? Motion::None : Motion::EndOfWord;
    default:
        return Motion::None;
    }
}

bool VimTextEditController::move(Motion _motion, int _count)
{
    auto cursor = m_editor->textCursor();
    cursor.clearSelection();

    const bool moved = moveCursor(cursor, _motion, _count);
    if (moved) {
        m_editor->setTextCursor(cursor);
    }
    return moved;
}

bool VimTextEditController::moveVisual(Motion _motion, int _count)
{
    if (m_visualAnchorPosition < 0 || m_visualCursorPosition < 0) {
        auto cursor = m_editor->textCursor();
        cursor.clearSelection();
        m_visualAnchorPosition = visualCursorPosition(cursor.position());
        m_visualCursorPosition = m_visualAnchorPosition;
    }

    QTextCursor cursor(m_editor->document());
    cursor.setPosition(boundedPosition(m_visualCursorPosition));
    const bool moved = moveCursor(cursor, _motion, _count);
    if (moved) {
        m_visualCursorPosition = visualCursorPosition(cursor.position());
        updateVisualSelection();
    }
    return moved;
}

bool VimTextEditController::moveCursor(QTextCursor& _cursor, Motion _motion, int _count) const
{
    //
    // Перемещения в пределах строки выполняются единожды, вне зависимости от количества повторений
    //
    switch (_motion) {
    case Motion::None: {
        return false;
    }
    case Motion::StartOfLine: {
        _cursor.movePosition(QTextCursor::StartOfBlock);
        return true;
    }
    case Motion::FirstNonBlank: {
        moveToFirstNonBlank(_cursor);
        return true;
    }
    case Motion::EndOfLine: {
        _cursor.movePosition(QTextCursor::EndOfBlock);
        return true;
    }
    default: {
        break;
    }
    }

    bool moved = false;
    for (int index = 0; index < qMax(1, _count); ++index) {
        bool movedOnStep = false;
        switch (_motion) {
        case Motion::Left: {
            movedOnStep = moveLeft(_cursor);
            break;
        }
        case Motion::Down: {
            movedOnStep = _cursor.movePosition(QTextCursor::Down);
            break;
        }
        case Motion::Up: {
            movedOnStep = _cursor.movePosition(QTextCursor::Up);
            break;
        }
        case Motion::Right: {
            movedOnStep = moveRight(_cursor);
            break;
        }
        case Motion::NextWord: {
            movedOnStep = _cursor.movePosition(QTextCursor::NextWord);
            break;
        }
        case Motion::PreviousWord: {
            movedOnStep = _cursor.movePosition(QTextCursor::PreviousWord);
            break;
        }
        case Motion::EndOfWord: {
            movedOnStep = moveToEndOfWord(_cursor);
            break;
        }
        default: {
            break;
        }
        }

        //
        // ... если дальше двигаться некуда, то нет смысла продолжать
        //
        if (!movedOnStep) {
            break;
        }
        moved = true;
    }
    return moved;
}

bool VimTextEditController::moveLeft(QTextCursor& _cursor) const
{
    moveToCharacterUnderBlockCursor(_cursor);
    if (_cursor.position() <= _cursor.block().position()) {
        return false;
    }

    return _cursor.movePosition(QTextCursor::PreviousCharacter);
}

bool VimTextEditController::moveRight(QTextCursor& _cursor) const
{
    if (_cursor.block().length() <= 1) {
        return false;
    }

    const int lastCharacterPosition = _cursor.block().position() + _cursor.block().length() - 2;
    if (_cursor.position() >= lastCharacterPosition) {
        return false;
    }

    return _cursor.movePosition(QTextCursor::NextCharacter);
}

bool VimTextEditController::moveToEndOfWord(QTextCursor& _cursor) const
{
    //
    // В vim курсор встаёт на последний символ слова, поэтому идём вперёд до тех пор, пока не
    // найдём конец слова, расположенный дальше текущей позиции курсора
    //
    const int startPosition = _cursor.position();
    auto cursor = _cursor;
    while (cursor.movePosition(QTextCursor::NextCharacter)) {
        auto endOfWordCursor = cursor;
        if (!endOfWordCursor.movePosition(QTextCursor::EndOfWord)) {
            continue;
        }

        const int endOfWordPosition = endOfWordCursor.position() - 1;
        if (endOfWordPosition > startPosition) {
            _cursor.setPosition(endOfWordPosition);
            return true;
        }
    }
    return false;
}

void VimTextEditController::moveToFirstNonBlank(QTextCursor& _cursor) const
{
    const auto block = _cursor.block();
    const auto text = block.text();
    int offset = 0;
    while (offset < text.size() && text.at(offset).isSpace()) {
        ++offset;
    }
    _cursor.setPosition(block.position() + offset);
}

void VimTextEditController::moveToCharacterUnderBlockCursor(QTextCursor& _cursor) const
{
    if (_cursor.atBlockEnd() && _cursor.block().length() > 1) {
        _cursor.movePosition(QTextCursor::PreviousCharacter);
    }
}

void VimTextEditController::goToLine(int _lineNumber)
{
    goToPosition(linePosition(_lineNumber));
}

void VimTextEditController::goToPosition(int _position)
{
    if (isVisualMode()) {
        m_visualCursorPosition = visualCursorPosition(_position);
        updateVisualSelection();
    } else {
        setCursorPosition(_position);
    }
}

void VimTextEditController::setCursorPosition(int _position)
{
    auto cursor = m_editor->textCursor();
    cursor.setPosition(boundedPosition(_position));
    m_editor->setTextCursor(cursor);
}

int VimTextEditController::linePosition(int _lineNumber) const
{
    auto document = m_editor->document();
    const auto block
        = document->findBlockByNumber(qBound(0, _lineNumber - 1, document->blockCount() - 1));
    QTextCursor cursor(document);
    cursor.setPosition(block.position());
    moveToFirstNonBlank(cursor);
    return cursor.position();
}

int VimTextEditController::lastLinePosition(int _fromPosition, int _count) const
{
    QTextCursor cursor(m_editor->document());
    cursor.setPosition(boundedPosition(_fromPosition));
    for (int index = 1; index < qMax(1, _count); ++index) {
        if (!cursor.movePosition(QTextCursor::NextBlock)) {
            break;
        }
    }
    return cursor.position();
}

int VimTextEditController::boundedPosition(int _position) const
{
    const int maxPosition = qMax(0, m_editor->document()->characterCount() - 1);
    return qBound(0, _position, maxPosition);
}

int VimTextEditController::visualCursorPosition(int _position) const
{
    QTextCursor cursor(m_editor->document());
    cursor.setPosition(boundedPosition(_position));
    if (cursor.atBlockEnd() && cursor.block().length() > 1) {
        cursor.movePosition(QTextCursor::PreviousCharacter);
    }
    return cursor.position();
}

int VimTextEditController::visualSelectionEndPosition(int _position) const
{
    QTextCursor cursor(m_editor->document());
    cursor.setPosition(boundedPosition(_position));
    if (!cursor.atBlockEnd()) {
        cursor.movePosition(QTextCursor::NextCharacter);
    }
    return cursor.position();
}

void VimTextEditController::undo()
{
    if (isVisualMode()) {
        enterNormalMode();
    }
    triggerEditorShortcut(QKeySequence::Undo);
    resetPendingCommand();
}

void VimTextEditController::redo()
{
    if (isVisualMode()) {
        enterNormalMode();
    }
    triggerEditorShortcut(QKeySequence::Redo);
    resetPendingCommand();
}

void VimTextEditController::triggerEditorShortcut(QKeySequence::StandardKey _key)
{
    //
    // Отмена и повтор действия реализованы в самих редакторах, поэтому имитируем нажатие
    // системного сочетания клавиш, чтобы редактор обработал его сам
    //
    const auto keyBindings = QKeySequence::keyBindings(_key);
    if (keyBindings.isEmpty() || keyBindings.constFirst().count() == 0) {
        return;
    }

    const auto keyCombination = keyBindings.constFirst()[0];
    QScopedValueRollback<bool> enabledRollback(m_enabled, false);
    QKeyEvent event(QEvent::KeyPress, keyCombination.key(), keyCombination.keyboardModifiers());
    QGuiApplication::sendEvent(m_editor, &event);
}

void VimTextEditController::deleteCharacters(int _count, bool _previous)
{
    if (!canEdit()) {
        return;
    }

    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    moveToCharacterUnderBlockCursor(cursor);

    if (_previous) {
        for (int index = 0; index < _count && !cursor.atBlockStart(); ++index) {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        }
    } else {
        for (int index = 0; index < _count && !cursor.atBlockEnd(); ++index) {
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        }
    }

    if (cursor.hasSelection()) {
        m_editor->setTextCursor(cursor);
        deleteSelection();
    }
}

void VimTextEditController::deleteToEndOfLine()
{
    if (!canEdit()) {
        return;
    }

    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    moveToCharacterUnderBlockCursor(cursor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    if (!cursor.hasSelection()) {
        return;
    }

    m_editor->setTextCursor(cursor);
    deleteSelection();
}

void VimTextEditController::deleteSelection(bool _yank)
{
    auto cursor = m_editor->textCursor();
    if (!cursor.hasSelection()) {
        if (isVisualMode()) {
            resetVisualSelection();
        }
        return;
    }

    if (_yank) {
        yankSelection();
    }

    if (canEdit()) {
        sendDeleteKeyPressToEditor();
    }
    if (isVisualMode()) {
        resetVisualSelection();
    }
}

void VimTextEditController::changeSelection()
{
    deleteSelection();
    enterInsertMode();
}

void VimTextEditController::yankSelection()
{
    const auto cursor = m_editor->textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    setRegisterText(selectedTextForClipboard(cursor), false);
}

QTextCursor VimTextEditController::linesCursor(int _fromPosition, int _toPosition,
                                               bool _includeSeparator) const
{
    QTextCursor cursor(m_editor->document());
    cursor.setPosition(boundedPosition(qMin(_fromPosition, _toPosition)));
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.setPosition(boundedPosition(qMax(_fromPosition, _toPosition)), QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    //
    // Чтобы строки действительно исчезли, а не превратились в пустые, захватываем ещё и
    // разделитель абзацев - следующий, а для последних строк документа предыдущий
    //
    if (_includeSeparator) {
        if (!cursor.atEnd()) {
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        } else if (cursor.anchor() > 0) {
            const int endPosition = cursor.position();
            cursor.setPosition(cursor.anchor());
            cursor.movePosition(QTextCursor::PreviousCharacter);
            cursor.setPosition(endPosition, QTextCursor::KeepAnchor);
        }
    }

    return cursor;
}

void VimTextEditController::yankLines(int _fromPosition, int _toPosition)
{
    const auto cursor = linesCursor(_fromPosition, _toPosition, false);
    setRegisterText(selectedTextForClipboard(cursor), true);
}

void VimTextEditController::deleteLines(int _fromPosition, int _toPosition, bool _yank)
{
    if (!canEdit()) {
        return;
    }

    if (_yank) {
        yankLines(_fromPosition, _toPosition);
    }

    auto cursor = linesCursor(_fromPosition, _toPosition, true);
    if (!cursor.hasSelection()) {
        return;
    }

    m_editor->setTextCursor(cursor);
    sendDeleteKeyPressToEditor();

    //
    // ... курсор ставим на первый значимый символ оставшейся строки, как это делает vim
    //
    cursor = m_editor->textCursor();
    cursor.clearSelection();
    moveToFirstNonBlank(cursor);
    m_editor->setTextCursor(cursor);
}

void VimTextEditController::changeLines(int _fromPosition, int _toPosition, bool _yank)
{
    if (!canEdit()) {
        return;
    }

    if (_yank) {
        yankLines(_fromPosition, _toPosition);
    }

    //
    // ... в отличие от удаления, сами строки остаются на месте, лишь очищается их содержимое
    //
    auto cursor = linesCursor(_fromPosition, _toPosition, false);
    if (cursor.hasSelection()) {
        m_editor->setTextCursor(cursor);
        sendDeleteKeyPressToEditor();
    } else {
        setCursorPosition(cursor.position());
    }

    enterInsertMode();
}

void VimTextEditController::paste(bool _before, int _count)
{
    bool linewise = false;
    const auto text = registerText(&linewise);
    pasteText(text, linewise, _before, _count);
}

void VimTextEditController::pasteText(const QString& _text, bool _linewise, bool _before,
                                      int _count)
{
    if (!canEdit() || _text.isEmpty()) {
        return;
    }

    auto cursor = m_editor->textCursor();
    cursor.clearSelection();

    if (_linewise) {
        auto lineText = _text;
        while (lineText.endsWith("\n")) {
            lineText.chop(1);
        }

        cursor.movePosition(_before ? QTextCursor::StartOfBlock : QTextCursor::EndOfBlock);
        const int insertPosition = cursor.position();
        cursor.beginEditBlock();
        for (int index = 0; index < _count; ++index) {
            if (_before) {
                cursor.insertText(lineText);
                cursor.insertBlock();
            } else {
                cursor.insertBlock();
                cursor.insertText(lineText);
            }
        }
        cursor.endEditBlock();

        //
        // ... курсор ставим на первый значимый символ первой вставленной строки
        //
        cursor.setPosition(boundedPosition(_before ? insertPosition : insertPosition + 1));
        moveToFirstNonBlank(cursor);
    } else {
        if (!_before && !cursor.atBlockEnd()) {
            cursor.movePosition(QTextCursor::NextCharacter);
        }

        const int insertPosition = cursor.position();
        cursor.beginEditBlock();
        for (int index = 0; index < _count; ++index) {
            cursor.insertText(_text);
        }
        cursor.endEditBlock();

        //
        // ... курсор ставим на последний вставленный символ
        //
        cursor.setPosition(qMax(insertPosition, cursor.position() - 1));
    }

    m_editor->setTextCursor(cursor);
}

void VimTextEditController::replaceSelectionWithRegister()
{
    bool linewise = false;
    const auto text = registerText(&linewise);
    if (text.isEmpty()) {
        enterNormalMode();
        return;
    }

    const int anchorPosition = m_visualAnchorPosition;
    const int cursorPosition = m_visualCursorPosition;
    const bool selectionLinewise = m_mode == Mode::VisualLine;
    leaveVisualMode();

    if (selectionLinewise) {
        deleteLines(anchorPosition, cursorPosition, false);
        pasteText(text, true, true, 1);
    } else {
        deleteSelection(false);
        pasteText(text, linewise, true, 1);
    }
}

void VimTextEditController::replaceCharacters(const QString& _text, int _count)
{
    if (!canEdit() || _text.isEmpty()) {
        return;
    }

    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    moveToCharacterUnderBlockCursor(cursor);

    //
    // ... если в строке не хватает символов, то замена не выполняется вовсе
    //
    auto availableCharactersCursor = cursor;
    for (int index = 0; index < _count; ++index) {
        if (availableCharactersCursor.atBlockEnd()) {
            return;
        }
        availableCharactersCursor.movePosition(QTextCursor::NextCharacter);
    }

    const int startPosition = cursor.position();
    cursor.beginEditBlock();
    for (int index = 0; index < _count; ++index) {
        cursor.deleteChar();
        cursor.insertText(_text);
    }
    cursor.endEditBlock();

    //
    // ... курсор остаётся на последнем заменённом символе
    //
    cursor.setPosition(qMax(startPosition, cursor.position() - 1));
    m_editor->setTextCursor(cursor);
}

void VimTextEditController::openLineBelow()
{
    if (!canEdit()) {
        return;
    }

    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::EndOfBlock);
    m_editor->setTextCursor(cursor);

    sendReturnKeyPressToEditor();
}

void VimTextEditController::openLineAbove()
{
    if (!canEdit()) {
        return;
    }

    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::StartOfBlock);
    m_editor->setTextCursor(cursor);

    sendReturnKeyPressToEditor();

    cursor = m_editor->textCursor();
    if (cursor.positionInBlock() == 0 && cursor.block().previous().isValid()
        && cursor.block().previous().length() == 1) {
        cursor.movePosition(QTextCursor::PreviousBlock);
        cursor.movePosition(QTextCursor::EndOfBlock);
        m_editor->setTextCursor(cursor);
    }
}

bool VimTextEditController::canEdit() const
{
    return !m_editor->isReadOnly();
}

void VimTextEditController::sendDeleteKeyPressToEditor()
{
    QScopedValueRollback<bool> enabledRollback(m_enabled, false);
    QKeyEvent deleteEvent(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
    QGuiApplication::sendEvent(m_editor, &deleteEvent);
}

void VimTextEditController::sendReturnKeyPressToEditor()
{
    QKeyEvent enterEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "\r");
    QScopedValueRollback<Mode> modeRollback(m_mode, Mode::Insert);
    QGuiApplication::sendEvent(m_editor, &enterEvent);
}

QString VimTextEditController::selectedTextForClipboard(const QTextCursor& _cursor) const
{
    QString text = _cursor.selectedText();
    text.replace(QChar::ParagraphSeparator, "\n");
    text.replace(QChar::LineSeparator, "\n");
    return text;
}

void VimTextEditController::setRegisterText(const QString& _text, bool _linewise)
{
    m_registerText = _text;
    //
    // ... построчный фрагмент всегда завершается переводом строки, чтобы при вставке он попадал
    //     на отдельную строку, даже если копировали последнюю строку документа
    //
    if (_linewise && !m_registerText.endsWith("\n")) {
        m_registerText.append("\n");
    }
    m_registerLinewise = _linewise;

    QGuiApplication::clipboard()->setText(m_registerText);
}

QString VimTextEditController::registerText(bool* _linewise) const
{
    const auto clipboardText = QGuiApplication::clipboard()->text();
    const bool useInternalRegister = !m_registerText.isEmpty() && clipboardText == m_registerText;
    if (_linewise != nullptr) {
        *_linewise = useInternalRegister && m_registerLinewise;
    }
    return useInternalRegister ? m_registerText : clipboardText;
}

void VimTextEditController::finishHandledEvent(QKeyEvent* _event)
{
    m_editor->ensureCursorVisible();
    _event->accept();
}

} // namespace Ui
