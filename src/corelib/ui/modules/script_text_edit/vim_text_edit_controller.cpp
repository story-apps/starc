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
    }

    return {};
}

bool VimTextEditController::usesBlockCursor() const
{
    return m_enabled && m_mode != Mode::Insert;
}

int VimTextEditController::blockCursorPosition() const
{
    if (m_mode == Mode::Visual && m_visualCursorPosition >= 0) {
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
        enterNormalMode(m_mode == Mode::Insert);
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

    const bool handled
        = m_mode == Mode::Visual ? handleVisualMode(_event) : handleNormalMode(_event);
    if (handled) {
        finishHandledEvent(_event);
    }
    return handled;
}

void VimTextEditController::resetPendingCommand()
{
    m_pendingOperator = Operator::None;
    m_pendingOperatorCount = 1;
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
    if (m_mode == Mode::Visual && m_visualCursorPosition >= 0) {
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

void VimTextEditController::enterVisualMode()
{
    resetPendingCommand();
    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    m_visualAnchorPosition = visualCursorPosition(cursor.position());
    m_visualCursorPosition = m_visualAnchorPosition;
    m_mode = Mode::Visual;
    updateVisualSelection();
}

void VimTextEditController::resetVisualSelection()
{
    m_visualAnchorPosition = -1;
    m_visualCursorPosition = -1;
}

void VimTextEditController::updateVisualSelection()
{
    if (m_mode != Mode::Visual || m_visualAnchorPosition < 0 || m_visualCursorPosition < 0) {
        return;
    }

    const int anchorPosition = boundedPosition(m_visualAnchorPosition);
    const int cursorPosition = boundedPosition(m_visualCursorPosition);
    const int selectionStart = qMin(anchorPosition, cursorPosition);
    const int selectionEnd = visualSelectionEndPosition(qMax(anchorPosition, cursorPosition));

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

int VimTextEditController::takeCount(int _default)
{
    const auto count = m_count.isEmpty() ? _default : m_count.toInt();
    m_count.clear();
    return qMax(1, count);
}

int VimTextEditController::combinedOperatorCount()
{
    return qMax(1, m_pendingOperatorCount) * takeCount(1);
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
        m_count.append(text);
        return true;
    }

    const auto key = _event->key();
    const auto motion = motionFromKey(_event);
    if (motion != Motion::None) {
        move(motion, takeCount(), QTextCursor::MoveAnchor);
        return true;
    }

    switch (key) {
    case Qt::Key_I: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        if (text == "I") {
            move(Motion::FirstNonBlank, 1, QTextCursor::MoveAnchor);
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
            move(Motion::EndOfLine, 1, QTextCursor::MoveAnchor);
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
        enterVisualMode();
        return true;
    }

    case Qt::Key_U: {
        if (text != "u") {
            resetPendingCommand();
            return true;
        }
        if (!canEditText) {
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
            m_pendingOperatorCount = takeCount();
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
            m_pendingOperatorCount = takeCount();
        }
        return true;
    }

    case Qt::Key_Y: {
        if (text == "Y") {
            yankLines(takeCount());
        } else {
            m_pendingOperator = Operator::Yank;
            m_pendingOperatorCount = takeCount();
        }
        return true;
    }

    case Qt::Key_G: {
        if (text == "G") {
            move(Motion::EndOfDocument, takeCount(), QTextCursor::MoveAnchor);
        } else {
            m_pendingOperator = Operator::Go;
            m_pendingOperatorCount = takeCount();
        }
        return true;
    }

    case Qt::Key_P: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        paste(text == "P");
        return true;
    }

    case Qt::Key_R: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        m_pendingOperator = Operator::Replace;
        m_pendingOperatorCount = takeCount();
        return true;
    }

    case Qt::Key_S: {
        if (!canEditText) {
            resetPendingCommand();
            return true;
        }
        if (text == "S") {
            deleteLines(takeCount());
        } else {
            deleteCharacters(takeCount());
        }
        enterInsertMode();
        return true;
    }

    default: {
        resetPendingCommand();
        return true;
    }
    }
}

bool VimTextEditController::handleVisualMode(QKeyEvent* _event)
{
    const bool canEditText = canEdit();

    if (m_pendingOperator == Operator::Go) {
        if (_event->key() == Qt::Key_G && _event->text() == "g") {
            moveVisual(Motion::StartOfDocument, combinedOperatorCount());
        }
        resetPendingCommand();
        return true;
    }

    const auto text = _event->text();
    if (isDigit(text) && (text != "0" || !m_count.isEmpty())) {
        m_count.append(text);
        return true;
    }

    const auto motion = motionFromKey(_event);
    if (motion != Motion::None) {
        moveVisual(motion, takeCount());
        return true;
    }

    switch (_event->key()) {
    case Qt::Key_V: {
        enterNormalMode();
        return true;
    }

    case Qt::Key_Y: {
        yankSelection(false);
        enterNormalMode();
        return true;
    }

    case Qt::Key_U: {
        if (text == "u") {
            if (canEditText) {
                undo();
            }
        } else {
            resetPendingCommand();
        }
        return true;
    }

    case Qt::Key_G: {
        if (text == "G") {
            moveVisual(Motion::EndOfDocument, takeCount());
        } else {
            m_pendingOperator = Operator::Go;
            m_pendingOperatorCount = takeCount();
        }
        return true;
    }

    case Qt::Key_D:
    case Qt::Key_X: {
        if (!canEditText) {
            enterNormalMode();
            return true;
        }
        deleteSelection();
        enterNormalMode();
        return true;
    }

    case Qt::Key_C:
    case Qt::Key_S: {
        if (!canEditText) {
            enterNormalMode();
            return true;
        }
        changeSelection();
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
        m_count.append(text);
        return true;
    }

    if (m_pendingOperator == Operator::Go) {
        if (_event->key() == Qt::Key_G && text == "g") {
            move(Motion::StartOfDocument, combinedOperatorCount(), QTextCursor::MoveAnchor);
        }
        resetPendingCommand();
        return true;
    }

    if ((m_pendingOperator == Operator::Delete || m_pendingOperator == Operator::Change)
        && !canEdit()) {
        resetPendingCommand();
        return true;
    }

    const bool linewise = (_event->key() == Qt::Key_D && m_pendingOperator == Operator::Delete)
        || (_event->key() == Qt::Key_C && m_pendingOperator == Operator::Change)
        || (_event->key() == Qt::Key_Y && m_pendingOperator == Operator::Yank);

    if (linewise) {
        const auto count = combinedOperatorCount();
        if (m_pendingOperator == Operator::Yank) {
            yankLines(count);
        } else {
            deleteLines(count);
            if (m_pendingOperator == Operator::Change) {
                enterInsertMode();
            }
        }
        resetPendingCommand();
        return true;
    }

    const auto motion = motionFromKey(_event);
    if (motion == Motion::None) {
        resetPendingCommand();
        return true;
    }

    const auto count = combinedOperatorCount();
    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    if (moveCursor(cursor, motion, count) && cursor.position() != m_editor->textCursor().position()) {
        const auto sourcePosition = m_editor->textCursor().position();
        const auto targetPosition = cursor.position();
        cursor.setPosition(sourcePosition);
        cursor.setPosition(targetPosition, QTextCursor::KeepAnchor);
        m_editor->setTextCursor(cursor);

        if (m_pendingOperator == Operator::Yank) {
            yankSelection(false);
            cursor.clearSelection();
            m_editor->setTextCursor(cursor);
        } else if (m_pendingOperator == Operator::Delete) {
            deleteSelection();
        } else if (m_pendingOperator == Operator::Change) {
            changeSelection();
        }
    }

    resetPendingCommand();
    return true;
}

bool VimTextEditController::handlePendingReplace(QKeyEvent* _event)
{
    if (canEdit() && !_event->text().isEmpty()) {
        replaceCharacters(_event->text(), m_pendingOperatorCount);
    }
    resetPendingCommand();
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

bool VimTextEditController::move(Motion _motion, int _count, QTextCursor::MoveMode _mode)
{
    auto cursor = m_editor->textCursor();
    if (_mode == QTextCursor::MoveAnchor) {
        cursor.clearSelection();
    }

    const bool moved = moveCursor(cursor, _motion, _count);
    if (moved) {
        if (_mode == QTextCursor::KeepAnchor) {
            const auto position = cursor.position();
            auto selectionCursor = m_editor->textCursor();
            selectionCursor.setPosition(position, QTextCursor::KeepAnchor);
            cursor = selectionCursor;
        }
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
    bool moved = false;
    const auto moveMode = QTextCursor::MoveAnchor;
    for (int index = 0; index < qMax(1, _count); ++index) {
        switch (_motion) {
        case Motion::Left:
            moved = moveLeft(_cursor) || moved;
            break;
        case Motion::Down:
            moved = _cursor.movePosition(QTextCursor::Down, moveMode) || moved;
            break;
        case Motion::Up:
            moved = _cursor.movePosition(QTextCursor::Up, moveMode) || moved;
            break;
        case Motion::Right:
            moved = moveRight(_cursor) || moved;
            break;
        case Motion::StartOfLine:
            _cursor.movePosition(QTextCursor::StartOfBlock, moveMode);
            moved = true;
            break;
        case Motion::FirstNonBlank:
            moveToFirstNonBlank(_cursor);
            moved = true;
            break;
        case Motion::EndOfLine:
            _cursor.movePosition(QTextCursor::EndOfBlock, moveMode);
            moved = true;
            break;
        case Motion::NextWord:
            moved = _cursor.movePosition(QTextCursor::NextWord, moveMode) || moved;
            break;
        case Motion::PreviousWord:
            moved = _cursor.movePosition(QTextCursor::PreviousWord, moveMode) || moved;
            break;
        case Motion::EndOfWord:
            moved = _cursor.movePosition(QTextCursor::EndOfWord, moveMode) || moved;
            break;
        case Motion::StartOfDocument:
            _cursor.movePosition(QTextCursor::Start, moveMode);
            moved = true;
            break;
        case Motion::EndOfDocument:
            _cursor.movePosition(QTextCursor::End, moveMode);
            moved = true;
            break;
        case Motion::None:
            return false;
        }
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
    if (m_mode == Mode::Visual) {
        enterNormalMode();
    }
    triggerUndoShortcut();
    resetPendingCommand();
}

void VimTextEditController::redo()
{
    if (m_mode == Mode::Visual) {
        enterNormalMode();
    }
    triggerRedoShortcut();
    resetPendingCommand();
}

void VimTextEditController::triggerUndoShortcut()
{
    QScopedValueRollback<bool> enabledRollback(m_enabled, false);
    QKeyEvent undoEvent(QEvent::KeyPress, Qt::Key_Z, Qt::ControlModifier);
    QGuiApplication::sendEvent(m_editor, &undoEvent);
}

void VimTextEditController::triggerRedoShortcut()
{
    QScopedValueRollback<bool> enabledRollback(m_enabled, false);
    QKeyEvent redoEvent(QEvent::KeyPress, Qt::Key_Y, Qt::ControlModifier);
    QGuiApplication::sendEvent(m_editor, &redoEvent);
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

void VimTextEditController::deleteLines(int _count)
{
    if (!canEdit()) {
        return;
    }

    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::StartOfBlock);

    for (int index = 0; index < _count; ++index) {
        if (!cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor)) {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            break;
        }
    }

    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::BlockUnderCursor);
    }

    m_editor->setTextCursor(cursor);
    deleteSelection(true, true);
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
    m_editor->setTextCursor(cursor);
    deleteSelection();
}

void VimTextEditController::deleteSelection(bool _yank, bool _linewise)
{
    auto cursor = m_editor->textCursor();
    if (!cursor.hasSelection()) {
        if (m_mode == Mode::Visual) {
            resetVisualSelection();
        }
        return;
    }

    if (_yank) {
        yankSelection(_linewise);
    }

    if (canEdit()) {
        sendDeleteKeyPressToEditor();
    }
    if (m_mode == Mode::Visual) {
        resetVisualSelection();
    }
}

void VimTextEditController::changeSelection()
{
    deleteSelection();
    enterInsertMode();
}

void VimTextEditController::yankSelection(bool _linewise)
{
    const auto cursor = m_editor->textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    setRegisterText(selectedTextForClipboard(cursor), _linewise);
}

void VimTextEditController::yankLines(int _count)
{
    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::StartOfBlock);
    for (int index = 0; index < _count; ++index) {
        if (!cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor)) {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            break;
        }
    }
    setRegisterText(selectedTextForClipboard(cursor), true);
}

