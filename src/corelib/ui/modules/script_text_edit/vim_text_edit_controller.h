#pragma once

#include <QChar>
#include <QKeySequence>
#include <QString>
#include <QTextCursor>

class QKeyEvent;

namespace Ui {

class ScriptTextEdit;

/**
 * @brief Контроллер Vim-режима редактирования текста
 */
class VimTextEditController
{
public:
    /**
     * @brief Режим работы редактора
     */
    enum class Mode {
        Insert,
        Normal,
        Visual,
        VisualLine,
    };

    explicit VimTextEditController(ScriptTextEdit* _editor);

    /**
     * @brief Включён ли Vim-режим
     */
    bool isEnabled() const;
    void setEnabled(bool _enabled);

    /**
     * @brief Текущий режим работы
     */
    Mode mode() const;

    /**
     * @brief Название текущего режима для отображения пользователю
     */
    QString modeName() const;

    /**
     * @brief Нужно ли отображать курсор в виде блока
     */
    bool usesBlockCursor() const;

    /**
     * @brief Позиция, в которой нужно отобразить курсор-блок
     */
    int blockCursorPosition() const;

    /**
     * @brief Нужно ли перехватить системное сочетание клавиш, чтобы обработать его самостоятельно
     */
    bool handleShortcutOverride(QKeyEvent* _event) const;

    /**
     * @brief Обработать нажатие клавиши
     * @return Обработано ли нажатие
     */
    bool handleKeyPress(QKeyEvent* _event);

private:
    /**
     * @brief Оператор, ожидающий цели, над которой нужно выполнить действие
     */
    enum class Operator {
        None,
        Change,
        Delete,
        Yank,
        Go,
        Replace,
    };

    /**
     * @brief Перемещения курсора
     */
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
    };

    /**
     * @brief Находимся ли в одном из режимов выделения
     */
    bool isVisualMode() const;

    /**
     * @brief Включает ли перемещение символ, на котором остановился курсор
     */
    bool isInclusiveMotion(Motion _motion) const;

    /**
     * @brief Переключение между режимами работы
     */
    void resetPendingCommand();
    void enterInsertMode();
    void enterNormalMode(bool _fromInsert = false);
    void enterVisualMode(Mode _mode);
    void leaveVisualMode();

    /**
     * @brief Работа с выделением режимов VISUAL
     */
    void resetVisualSelection();
    void updateVisualSelection();

    /**
     * @brief Работа с количеством повторений команды
     */
    void appendCount(const QString& _digit);
    int takeCount(int _default = 1);
    int takeOptionalCount();
    int combinedOperatorCount();

    /**
     * @brief Обработка нажатий в конкретном режиме работы
     */
    bool handleNormalMode(QKeyEvent* _event);
    bool handleVisualMode(QKeyEvent* _event);
    bool handlePendingOperator(QKeyEvent* _event);
    bool handlePendingGo(QKeyEvent* _event);
    bool handlePendingReplace(QKeyEvent* _event);

    /**
     * @brief Перемещение курсора
     */
    Motion motionFromKey(QKeyEvent* _event) const;
    bool move(Motion _motion, int _count);
    bool moveVisual(Motion _motion, int _count);
    bool moveCursor(QTextCursor& _cursor, Motion _motion, int _count) const;
    bool moveLeft(QTextCursor& _cursor) const;
    bool moveRight(QTextCursor& _cursor) const;
    bool moveToEndOfWord(QTextCursor& _cursor) const;
    void moveToFirstNonBlank(QTextCursor& _cursor) const;
    void moveToCharacterUnderBlockCursor(QTextCursor& _cursor) const;
    void goToLine(int _lineNumber);
    void goToPosition(int _position);
    void setCursorPosition(int _position);

    /**
     * @brief Вычисление позиций в документе
     */
    int linePosition(int _lineNumber) const;
    int lastLinePosition(int _fromPosition, int _count) const;
    int boundedPosition(int _position) const;
    int visualCursorPosition(int _position) const;
    int visualSelectionEndPosition(int _position) const;

    /**
     * @brief Отмена и повтор последнего действия силами самого редактора
     */
    void undo();
    void redo();
    void triggerEditorShortcut(QKeySequence::StandardKey _key);

    /**
     * @brief Редактирование текста
     */
    void deleteCharacters(int _count, bool _previous = false);
    void deleteToEndOfLine();
    void deleteSelection(bool _yank = true);
    void changeSelection();
    void yankSelection();
    QTextCursor linesCursor(int _fromPosition, int _toPosition, bool _includeSeparator) const;
    void yankLines(int _fromPosition, int _toPosition);
    void deleteLines(int _fromPosition, int _toPosition, bool _yank);
    void changeLines(int _fromPosition, int _toPosition, bool _yank);
    void paste(bool _before, int _count);
    void pasteText(const QString& _text, bool _linewise, bool _before, int _count);
    void replaceSelectionWithRegister();
    void replaceCharacters(const QString& _text, int _count);
    void openLineBelow();
    void openLineAbove();

    /**
     * @brief Доступно ли редактирование текста
     */
    bool canEdit() const;

    /**
     * @brief Отправить нажатие клавиши редактору, чтобы он обработал его сам
     */
    void sendDeleteKeyPressToEditor();
    void sendReturnKeyPressToEditor();

    /**
     * @brief Работа с регистром, в который копируется текст
     */
    QString selectedTextForClipboard(const QTextCursor& _cursor) const;
    void setRegisterText(const QString& _text, bool _linewise);
    QString registerText(bool* _linewise = nullptr) const;

    /**
     * @brief Завершить обработку нажатия клавиши
     */
    void finishHandledEvent(QKeyEvent* _event);

    ScriptTextEdit* m_editor = nullptr;
    bool m_enabled = false;
    Mode m_mode = Mode::Normal;
    Operator m_pendingOperator = Operator::None;
    int m_pendingOperatorCount = 0;
    QString m_count;
    int m_visualAnchorPosition = -1;
    int m_visualCursorPosition = -1;
    QString m_registerText;
    bool m_registerLinewise = false;
};

} // namespace Ui
