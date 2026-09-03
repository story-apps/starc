#include "vim_text_edit_controller.h"

#include "script_text_edit.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QRegularExpression>
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
 * @brief Максимальная длина записываемого макроса
 */
const int kMaxRecordedKeys = 10000;

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

bool isEnter(QKeyEvent* _event)
{
    return _event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter;
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
    closeCommandLine();
    m_recordingRegister = { };
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
        return QStringLiteral("recording @%1").arg(m_recordingRegister);
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
        return visualCursorPosition(m_visualCursorPosition);
    }

    return visualCursorPosition(m_editor->textCursor().position());
}

bool VimTextEditController::handleShortcutOverride(QKeyEvent* _event) const
{
    if (!m_enabled || _event == nullptr) {
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

    if (!canEdit()) {
        return false;
    }

    if (_event->matches(QKeySequence::Undo) || _event->matches(QKeySequence::Redo)) {
        return true;
    }

    if (m_mode == Mode::Insert || !_event->modifiers().testFlag(Qt::ControlModifier)) {
        return false;
    }

    return _event->key() == Qt::Key_R || _event->key() == Qt::Key_D || _event->key() == Qt::Key_U;
}

bool VimTextEditController::handleKeyPress(QKeyEvent* _event)
{
    if (!m_enabled || _event == nullptr) {
        return false;
    }

    const RecordedKey recordedKey{ _event->key(), _event->modifiers(), _event->text() };
    const bool wasInsertMode = m_mode == Mode::Insert && !m_commandLineActive;
    const int revisionBefore = m_editor->document()->revision();

    //
    // ... команды, которые сами по себе не являются изменением текста, отменяют его запоминание,
    //     а чтобы вложенные воспроизведения макросов не сбивали флаг, он живёт на стеке
    //
    bool trackChange = true;
    QScopedValueRollback<bool*> trackChangeRollback(m_trackChangeTarget, &trackChange);

    const bool handled = handleKeyPressImpl(_event);

    trackKeyPress(recordedKey, wasInsertMode, revisionBefore, trackChange);

    return handled;
}

bool VimTextEditController::handleKeyPressImpl(QKeyEvent* _event)
{
    if (isEscape(_event)) {
        //
        // Сперва закрываем строку ввода команды и подстановщика, если они показаны, и только
        // следующим нажатием выходим из режима вставки
        //
        if (m_commandLineActive) {
            closeCommandLine();
        } else if (m_editor->isCompleterVisible()) {
            m_editor->closeCompleter();
        } else {
            enterNormalMode(m_mode == Mode::Insert);
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
        return false;
    }

    if (_event->modifiers().testFlag(Qt::ControlModifier)) {
        switch (_event->key()) {
        case Qt::Key_R: {
            if (canEdit()) {
                redo();
            }
            finishHandledEvent(_event);
            return true;
        }
        case Qt::Key_D: {
            scrollPages(0.5);
            finishHandledEvent(_event);
            return true;
        }
        case Qt::Key_U: {
            scrollPages(-0.5);
            finishHandledEvent(_event);
            return true;
        }
        default: {
            break;
        }
        }
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

bool VimTextEditController::hasPendingInput() const
{
    return m_awaiting != Awaiting::None || m_pendingOperator != Operator::None || !m_count.isEmpty()
        || m_commandLineActive;
}

void VimTextEditController::resetPendingCommand()
{
    m_awaiting = Awaiting::None;
    m_pendingOperator = Operator::None;
    m_operatorCount = 0;
    m_count.clear();
    m_pendingRegister = { };
    m_textObjectAround = false;
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

    leaveVisualMode();
}

void VimTextEditController::enterVisualMode(Mode _mode)
{
    resetPendingCommand();

    //
    // ... при переключении между VISUAL и VISUAL LINE выделение сохраняется
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

void VimTextEditController::restoreLastVisualSelection()
{
    if (m_lastVisualAnchorPosition < 0 || m_lastVisualCursorPosition < 0) {
        return;
    }

    resetPendingCommand();
    m_visualAnchorPosition = boundedPosition(m_lastVisualAnchorPosition);
    m_visualCursorPosition = boundedPosition(m_lastVisualCursorPosition);
    m_mode = m_lastVisualMode;
    updateVisualSelection();
}

void VimTextEditController::resetVisualSelection()
{
    //
    // ... запоминаем выделение, чтобы его можно было восстановить командой gv
    //
    if (m_visualAnchorPosition >= 0 && m_visualCursorPosition >= 0) {
        m_lastVisualAnchorPosition = m_visualAnchorPosition;
        m_lastVisualCursorPosition = m_visualCursorPosition;
        m_lastVisualMode = isVisualMode() ? m_mode : m_lastVisualMode;
    }

    m_visualAnchorPosition = -1;
    m_visualCursorPosition = -1;
}

void VimTextEditController::updateVisualSelection()
{
    const auto range = visualRange();
    if (!range.isValid()) {
        return;
    }

    QTextCursor cursor(m_editor->document());
    if (boundedPosition(m_visualCursorPosition) < boundedPosition(m_visualAnchorPosition)) {
        cursor.setPosition(range.end);
        cursor.setPosition(range.start, QTextCursor::KeepAnchor);
    } else {
        cursor.setPosition(range.start);
        cursor.setPosition(range.end, QTextCursor::KeepAnchor);
    }
    m_editor->setTextCursor(cursor);
}

Vim::TextRange VimTextEditController::visualRange() const
{
    if (!isVisualMode() || m_visualAnchorPosition < 0 || m_visualCursorPosition < 0) {
        return {};
    }

    const int anchorPosition = boundedPosition(m_visualAnchorPosition);
    const int cursorPosition = boundedPosition(m_visualCursorPosition);
    if (m_mode == Mode::VisualLine) {
        return linesRange(anchorPosition, cursorPosition);
    }

    return { qMin(anchorPosition, cursorPosition),
             visualSelectionEndPosition(qMax(anchorPosition, cursorPosition)), false };
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
    return qBound(1, qMax(1, m_operatorCount) * takeCount(1), kMaxCount);
}

bool VimTextEditController::handleNormalMode(QKeyEvent* _event)
{
    if (m_awaiting != Awaiting::None) {
        return handleAwaitingInput(_event);
    }

    const auto text = _event->text();
    if (isDigit(text) && (text != "0" || !m_count.isEmpty())) {
        appendCount(text);
        return true;
    }

    if (handleCommonMotion(_event) || handleCommonCommand(_event)) {
        return true;
    }

    const auto motion = motionFromKey(_event);
    if (motion != Motion::None) {
        applyMotion(motionTargetPosition(motion, currentPosition(), takeCount(), false),
                    isInclusiveMotion(motion), isLinewiseMotion(motion));
        return true;
    }

    const bool canEditText = canEdit();
    if (!canEditText) {
        switch (_event->key()) {
        case Qt::Key_I:
        case Qt::Key_A:
        case Qt::Key_O:
        case Qt::Key_X:
        case Qt::Key_D:
        case Qt::Key_C:
        case Qt::Key_S:
        case Qt::Key_P:
        case Qt::Key_R:
        case Qt::Key_J:
        case Qt::Key_U:
        case Qt::Key_AsciiTilde:
        case Qt::Key_Period: {
            resetPendingCommand();
            return true;
        }
        default: {
            break;
        }
        }
    }

    switch (_event->key()) {
    case Qt::Key_I: {
        if (text == "I") {
            setCursorPosition(Vim::firstNonBlankPosition(m_editor->document(), currentPosition()));
        }
        enterInsertMode();
        return true;
    }

    case Qt::Key_A: {
        if (text == "A") {
            setCursorPosition(Vim::endOfLinePosition(m_editor->document(), currentPosition()));
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
        if (text != "u") {
            resetPendingCommand();
            return true;
        }
        undo();
        return true;
    }

    case Qt::Key_X: {
        const auto registerName = m_pendingRegister;
        const int count = takeCount();
        const bool previous = text == "X";
        resetPendingCommand();
        deleteCharacters(count, previous, registerName);
        return true;
    }

    case Qt::Key_D: {
        if (text == "D") {
            const auto registerName = m_pendingRegister;
            resetPendingCommand();
            deleteToEndOfLine(false, registerName);
        } else {
            m_pendingOperator = Operator::Delete;
            m_operatorCount = takeOptionalCount();
            m_awaiting = Awaiting::Motion;
        }
        return true;
    }

    case Qt::Key_C: {
        if (text == "C") {
            const auto registerName = m_pendingRegister;
            resetPendingCommand();
            deleteToEndOfLine(true, registerName);
        } else {
            m_pendingOperator = Operator::Change;
            m_operatorCount = takeOptionalCount();
            m_awaiting = Awaiting::Motion;
        }
        return true;
    }

    case Qt::Key_Y: {
        if (text == "Y") {
            const auto registerName = m_pendingRegister;
            const int fromPosition = currentPosition();
            const int count = takeCount();
            resetPendingCommand();
            yankRange(linesRange(fromPosition, lastLinePosition(fromPosition, count)),
                      registerName);
        } else {
            m_pendingOperator = Operator::Yank;
            m_operatorCount = takeOptionalCount();
            m_awaiting = Awaiting::Motion;
        }
        return true;
    }

    case Qt::Key_P: {
        const auto registerName = m_pendingRegister;
        const bool before = text == "P";
        const int count = takeCount();
        resetPendingCommand();
        paste(before, count, registerName);
        return true;
    }

    case Qt::Key_R: {
        m_operatorCount = takeOptionalCount();
        m_awaiting = Awaiting::ReplaceCharacter;
        return true;
    }

    case Qt::Key_S: {
        const auto registerName = m_pendingRegister;
        const int fromPosition = currentPosition();
        const int count = takeCount();
        const bool linewise = text == "S";
        resetPendingCommand();
        if (linewise) {
            changeRange(linesRange(fromPosition, lastLinePosition(fromPosition, count)),
                        registerName);
        } else {
            const int lineEnd = Vim::endOfLinePosition(m_editor->document(), fromPosition);
            changeRange({ fromPosition, qMin(lineEnd, fromPosition + count), false }, registerName);
        }
        return true;
    }

    case Qt::Key_J: {
        if (text != "J") {
            resetPendingCommand();
            return true;
        }
        const int count = takeCount();
        resetPendingCommand();
        joinLines(count, true);
        return true;
    }

    case Qt::Key_AsciiTilde: {
        const int count = takeCount();
        resetPendingCommand();
        toggleCaseUnderCursor(count);
        return true;
    }

    case Qt::Key_Period: {
        const int count = takeCount();
        resetPendingCommand();
        repeatLastChange(count);
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
    if (m_awaiting != Awaiting::None) {
        return handleAwaitingInput(_event);
    }

    const auto text = _event->text();
    if (isDigit(text) && (text != "0" || !m_count.isEmpty())) {
        appendCount(text);
        return true;
    }

    if (handleCommonMotion(_event) || handleCommonCommand(_event)) {
        return true;
    }

    const auto motion = motionFromKey(_event);
    if (motion != Motion::None) {
        goToPosition(motionTargetPosition(motion, currentPosition(), takeCount(), false));
        resetPendingCommand();
        return true;
    }

    //
    // ... текстовые объекты задают выделение целиком
    //
    if (text == "i" || text == "a") {
        m_textObjectAround = text == "a";
        m_awaiting = Awaiting::TextObject;
        return true;
    }

    const bool canEditText = canEdit();
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

    case Qt::Key_O: {
        //
        // ... меняем местами концы выделения
        //
        std::swap(m_visualAnchorPosition, m_visualCursorPosition);
        resetPendingCommand();
        updateVisualSelection();
        return true;
    }

    case Qt::Key_Y: {
        const auto registerName = m_pendingRegister;
        const auto range = visualRange();
        leaveVisualMode();
        yankRange(range, registerName);
        return true;
    }

    case Qt::Key_D:
    case Qt::Key_X: {
        if (!canEditText) {
            enterNormalMode();
            return true;
        }
        const auto registerName = m_pendingRegister;
        const auto range = visualRange();
        leaveVisualMode();
        deleteRange(range, registerName);
        return true;
    }

    case Qt::Key_C:
    case Qt::Key_S: {
        if (!canEditText) {
            enterNormalMode();
            return true;
        }
        const auto registerName = m_pendingRegister;
        const auto range = visualRange();
        leaveVisualMode();
        changeRange(range, registerName);
        return true;
    }

    case Qt::Key_P: {
        if (!canEditText) {
            enterNormalMode();
            return true;
        }
        const auto registerName = m_pendingRegister;
        resetPendingCommand();
        replaceSelectionWithRegister(registerName);
        return true;
    }

    case Qt::Key_R: {
        if (!canEditText) {
            enterNormalMode();
            return true;
        }
        m_awaiting = Awaiting::ReplaceCharacter;
        return true;
    }

    case Qt::Key_U: {
        if (!canEditText) {
            enterNormalMode();
            return true;
        }
        const auto range = visualRange();
        const auto caseOperator = text == "U" ? Operator::Uppercase : Operator::Lowercase;
        leaveVisualMode();
        changeCaseRange(range, caseOperator);
        return true;
    }

    case Qt::Key_AsciiTilde: {
        if (!canEditText) {
            enterNormalMode();
            return true;
        }
        const auto range = visualRange();
        leaveVisualMode();
        changeCaseRange(range, Operator::SwapCase);
        return true;
    }

    case Qt::Key_J: {
        if (!canEditText || text != "J") {
            enterNormalMode();
            return true;
        }
        const auto range = visualRange();
        const int lines = m_editor->document()->findBlock(range.end).blockNumber()
            - m_editor->document()->findBlock(range.start).blockNumber();
        leaveVisualMode();
        setCursorPosition(range.start);
        joinLines(qMax(1, lines), true);
        return true;
    }

    default: {
        resetPendingCommand();
        return true;
    }
    }
}

bool VimTextEditController::handleCommonMotion(QKeyEvent* _event)
{
    const auto text = _event->text();
    auto document = m_editor->document();

    switch (_event->key()) {
    case Qt::Key_F:
    case Qt::Key_T: {
        m_findForward = text == "f" || text == "t";
        m_findTill = text == "t" || text == "T";
        m_awaiting = Awaiting::FindCharacter;
        return true;
    }

    case Qt::Key_Semicolon:
    case Qt::Key_Comma: {
        if (m_findCharacter.isNull()) {
            resetPendingCommand();
            return true;
        }

        const bool forward = text == ";" ? m_findForward : !m_findForward;
        const int count = combinedOperatorCount();
        int fromPosition = currentPosition();
        //
        // ... при повторе поиска до символа нужно перешагнуть через него, иначе курсор застрянет
        //
        if (m_findTill) {
            fromPosition = qBound(Vim::startOfLinePosition(document, fromPosition),
                                  fromPosition + (forward ? 1 : -1),
                                  Vim::endOfLinePosition(document, fromPosition));
        }
        applyMotion(Vim::findCharacterPosition(document, fromPosition, m_findCharacter, forward,
                                               m_findTill, count),
                    forward, false);
        return true;
    }

    case Qt::Key_QuoteLeft:
    case Qt::Key_Apostrophe: {
        m_jumpToMarkExact = _event->key() == Qt::Key_QuoteLeft;
        m_awaiting = Awaiting::JumpToMark;
        return true;
    }

    case Qt::Key_N: {
        searchNext(text == "N", takeCount());
        return true;
    }

    case Qt::Key_Asterisk:
    case Qt::Key_NumberSign: {
        searchWordUnderCursor(_event->key() == Qt::Key_Asterisk);
        return true;
    }

    case Qt::Key_G: {
        if (text != "G") {
            m_awaiting = Awaiting::GoCommand;
            return true;
        }

        int lineNumber = takeOptionalCount();
        if (lineNumber <= 0) {
            lineNumber = m_operatorCount;
        }
        rememberJumpPosition();
        applyMotion(
            Vim::linePosition(document, lineNumber > 0 ? lineNumber : document->blockCount()),
            false, m_pendingOperator != Operator::None);
        return true;
    }

    default: {
        return false;
    }
    }
}

bool VimTextEditController::handleCommonCommand(QKeyEvent* _event)
{
    const auto text = _event->text();
    switch (_event->key()) {
    case Qt::Key_QuoteDbl: {
        m_awaiting = Awaiting::RegisterName;
        return true;
    }

    case Qt::Key_M: {
        m_awaiting = Awaiting::SetMark;
        return true;
    }

    case Qt::Key_Q: {
        if (!m_recordingRegister.isNull()) {
            toggleMacroRecording(m_recordingRegister);
        } else {
            m_awaiting = Awaiting::RecordRegister;
        }
        return true;
    }

    case Qt::Key_At: {
        m_awaiting = Awaiting::PlayRegister;
        return true;
    }

    case Qt::Key_Z: {
        m_awaiting = Awaiting::ScrollCommand;
        return true;
    }

    case Qt::Key_Slash:
    case Qt::Key_Question: {
        openCommandLine(text == "?" ? QStringLiteral("?") : QStringLiteral("/"));
        return true;
    }

    case Qt::Key_Colon: {
        openCommandLine(QStringLiteral(":"));
        return true;
    }

    default: {
        return false;
    }
    }
}

bool VimTextEditController::handleAwaitingInput(QKeyEvent* _event)
{
    const auto text = _event->text();
    switch (m_awaiting) {
    case Awaiting::Motion: {
        return handleMotionTarget(_event);
    }

    case Awaiting::TextObject: {
        return handleTextObject(_event);
    }

    case Awaiting::FindCharacter: {
        return handleFindCharacter(_event);
    }

    case Awaiting::ReplaceCharacter: {
        return handleReplaceCharacter(_event);
    }

    case Awaiting::GoCommand: {
        return handleGoCommand(_event);
    }

    case Awaiting::ScrollCommand: {
        return handleScrollCommand(_event);
    }

    case Awaiting::SetMark: {
        m_awaiting = Awaiting::None;
        if (!text.isEmpty() && text.at(0).isLetterOrNumber()) {
            setMark(text.at(0));
        }
        resetPendingCommand();
        return true;
    }

    case Awaiting::JumpToMark: {
        m_awaiting = Awaiting::None;
        if (text.isEmpty()) {
            resetPendingCommand();
            return true;
        }
        jumpToMark(text.at(0), m_jumpToMarkExact);
        return true;
    }

    case Awaiting::RegisterName: {
        m_awaiting = Awaiting::None;
        if (text.isEmpty() || !Vim::Registers::isValidName(text.at(0))) {
            resetPendingCommand();
            return true;
        }
        m_pendingRegister = text.at(0);
        return true;
    }

    case Awaiting::RecordRegister: {
        m_awaiting = Awaiting::None;
        if (!text.isEmpty() && text.at(0).isLetter()) {
            toggleMacroRecording(text.at(0));
        }
        resetPendingCommand();
        return true;
    }

    case Awaiting::PlayRegister: {
        m_awaiting = Awaiting::None;
        const int count = takeCount();
        const auto registerName
            = text == "@" ? m_lastPlayedRegister : (text.isEmpty() ? QChar() : text.at(0));
        resetPendingCommand();
        if (!registerName.isNull()) {
            playMacro(registerName, count);
        }
        return true;
    }

    default: {
        resetPendingCommand();
        return true;
    }
    }
}

bool VimTextEditController::handleMotionTarget(QKeyEvent* _event)
{
    const auto text = _event->text();
    if (isDigit(text) && (text != "0" || !m_count.isEmpty())) {
        appendCount(text);
        return true;
    }

    //
    // ... удвоенный оператор работает построчно: dd, cc, yy, guu, gUU, g~~
    //
    const bool doubledOperator
        = (_event->key() == Qt::Key_D && m_pendingOperator == Operator::Delete)
        || (_event->key() == Qt::Key_C && m_pendingOperator == Operator::Change)
        || (_event->key() == Qt::Key_Y && m_pendingOperator == Operator::Yank)
        || (_event->key() == Qt::Key_U && m_pendingOperator == Operator::Lowercase)
        || (_event->key() == Qt::Key_U && m_pendingOperator == Operator::Uppercase)
        || (_event->key() == Qt::Key_AsciiTilde && m_pendingOperator == Operator::SwapCase);
    if (doubledOperator) {
        const auto pendingOperator = m_pendingOperator;
        const int fromPosition = currentPosition();
        const int count = combinedOperatorCount();
        applyOperator(pendingOperator,
                      linesRange(fromPosition, lastLinePosition(fromPosition, count)));
        return true;
    }

    //
    // ... текстовые объекты
    //
    if (text == "i" || text == "a") {
        m_textObjectAround = text == "a";
        m_awaiting = Awaiting::TextObject;
        return true;
    }

    m_awaiting = Awaiting::None;
    if (handleCommonMotion(_event)) {
        return true;
    }

    auto motion = motionFromKey(_event);
    if (motion == Motion::None) {
        resetPendingCommand();
        return true;
    }

    //
    // ... cw и cW в vim работают как ce и cE, т.е. не захватывают пробелы за словом
    //
    bool inclusive = isInclusiveMotion(motion);
    const int position = currentPosition();
    if (m_pendingOperator == Operator::Change
        && (motion == Motion::NextWord || motion == Motion::NextBigWord)
        && !m_editor->document()->characterAt(position).isSpace()) {
        motion = motion == Motion::NextWord ? Motion::EndOfWord : Motion::EndOfBigWord;
        inclusive = true;
    }

    applyMotion(motionTargetPosition(motion, position, combinedOperatorCount(), true), inclusive,
                isLinewiseMotion(motion));
    return true;
}

bool VimTextEditController::handleTextObject(QKeyEvent* _event)
{
    const auto text = _event->text();
    const bool around = m_textObjectAround;
    const int position = currentPosition();
    auto document = m_editor->document();
    m_awaiting = Awaiting::None;

    Vim::TextRange range;
    if (text == "w") {
        range = Vim::wordRange(document, position, false, around);
    } else if (text == "W") {
        range = Vim::wordRange(document, position, true, around);
    } else if (text == "p") {
        range = Vim::paragraphRange(document, position, around);
    } else if (text == "(" || text == ")" || text == "b") {
        range = Vim::blockRange(document, position, '(', ')', around);
    } else if (text == "[" || text == "]") {
        range = Vim::blockRange(document, position, '[', ']', around);
    } else if (text == "{" || text == "}" || text == "B") {
        range = Vim::blockRange(document, position, '{', '}', around);
    } else if (text == "<" || text == ">") {
        range = Vim::blockRange(document, position, '<', '>', around);
    } else if (text == "\"" || text == "'" || text == "`") {
        range = Vim::quoteRange(document, position, text.at(0), around);
    }

    if (!range.isValid()) {
        resetPendingCommand();
        return true;
    }

    //
    // ... в визуальном режиме объект просто становится выделением
    //
    if (isVisualMode() && m_pendingOperator == Operator::None) {
        resetPendingCommand();
        m_visualAnchorPosition = range.start;
        m_visualCursorPosition = visualCursorPosition(qMax(range.start, range.end - 1));
        if (range.linewise) {
            m_mode = Mode::VisualLine;
        }
        updateVisualSelection();
        return true;
    }

    applyOperator(m_pendingOperator, range);
    return true;
}

bool VimTextEditController::handleFindCharacter(QKeyEvent* _event)
{
    const auto text = _event->text();
    m_awaiting = Awaiting::None;

    if (text.isEmpty() || !text.at(0).isPrint()) {
        resetPendingCommand();
        return true;
    }

    m_findCharacter = text.at(0);
    const int count = combinedOperatorCount();
    applyMotion(Vim::findCharacterPosition(m_editor->document(), currentPosition(), m_findCharacter,
                                           m_findForward, m_findTill, count),
                m_findForward, false);
    return true;
}

bool VimTextEditController::handleReplaceCharacter(QKeyEvent* _event)
{
    const auto text = _event->text();
    const bool visual = isVisualMode();
    const auto range = visual ? visualRange() : Vim::TextRange{};
    const int count = combinedOperatorCount();
    m_awaiting = Awaiting::None;
    if (visual) {
        leaveVisualMode();
    } else {
        resetPendingCommand();
    }

    if (!canEdit() || text.isEmpty() || !text.at(0).isPrint()) {
        return true;
    }

    if (visual) {
        replaceRangeCharacters(range, text);
    } else {
        replaceCharacters(text, count);
    }
    return true;
}

bool VimTextEditController::handleGoCommand(QKeyEvent* _event)
{
    const auto text = _event->text();
    const auto pendingOperator = m_pendingOperator;
    m_awaiting = Awaiting::None;

    //
    // ... переход к строке по её номеру
    //
    if (_event->key() == Qt::Key_G && text == "g") {
        int lineNumber = takeOptionalCount();
        if (lineNumber <= 0) {
            lineNumber = m_operatorCount;
        }
        rememberJumpPosition();
        applyMotion(Vim::linePosition(m_editor->document(), lineNumber > 0 ? lineNumber : 1), false,
                    pendingOperator != Operator::None);
        return true;
    }

    if (pendingOperator != Operator::None) {
        //
        // ... остальные команды не годятся в качестве цели для оператора
        //
        resetPendingCommand();
        return true;
    }

    const bool caseCommand = text == "u" || text == "U" || text == "~";
    const auto caseOperator = text == "u"
        ? Operator::Lowercase
        : (text == "U" ? Operator::Uppercase : Operator::SwapCase);

    if (isVisualMode()) {
        if (caseCommand) {
            if (!canEdit()) {
                enterNormalMode();
                return true;
            }
            const auto range = visualRange();
            leaveVisualMode();
            changeCaseRange(range, caseOperator);
            return true;
        }
        if (text == "J") {
            if (!canEdit()) {
                enterNormalMode();
                return true;
            }
            const auto range = visualRange();
            auto document = m_editor->document();
            const int lines = document->findBlock(range.end).blockNumber()
                - document->findBlock(range.start).blockNumber();
            leaveVisualMode();
            setCursorPosition(range.start);
            joinLines(qMax(1, lines), false);
            return true;
        }
        resetPendingCommand();
        return true;
    }

    if (text == "v") {
        restoreLastVisualSelection();
        return true;
    }

    if (!canEdit()) {
        resetPendingCommand();
        return true;
    }

    if (caseCommand) {
        m_pendingOperator = caseOperator;
        m_operatorCount = takeOptionalCount();
        m_awaiting = Awaiting::Motion;
        return true;
    }

    if (text == "J") {
        const int count = takeCount();
        resetPendingCommand();
        joinLines(count, false);
        return true;
    }

    resetPendingCommand();
    return true;
}

bool VimTextEditController::handleScrollCommand(QKeyEvent* _event)
{
    const auto text = _event->text();
    m_awaiting = Awaiting::None;
    resetPendingCommand();

    if (text == "z") {
        scrollCursorTo(ScrollAlignment::Center);
    } else if (text == "t") {
        scrollCursorTo(ScrollAlignment::Top);
    } else if (text == "b") {
        scrollCursorTo(ScrollAlignment::Bottom);
    }
    return true;
}

VimTextEditController::Motion VimTextEditController::motionFromKey(QKeyEvent* _event) const
{
    const auto text = _event->text();
    switch (_event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Backspace:
        return Motion::Left;
    case Qt::Key_Down:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return Motion::Down;
    case Qt::Key_Up:
        return Motion::Up;
    case Qt::Key_Right:
    case Qt::Key_Space:
        return Motion::Right;
    case Qt::Key_Home:
        return Motion::StartOfLine;
    case Qt::Key_End:
        return Motion::EndOfLine;
    case Qt::Key_H:
        return text == "h" ? Motion::Left : Motion::None;
    case Qt::Key_J:
        return text == "j" ? Motion::Down : Motion::None;
    case Qt::Key_K:
        return text == "k" ? Motion::Up : Motion::None;
    case Qt::Key_L:
        return text == "l" ? Motion::Right : Motion::None;
    case Qt::Key_0:
        return Motion::StartOfLine;
    case Qt::Key_AsciiCircum:
        return Motion::FirstNonBlank;
    case Qt::Key_Dollar:
        return Motion::EndOfLine;
    case Qt::Key_W:
        return text == "W" ? Motion::NextBigWord : Motion::NextWord;
    case Qt::Key_B:
        return text == "B" ? Motion::PreviousBigWord : Motion::PreviousWord;
    case Qt::Key_E:
        return text == "E" ? Motion::EndOfBigWord : Motion::EndOfWord;
    case Qt::Key_BraceRight:
        return Motion::NextParagraph;
    case Qt::Key_BraceLeft:
        return Motion::PreviousParagraph;
    case Qt::Key_Percent:
        return Motion::MatchingBracket;
    default:
        return Motion::None;
    }
}

bool VimTextEditController::isInclusiveMotion(Motion _motion) const
{
    return _motion == Motion::EndOfWord || _motion == Motion::EndOfBigWord
        || _motion == Motion::MatchingBracket;
}

bool VimTextEditController::isLinewiseMotion(Motion _motion) const
{
    return _motion == Motion::Down || _motion == Motion::Up;
}

int VimTextEditController::motionTargetPosition(Motion _motion, int _fromPosition, int _count,
                                                bool _forOperator) const
{
    auto document = m_editor->document();
    const int count = qMax(1, _count);
    int position = boundedPosition(_fromPosition);

    switch (_motion) {
    case Motion::None: {
        return -1;
    }
    case Motion::StartOfLine: {
        return Vim::startOfLinePosition(document, position);
    }
    case Motion::FirstNonBlank: {
        return Vim::firstNonBlankPosition(document, position);
    }
    case Motion::EndOfLine: {
        return Vim::endOfLinePosition(document, position);
    }
    case Motion::MatchingBracket: {
        return Vim::matchingBracketPosition(document, position);
    }
    case Motion::NextParagraph: {
        return Vim::paragraphPosition(document, position, true, count);
    }
    case Motion::PreviousParagraph: {
        return Vim::paragraphPosition(document, position, false, count);
    }
    default: {
        break;
    }
    }

    QTextCursor cursor(document);
    for (int step = 0; step < count; ++step) {
        const int previousPosition = position;
        switch (_motion) {
        case Motion::Left: {
            const int lineStart = Vim::startOfLinePosition(document, position);
            const int lineEnd = Vim::endOfLinePosition(document, position);
            //
            // ... курсор-блок стоит на последнем символе строки, а не за ним
            //
            if (!_forOperator && position >= lineEnd && lineEnd > lineStart) {
                position = lineEnd - 1;
            }
            position = qMax(lineStart, position - 1);
            break;
        }
        case Motion::Right: {
            const int lineStart = Vim::startOfLinePosition(document, position);
            const int lineEnd = Vim::endOfLinePosition(document, position);
            const int lastAllowedPosition = _forOperator ? lineEnd : qMax(lineStart, lineEnd - 1);
            position = qMin(lastAllowedPosition, position + 1);
            break;
        }
        case Motion::Down:
        case Motion::Up: {
            cursor.setPosition(position);
            if (!cursor.movePosition(_motion == Motion::Down ? QTextCursor::Down
                                                             : QTextCursor::Up)) {
                break;
            }
            position = cursor.position();
            break;
        }
        case Motion::NextWord:
        case Motion::NextBigWord: {
            position = Vim::nextWordPosition(document, position, _motion == Motion::NextBigWord);
            break;
        }
        case Motion::PreviousWord:
        case Motion::PreviousBigWord: {
            position
                = Vim::previousWordPosition(document, position, _motion == Motion::PreviousBigWord);
            break;
        }
        case Motion::EndOfWord:
        case Motion::EndOfBigWord: {
            position = Vim::endOfWordPosition(document, position, _motion == Motion::EndOfBigWord);
            break;
        }
        default: {
            break;
        }
        }

        //
        // ... если дальше двигаться некуда, то нет смысла продолжать
        //
        if (position == previousPosition) {
            break;
        }
    }

    return position;
}

void VimTextEditController::applyMotion(int _targetPosition, bool _inclusive, bool _linewise)
{
    const auto pendingOperator = m_pendingOperator;
    if (_targetPosition < 0) {
        resetPendingCommand();
        return;
    }

    if (pendingOperator == Operator::None) {
        goToPosition(_targetPosition);
        resetPendingCommand();
        return;
    }

    const int sourcePosition = currentPosition();
    if (_linewise) {
        applyOperator(pendingOperator, linesRange(sourcePosition, _targetPosition));
        return;
    }

    const int start = qMin(sourcePosition, _targetPosition);
    int end = qMax(sourcePosition, _targetPosition);
    if (_inclusive) {
        end = boundedPosition(end + 1);
    }
    applyOperator(pendingOperator, { start, end, false });
}

void VimTextEditController::goToPosition(int _position)
{
    if (_position < 0) {
        return;
    }

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

int VimTextEditController::currentPosition() const
{
    if (isVisualMode() && m_visualCursorPosition >= 0) {
        return boundedPosition(m_visualCursorPosition);
    }

    return boundedPosition(m_editor->textCursor().position());
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
    return Vim::boundedPosition(m_editor->document(), _position);
}

int VimTextEditController::visualCursorPosition(int _position) const
{
    auto document = m_editor->document();
    const int position = boundedPosition(_position);
    const int lineStart = Vim::startOfLinePosition(document, position);
    const int lineEnd = Vim::endOfLinePosition(document, position);
    if (position >= lineEnd && lineEnd > lineStart) {
        return lineEnd - 1;
    }
    return position;
}

int VimTextEditController::visualSelectionEndPosition(int _position) const
{
    auto document = m_editor->document();
    const int position = boundedPosition(_position);
    return qMin(Vim::endOfLinePosition(document, position), position + 1);
}

Vim::TextRange VimTextEditController::linesRange(int _fromPosition, int _toPosition) const
{
    auto document = m_editor->document();
    return { Vim::startOfLinePosition(document, qMin(_fromPosition, _toPosition)),
             Vim::endOfLinePosition(document, qMax(_fromPosition, _toPosition)), true };
}

QTextCursor VimTextEditController::rangeCursor(const Vim::TextRange& _range,
                                               bool _includeSeparator) const
{
    QTextCursor cursor(m_editor->document());
    cursor.setPosition(boundedPosition(_range.start));
    cursor.setPosition(boundedPosition(_range.end), QTextCursor::KeepAnchor);

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

void VimTextEditController::applyOperator(Operator _operator, const Vim::TextRange& _range)
{
    const auto registerName = m_pendingRegister;
    resetPendingCommand();

    if (!_range.isValid()) {
        return;
    }

    switch (_operator) {
    case Operator::Yank: {
        yankRange(_range, registerName);
        break;
    }
    case Operator::Delete: {
        deleteRange(_range, registerName);
        break;
    }
    case Operator::Change: {
        changeRange(_range, registerName);
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

void VimTextEditController::yankRange(const Vim::TextRange& _range, QChar _registerName)
{
    if (!_range.isValid()) {
        return;
    }

    const auto cursor = rangeCursor(_range, false);
    m_registers.setYankedContent(_registerName,
                                 { selectedTextForClipboard(cursor), _range.linewise });

    //
    // ... курсор встаёт в начало скопированного фрагмента, как это делает vim
    //
    setCursorPosition(_range.linewise
                          ? Vim::firstNonBlankPosition(m_editor->document(), _range.start)
                          : _range.start);
}

void VimTextEditController::deleteRange(const Vim::TextRange& _range, QChar _registerName)
{
    if (!canEdit() || !_range.isValid()) {
        return;
    }

    auto cursor = rangeCursor(_range, false);
    m_registers.setDeletedContent(_registerName,
                                  { selectedTextForClipboard(cursor), _range.linewise });

    cursor = rangeCursor(_range, _range.linewise);
    if (!cursor.hasSelection()) {
        setCursorPosition(_range.start);
        return;
    }

    m_editor->setTextCursor(cursor);
    sendDeleteKeyPressToEditor();

    if (_range.linewise) {
        //
        // ... курсор ставим на первый значимый символ оставшейся строки, как это делает vim
        //
        setCursorPosition(
            Vim::firstNonBlankPosition(m_editor->document(), m_editor->textCursor().position()));
    }
}

void VimTextEditController::changeRange(const Vim::TextRange& _range, QChar _registerName)
{
    if (!canEdit() || !_range.isValid()) {
        return;
    }

    //
    // ... в отличие от удаления, сами строки остаются на месте, лишь очищается их содержимое
    //
    const auto cursor = rangeCursor(_range, false);
    m_registers.setDeletedContent(_registerName,
                                  { selectedTextForClipboard(cursor), _range.linewise });

    if (cursor.hasSelection()) {
        m_editor->setTextCursor(cursor);
        sendDeleteKeyPressToEditor();
    } else {
        setCursorPosition(_range.start);
    }

    enterInsertMode();
}

void VimTextEditController::changeCaseRange(const Vim::TextRange& _range, Operator _operator)
{
    if (!canEdit() || !_range.isValid()) {
        return;
    }

    auto document = m_editor->document();
    const int start = boundedPosition(_range.start);
    const int end = boundedPosition(_range.end);
    if (start >= end) {
        return;
    }

    //
    // ... идём по абзацам, чтобы не потерять их форматирование
    //
    QVector<QPair<int, int>> blockRanges;
    auto block = document->findBlock(start);
    while (block.isValid() && block.position() < end) {
        const int blockStart = qMax(start, block.position());
        const int blockEnd = qMin(end, block.position() + block.length() - 1);
        if (blockEnd > blockStart) {
            blockRanges.append({ blockStart, blockEnd });
        }
        block = block.next();
    }

    QTextCursor cursor(document);
    cursor.beginEditBlock();
    for (int index = blockRanges.size() - 1; index >= 0; --index) {
        cursor.setPosition(blockRanges.at(index).first);
        cursor.setPosition(blockRanges.at(index).second, QTextCursor::KeepAnchor);
        const auto text = cursor.selectedText();
        QString changedText;
        changedText.reserve(text.size());
        for (const auto character : text) {
            switch (_operator) {
            case Operator::Lowercase: {
                changedText.append(character.toLower());
                break;
            }
            case Operator::Uppercase: {
                changedText.append(character.toUpper());
                break;
            }
            default: {
                changedText.append(character.isUpper() ? character.toLower() : character.toUpper());
                break;
            }
            }
        }
        cursor.insertText(changedText);
    }
    cursor.endEditBlock();

    setCursorPosition(start);
}

void VimTextEditController::undo()
{
    if (isVisualMode()) {
        enterNormalMode();
    }
    resetPendingCommand();
    skipChangeTracking();
    triggerEditorShortcut(QKeySequence::Undo);
}

void VimTextEditController::redo()
{
    if (isVisualMode()) {
        enterNormalMode();
    }
    resetPendingCommand();
    skipChangeTracking();
    triggerEditorShortcut(QKeySequence::Redo);
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

void VimTextEditController::deleteCharacters(int _count, bool _previous, QChar _registerName)
{
    if (!canEdit()) {
        return;
    }

    auto document = m_editor->document();
    int position = currentPosition();
    const int lineStart = Vim::startOfLinePosition(document, position);
    const int lineEnd = Vim::endOfLinePosition(document, position);
    if (lineEnd <= lineStart) {
        return;
    }
    if (position >= lineEnd) {
        position = lineEnd - 1;
    }

    const int count = qMax(1, _count);
    const int start = _previous ? qMax(lineStart, position - count) : position;
    const int end = _previous ? position : qMin(lineEnd, position + count);
    if (end <= start) {
        return;
    }

    deleteRange({ start, end, false }, _registerName);
}

void VimTextEditController::deleteToEndOfLine(bool _change, QChar _registerName)
{
    if (!canEdit()) {
        return;
    }

    auto document = m_editor->document();
    int position = currentPosition();
    const int lineStart = Vim::startOfLinePosition(document, position);
    const int lineEnd = Vim::endOfLinePosition(document, position);
    if (!_change && position >= lineEnd && lineEnd > lineStart) {
        position = lineEnd - 1;
    }
    position = qBound(lineStart, position, lineEnd);

    const Vim::TextRange range{ position, lineEnd, false };
    if (_change) {
        changeRange(range, _registerName);
    } else {
        deleteRange(range, _registerName);
    }
}

void VimTextEditController::replaceCharacters(const QString& _text, int _count)
{
    if (!canEdit() || _text.isEmpty()) {
        return;
    }

    auto document = m_editor->document();
    int position = currentPosition();
    const int lineStart = Vim::startOfLinePosition(document, position);
    const int lineEnd = Vim::endOfLinePosition(document, position);
    if (lineEnd <= lineStart) {
        return;
    }
    if (position >= lineEnd) {
        position = lineEnd - 1;
    }

    //
    // ... если в строке не хватает символов, то замена не выполняется вовсе
    //
    const int count = qMax(1, _count);
    if (position + count > lineEnd) {
        return;
    }

    QTextCursor cursor(document);
    cursor.setPosition(position);
    cursor.setPosition(position + count, QTextCursor::KeepAnchor);
    cursor.insertText(QString(count, _text.at(0)));

    //
    // ... курсор остаётся на последнем заменённом символе
    //
    setCursorPosition(position + count - 1);
}

void VimTextEditController::replaceRangeCharacters(const Vim::TextRange& _range,
                                                   const QString& _text)
{
    if (!canEdit() || !_range.isValid() || _text.isEmpty()) {
        return;
    }

    auto document = m_editor->document();
    const int start = boundedPosition(_range.start);
    const int end = boundedPosition(_range.end);

    QVector<QPair<int, int>> blockRanges;
    auto block = document->findBlock(start);
    while (block.isValid() && block.position() < end) {
        const int blockStart = qMax(start, block.position());
        const int blockEnd = qMin(end, block.position() + block.length() - 1);
        if (blockEnd > blockStart) {
            blockRanges.append({ blockStart, blockEnd });
        }
        block = block.next();
    }

    QTextCursor cursor(document);
    cursor.beginEditBlock();
    for (int index = blockRanges.size() - 1; index >= 0; --index) {
        const int blockStart = blockRanges.at(index).first;
        const int blockEnd = blockRanges.at(index).second;
        cursor.setPosition(blockStart);
        cursor.setPosition(blockEnd, QTextCursor::KeepAnchor);
        cursor.insertText(QString(blockEnd - blockStart, _text.at(0)));
    }
    cursor.endEditBlock();

    setCursorPosition(start);
}

void VimTextEditController::toggleCaseUnderCursor(int _count)
{
    if (!canEdit()) {
        return;
    }

    auto document = m_editor->document();
    const int position = currentPosition();
    const int lineEnd = Vim::endOfLinePosition(document, position);
    const int end = qMin(lineEnd, position + qMax(1, _count));
    if (end <= position) {
        return;
    }

    changeCaseRange({ position, end, false }, Operator::SwapCase);
    setCursorPosition(end);
}

void VimTextEditController::joinLines(int _count, bool _withSpace)
{
    if (!canEdit()) {
        return;
    }

    auto document = m_editor->document();
    auto cursor = m_editor->textCursor();
    cursor.clearSelection();

    cursor.beginEditBlock();
    for (int step = 0; step < qMax(1, _count); ++step) {
        const auto block = document->findBlock(cursor.position());
        if (!block.isValid() || !block.next().isValid()) {
            break;
        }

        const int joinPosition = block.position() + block.length() - 1;
        int endPosition = joinPosition + 1;
        if (_withSpace) {
            const auto nextText = block.next().text();
            int offset = 0;
            while (offset < nextText.size() && nextText.at(offset).isSpace()) {
                ++offset;
            }
            endPosition += offset;
        }

        cursor.setPosition(joinPosition);
        cursor.setPosition(qMin(endPosition, Vim::lastPosition(document)), QTextCursor::KeepAnchor);
        if (_withSpace) {
            cursor.insertText(QStringLiteral(" "));
        } else {
            cursor.removeSelectedText();
        }
        cursor.setPosition(joinPosition);
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

void VimTextEditController::paste(bool _before, int _count, QChar _registerName)
{
    pasteContent(m_registers.content(_registerName), _before, _count);
}

void VimTextEditController::pasteContent(const Vim::RegisterContent& _content, bool _before,
                                         int _count)
{
    if (!canEdit() || _content.isEmpty()) {
        return;
    }

    const int count = qMax(1, _count);
    auto cursor = m_editor->textCursor();
    cursor.clearSelection();

    if (_content.linewise) {
        auto lineText = _content.text;
        while (lineText.endsWith("\n")) {
            lineText.chop(1);
        }

        cursor.movePosition(_before ? QTextCursor::StartOfBlock : QTextCursor::EndOfBlock);
        const int insertPosition = cursor.position();
        cursor.beginEditBlock();
        for (int index = 0; index < count; ++index) {
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
        setCursorPosition(Vim::firstNonBlankPosition(
            m_editor->document(), boundedPosition(_before ? insertPosition : insertPosition + 1)));
        return;
    }

    if (!_before && !cursor.atBlockEnd()) {
        cursor.movePosition(QTextCursor::NextCharacter);
    }

    const int insertPosition = cursor.position();
    cursor.beginEditBlock();
    for (int index = 0; index < count; ++index) {
        cursor.insertText(_content.text);
    }
    cursor.endEditBlock();

    //
    // ... курсор ставим на последний вставленный символ
    //
    setCursorPosition(qMax(insertPosition, cursor.position() - 1));
}

void VimTextEditController::replaceSelectionWithRegister(QChar _registerName)
{
    const auto content = m_registers.content(_registerName);
    const auto range = visualRange();
    leaveVisualMode();

    if (!range.isValid()) {
        return;
    }

    //
    // ... удаляем в чёрную дыру, чтобы не затереть то, что собираемся вставить
    //
    deleteRange(range, QChar('_'));
    pasteContent(content, true, 1);
}

void VimTextEditController::setMark(QChar _name)
{
    QTextCursor cursor(m_editor->document());
    cursor.setPosition(currentPosition());
    m_marks[_name] = cursor;
}

void VimTextEditController::jumpToMark(QChar _name, bool _exact)
{
    if (!m_marks.contains(_name)) {
        resetPendingCommand();
        return;
    }

    const bool linewise = !_exact && m_pendingOperator != Operator::None;
    const int position = boundedPosition(m_marks.value(_name).position());
    rememberJumpPosition();
    applyMotion(_exact ? position : Vim::firstNonBlankPosition(m_editor->document(), position),
                false, linewise);
}

void VimTextEditController::rememberJumpPosition()
{
    QTextCursor cursor(m_editor->document());
    cursor.setPosition(currentPosition());
    m_marks[QChar('`')] = cursor;
    m_marks[QChar('\'')] = cursor;
}

void VimTextEditController::search(const QString& _pattern, bool _forward)
{
    if (!_pattern.isEmpty()) {
        m_searchPattern = _pattern;
    }
    m_searchForward = _forward;
    if (m_searchPattern.isEmpty()) {
        return;
    }

    const int target = searchPosition(m_searchPattern, currentPosition(), _forward);
    if (target < 0) {
        return;
    }

    rememberJumpPosition();
    goToPosition(target);
}

void VimTextEditController::searchNext(bool _reverse, int _count)
{
    if (m_searchPattern.isEmpty()) {
        resetPendingCommand();
        return;
    }

    const bool forward = _reverse ? !m_searchForward : m_searchForward;
    int position = currentPosition();
    for (int step = 0; step < qMax(1, _count); ++step) {
        const int target = searchPosition(m_searchPattern, position, forward);
        if (target < 0) {
            break;
        }
        position = target;
    }

    rememberJumpPosition();
    applyMotion(position, false, false);
}

void VimTextEditController::searchWordUnderCursor(bool _forward)
{
    const auto word = Vim::wordUnderCursor(m_editor->document(), currentPosition());
    resetPendingCommand();
    if (word.isEmpty()) {
        return;
    }

    m_searchPattern = QStringLiteral("\\b%1\\b").arg(QRegularExpression::escape(word));
    m_searchForward = _forward;

    const int target = searchPosition(m_searchPattern, currentPosition(), _forward);
    if (target < 0) {
        return;
    }

    rememberJumpPosition();
    goToPosition(target);
}

int VimTextEditController::searchPosition(const QString& _pattern, int _fromPosition,
                                          bool _forward) const
{
    if (_pattern.isEmpty()) {
        return -1;
    }

    auto document = m_editor->document();
    QRegularExpression expression(_pattern);
    if (!expression.isValid()) {
        expression = QRegularExpression(QRegularExpression::escape(_pattern));
    }

    QTextDocument::FindFlags findFlags;
    if (!_forward) {
        findFlags |= QTextDocument::FindBackward;
    }

    //
    // ... поиск вперёд начинаем со следующего символа, иначе будем находить сами себя
    //
    const int fromPosition
        = _forward ? boundedPosition(_fromPosition + 1) : boundedPosition(_fromPosition);
    auto cursor = document->find(expression, fromPosition, findFlags);
    if (cursor.isNull()) {
        //
        // ... и продолжаем поиск с другого конца документа
        //
        cursor = document->find(expression, _forward ? 0 : Vim::lastPosition(document), findFlags);
    }

    return cursor.isNull() ? -1 : cursor.selectionStart();
}

void VimTextEditController::scrollPages(qreal _pages)
{
    const int cursorHeight = qMax(1, m_editor->cursorRect().height());
    const int visibleLines = qMax(1, m_editor->viewport()->height() / cursorHeight);
    const int lines = qMax(1, static_cast<int>(qAbs(_pages) * visibleLines));
    const bool forward = _pages > 0;

    QTextCursor cursor(m_editor->document());
    cursor.setPosition(currentPosition());
    for (int step = 0; step < lines; ++step) {
        if (!cursor.movePosition(forward ? QTextCursor::Down : QTextCursor::Up)) {
            break;
        }
    }

    resetPendingCommand();
    goToPosition(cursor.position());
}

void VimTextEditController::scrollCursorTo(ScrollAlignment _alignment)
{
    m_editor->ensureCursorVisible();

    const auto cursorRect = m_editor->cursorRect();
    const int viewportHeight = m_editor->viewport()->height();
    int delta = 0;
    switch (_alignment) {
    case ScrollAlignment::Center: {
        delta = cursorRect.top() - (viewportHeight - cursorRect.height()) / 2;
        break;
    }
    case ScrollAlignment::Top: {
        delta = cursorRect.top();
        break;
    }
    case ScrollAlignment::Bottom: {
        delta = cursorRect.bottom() - viewportHeight;
        break;
    }
    }

    m_editor->scrollVerticallyBy(delta);
}

void VimTextEditController::openCommandLine(const QString& _prefix)
{
    resetPendingCommand();
    m_commandLineActive = true;
    m_commandLine = _prefix;
}

void VimTextEditController::closeCommandLine()
{
    m_commandLineActive = false;
    m_commandLine.clear();
}

bool VimTextEditController::handleCommandLine(QKeyEvent* _event)
{
    if (isEnter(_event)) {
        const auto command = m_commandLine;
        closeCommandLine();
        executeCommandLine(command);
        return true;
    }

    if (_event->key() == Qt::Key_Backspace) {
        m_commandLine.chop(1);
        if (m_commandLine.isEmpty()) {
            closeCommandLine();
        }
        return true;
    }

    const auto text = _event->text();
    if (!text.isEmpty() && text.at(0).isPrint()) {
        m_commandLine.append(text);
    }
    return true;
}

void VimTextEditController::executeCommandLine(const QString& _command)
{
    if (_command.size() < 2) {
        return;
    }

    const auto prefix = _command.at(0);
    const auto body = _command.mid(1);
    if (prefix == '/' || prefix == '?') {
        search(body, prefix == '/');
        return;
    }

    if (prefix == ':') {
        executeExCommand(body);
    }
}

void VimTextEditController::executeExCommand(const QString& _command)
{
    const auto command = _command.trimmed();
    if (command.isEmpty()) {
        return;
    }

    auto document = m_editor->document();

    //
    // ... переход к строке по её номеру
    //
    if (command == "$") {
        rememberJumpPosition();
        goToPosition(Vim::linePosition(document, document->blockCount()));
        return;
    }
    bool isLineNumber = false;
    const int lineNumber = command.toInt(&isLineNumber);
    if (isLineNumber) {
        rememberJumpPosition();
        goToPosition(Vim::linePosition(document, lineNumber));
        return;
    }

    //
    // ... замена текста в текущей строке, либо во всём документе
    //
    if (command.startsWith("%s") && command.size() > 2 && !command.at(2).isLetterOrNumber()) {
        substitute(command.mid(2), true);
        return;
    }
    if (command.startsWith("s") && command.size() > 1 && !command.at(1).isLetterOrNumber()) {
        substitute(command.mid(1), false);
    }
}

void VimTextEditController::substitute(const QString& _arguments, bool _wholeDocument)
{
    if (!canEdit() || _arguments.size() < 2) {
        return;
    }

    const auto separator = _arguments.at(0);
    const auto parts = _arguments.mid(1).split(separator);
    const auto pattern = parts.value(0);
    const auto replacement = parts.value(1);
    const auto flags = parts.value(2);
    if (pattern.isEmpty()) {
        return;
    }

    QRegularExpression expression(pattern);
    if (!expression.isValid()) {
        expression = QRegularExpression(QRegularExpression::escape(pattern));
    }
    if (flags.contains('i')) {
        expression.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    }
    const bool replaceAll = flags.contains('g');

    auto document = m_editor->document();
    const int startPosition
        = _wholeDocument ? 0 : Vim::startOfLinePosition(document, currentPosition());
    const int endPosition = _wholeDocument ? Vim::lastPosition(document)
                                           : Vim::endOfLinePosition(document, currentPosition());

    //
    // ... собираем замены по абзацам, чтобы не потерять их форматирование
    //
    struct BlockReplacement {
        int position = 0;
        int length = 0;
        QString text;
    };
    QVector<BlockReplacement> replacements;
    auto block = document->findBlock(startPosition);
    while (block.isValid() && block.position() <= endPosition) {
        const auto text = block.text();
        auto replacedText = text;
        if (replaceAll) {
            replacedText.replace(expression, replacement);
        } else {
            const auto match = expression.match(text);
            if (match.hasMatch()) {
                replacedText = text.left(match.capturedStart()) + replacement
                    + text.mid(match.capturedEnd());
            }
        }

        if (replacedText != text) {
            replacements.append({ block.position(), block.length() - 1, replacedText });
        }
        block = block.next();
    }

    if (replacements.isEmpty()) {
        return;
    }

    QTextCursor cursor(document);
    cursor.beginEditBlock();
    for (int index = replacements.size() - 1; index >= 0; --index) {
        const auto replacement = replacements.at(index);
        cursor.setPosition(replacement.position);
        cursor.setPosition(replacement.position + replacement.length, QTextCursor::KeepAnchor);
        cursor.insertText(replacement.text);
    }
    cursor.endEditBlock();
}

void VimTextEditController::trackKeyPress(const RecordedKey& _key, bool _wasInsertMode,
                                          int _revisionBefore, bool _trackChange)
{
    //
    // ... записываем макрос
    //
    if (m_replayDepth == 0 && !m_recordingRegister.isNull()) {
        if (m_skipMacroRecordKey) {
            m_skipMacroRecordKey = false;
        } else if (m_recordedKeys.size() < kMaxRecordedKeys) {
            m_recordedKeys.append(_key);
        }
    }

    //
    // ... и собираем команду, которую можно будет повторить
    //
    if (!_trackChange) {
        m_changeKeys.clear();
        m_collectingChange = false;
        return;
    }

    if (_wasInsertMode) {
        if (!m_collectingChange) {
            return;
        }

        m_changeKeys.append(_key);
        if (m_mode != Mode::Insert) {
            m_lastChangeKeys = m_changeKeys;
            m_changeKeys.clear();
            m_collectingChange = false;
        }
        return;
    }

    if (m_commandLineActive) {
        m_changeKeys.clear();
        m_collectingChange = false;
        return;
    }

    m_changeKeys.append(_key);
    m_collectingChange = true;

    //
    // ... команда ещё не завершена
    //
    if (m_mode == Mode::Insert || hasPendingInput()) {
        return;
    }

    if (m_editor->document()->revision() != _revisionBefore) {
        m_lastChangeKeys = m_changeKeys;
    }
    m_changeKeys.clear();
    m_collectingChange = false;
}

void VimTextEditController::skipChangeTracking()
{
    if (m_trackChangeTarget != nullptr) {
        *m_trackChangeTarget = false;
    }
}

void VimTextEditController::toggleMacroRecording(QChar _name)
{
    skipChangeTracking();

    if (!m_recordingRegister.isNull()) {
        m_macros[m_recordingRegister] = m_recordedKeys;
        m_recordedKeys.clear();
        m_recordingRegister = { };
        return;
    }

    m_recordingRegister = _name.toLower();
    m_recordedKeys.clear();
    m_skipMacroRecordKey = true;
}

void VimTextEditController::playMacro(QChar _name, int _count)
{
    const auto keys = m_macros.value(_name.toLower());
    if (keys.isEmpty()) {
        return;
    }

    m_lastPlayedRegister = _name.toLower();
    skipChangeTracking();
    for (int step = 0; step < qMax(1, _count); ++step) {
        replayKeys(keys);
    }
}

void VimTextEditController::repeatLastChange(int _count)
{
    if (m_lastChangeKeys.isEmpty()) {
        return;
    }

    skipChangeTracking();
    const auto keys = m_lastChangeKeys;
    for (int step = 0; step < qMax(1, _count); ++step) {
        replayKeys(keys);
    }
}

void VimTextEditController::replayKeys(QVector<RecordedKey> _keys)
{
    if (_keys.isEmpty() || m_replayDepth >= kMaxReplayDepth) {
        return;
    }

    ++m_replayDepth;
    for (const auto& key : _keys) {
        QKeyEvent event(QEvent::KeyPress, key.key, key.modifiers, key.text);
        QGuiApplication::sendEvent(m_editor, &event);
    }
    --m_replayDepth;
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

void VimTextEditController::finishHandledEvent(QKeyEvent* _event)
{
    m_editor->ensureCursorVisible();
    _event->accept();
}

} // namespace Ui