void VimTextEditController::paste(bool _before)
{
    if (!canEdit()) {
        return;
    }

    bool linewise = false;
    const auto text = registerText(&linewise);
    if (text.isEmpty()) {
        return;
    }

    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    cursor.beginEditBlock();
    if (linewise) {
        auto lineText = text;
        if (lineText.endsWith("\n")) {
            lineText.chop(1);
        }
        cursor.movePosition(_before ? QTextCursor::StartOfBlock : QTextCursor::EndOfBlock);
        if (!_before) {
            cursor.insertBlock();
        }
        cursor.insertText(lineText);
        if (_before) {
            cursor.insertBlock();
            cursor.movePosition(QTextCursor::PreviousBlock);
        }
    } else {
        if (!_before && !cursor.atBlockEnd()) {
            cursor.movePosition(QTextCursor::NextCharacter);
        }
        cursor.insertText(text);
    }
    cursor.endEditBlock();
    m_editor->setTextCursor(cursor);
}

void VimTextEditController::replaceCharacters(const QString& _text, int _count)
{
    if (!canEdit() || _text.isEmpty()) {
        return;
    }

    auto cursor = m_editor->textCursor();
    cursor.clearSelection();
    moveToCharacterUnderBlockCursor(cursor);
    cursor.beginEditBlock();
    for (int index = 0; index < _count && !cursor.atBlockEnd(); ++index) {
        cursor.deleteChar();
        cursor.insertText(_text);
    }
    cursor.endEditBlock();
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
    m_registerLinewise = _linewise;
    QGuiApplication::clipboard()->setText(_text);
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
