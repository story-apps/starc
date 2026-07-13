#pragma once

#include <QChar>
#include <QString>
#include <QTextCursor>

class QKeyEvent;

namespace Ui {

class ScriptTextEdit;

class VimTextEditController
{
public:
    enum class Mode {
        Insert,
        Normal,
        Visual,
    };

    explicit VimTextEditController(ScriptTextEdit* _editor);

    bool isEnabled() const;
    void setEnabled(bool _enabled);
    Mode mode() const;
    QString modeName() const;
    bool usesBlockCursor() const;
    int blockCursorPosition() const;

    bool handleShortcutOverride(QKeyEvent* _event) const;
    bool handleKeyPress(QKeyEvent* _event);

private:
    enum class Operator {
        None,
        Change,
        Delete,
        Yank,
        Go,
        Replace,
    };

    enum class Motion {
        None,
        Left,
        Down,
        Up,
        Right,
        StartOfLine,
        FirstNonBlank,
        EndOfLine,
        NextWord,
        PreviousWord,
        EndOfWord,
        StartOfDocument,
        EndOfDocument,
    };

    void resetPendingCommand();
    void enterInsertMode();
    void enterNormalMode(bool _fromInsert = false);
    void enterVisualMode();
    void resetVisualSelection();
    void updateVisualSelection();

    int takeCount(int _default = 1);
    int combinedOperatorCount();

    bool handleNormalMode(QKeyEvent* _event);
    bool handleVisualMode(QKeyEvent* _event);
    bool handlePendingOperator(QKeyEvent* _event);
    bool handlePendingReplace(QKeyEvent* _event);

    Motion motionFromKey(QKeyEvent* _event) const;
    bool move(Motion _motion, int _count, QTextCursor::MoveMode _mode);
    bool moveVisual(Motion _motion, int _count);
    bool moveCursor(QTextCursor& _cursor, Motion _motion, int _count) const;
    bool moveLeft(QTextCursor& _cursor) const;
    bool moveRight(QTextCursor& _cursor) const;
    void moveToFirstNonBlank(QTextCursor& _cursor) const;
    void moveToCharacterUnderBlockCursor(QTextCursor& _cursor) const;
    int boundedPosition(int _position) const;
    int visualCursorPosition(int _position) const;
    int visualSelectionEndPosition(int _position) const;

    void undo();
    void redo();
    void triggerUndoShortcut();
    void triggerRedoShortcut();
    void deleteCharacters(int _count, bool _previous = false);
    void deleteLines(int _count);
    void deleteToEndOfLine();
    void deleteSelection(bool _yank = true, bool _linewise = false);
    void changeSelection();
    void yankSelection(bool _linewise = false);
    void yankLines(int _count);
    void paste(bool _before);
    void replaceCharacters(const QString& _text, int _count);
    void openLineBelow();
    void openLineAbove();
    bool canEdit() const;
    void sendDeleteKeyPressToEditor();
    void sendReturnKeyPressToEditor();

    QString selectedTextForClipboard(const QTextCursor& _cursor) const;
    void setRegisterText(const QString& _text, bool _linewise);
    QString registerText(bool* _linewise = nullptr) const;

    void finishHandledEvent(QKeyEvent* _event);

    ScriptTextEdit* m_editor = nullptr;
    bool m_enabled = false;
    Mode m_mode = Mode::Normal;
    Operator m_pendingOperator = Operator::None;
    int m_pendingOperatorCount = 1;
    QString m_count;
    int m_visualAnchorPosition = -1;
    int m_visualCursorPosition = -1;
    QString m_registerText;
    bool m_registerLinewise = false;
};

} // namespace Ui
