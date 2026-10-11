#pragma once

#include "vim_motions.h"
#include "vim_registers.h"

#include <QChar>
#include <QHash>
#include <QKeySequence>
#include <QString>
#include <QTextCursor>
#include <QVector>

class QInputMethodEvent;
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
     * @brief Текст строки состояния - набираемая команда, или индикатор записи макроса
     */
    QString statusText() const;

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

    /**
     * @brief Обработать ввод через метод ввода, например символов с мёртвыми клавишами
     * @return Обработано ли событие
     */
    bool handleInputMethodEvent(QInputMethodEvent* _event);

    /**
     * @brief Синхронизировать режим с выделением, сделанным не через Vim-команды, например мышью
     */
    void handleSelectionChange();

private:
    /**
     * @brief Оператор, ожидающий цели, над которой нужно выполнить действие
     */
    enum class Operator {
        None,
        Change,
        Delete,
        Yank,
        SwapCase,
        Lowercase,
        Uppercase,
    };

    /**
     * @brief Что ожидается от следующего нажатия клавиши
     */
    enum class Awaiting {
        None,
        TextObject,
        FindCharacter,
        ReplaceCharacter,
        SetMark,
        JumpToMark,
        RegisterName,
        RecordRegister,
        PlayRegister,
        GoCommand,
        ScrollCommand,
    };

    /**
     * @brief Результат вычисления перемещения курсора
     */
    struct Motion {
        /**
         * @brief Позиция, в которую нужно переместить курсор, -1 если перемещение невозможно
         */
        int position = -1;

        /**
         * @brief Входит ли символ в позиции назначения в диапазон оператора
         */
        bool inclusive = false;

        /**
         * @brief Работает ли оператор с перемещением построчно
         */
        bool linewise = false;

        /**
         * @brief Является ли перемещение прыжком, после которого можно вернуться назад
         */
        bool jump = false;

        /**
         * @brief Сохранить ли горизонтальное положение курсора для перемещений вверх и вниз
         */
        bool keepPreferredX = false;

        /**
         * @brief Не переносить конец диапазона оператора с начала строки на конец предыдущей
         */
        bool keepOperatorEnd = false;

        /**
         * @brief Перемещение в конец строки, которое сохраняется при перемещениях вверх и вниз
         */
        bool toLineEnd = false;

        /**
         * @brief Попадает ли текст в нумерованный регистр при удалении
         */
        bool toNumberedRegister = false;
    };

    /**
     * @brief Запомненное нажатие клавиши - для макросов и повтора последнего изменения
     */
    struct RecordedKey {
        int key = 0;
        Qt::KeyboardModifiers modifiers;
        QString text;
    };

    /**
     * @brief Состояние, общее для всех редакторов: регистры, макросы и последний поиск
     */
    struct SharedState;
    static SharedState& shared();

    //
    // Режимы
    //

    bool isVisualMode() const;
    bool hasPendingInput() const;
    void resetPendingCommand();
    void enterInsertMode(int _repeat, QChar _command);
    void leaveInsertMode();
    void enterNormalMode();
    void enterVisualMode(Mode _mode, int _anchor, int _cursor);
    void leaveVisualMode();
    void restoreLastVisualSelection();
    void updateVisualSelection();
    Vim::TextRange visualRange() const;
    int visualLinesCount() const;

    //
    // Количество повторений
    //

    void appendCount(QChar _digit);
    bool hasCount() const;
    int takeCount();
    int takeOptionalCount();

    //
    // Обработка нажатий
    //

    bool handleKeyPressImpl(QKeyEvent* _event);
    bool handleCommand(QKeyEvent* _event);
    bool handleAwaitingInput(QKeyEvent* _event);
    bool handleNormalCommand(QKeyEvent* _event);
    bool handleVisualCommand(QKeyEvent* _event);
    bool handleOperatorTarget(QKeyEvent* _event);
    bool handleGoCommand(QKeyEvent* _event);
    bool handleScrollCommand(QKeyEvent* _event);
    bool handleTextObject(QKeyEvent* _event);
    bool handleCommonCommand(QKeyEvent* _event);

    //
    // Перемещения
    //

    bool isMotionKey(QKeyEvent* _event) const;
    Motion motionForKey(QKeyEvent* _event, int _count, bool _hasCount);
    Motion verticalMotion(int _count, bool _down, bool _visualLines);
    Motion lineMotion(int _lineNumber, bool _firstNonBlank) const;
    Motion findCharacterMotion(QChar _character, bool _forward, bool _till, int _count,
                               bool _repeat) const;
    Motion markMotion(QChar _name, bool _exact) const;
    Motion searchMotion(const QString& _pattern, bool _forward, int _count, int _fromPosition);
    void applyMotion(const Motion& _motion);
    void applyOperatorToMotion(const Motion& _motion);
    void applyOperatorToObject(const Vim::ObjectRange& _object);

    //
    // Позиции в документе
    //

    QTextDocument* document() const;
    int currentPosition() const;
    QTextBlock currentBlock() const;
    int characterPosition(int _position) const;
    void setCursorPosition(int _position);
    Vim::TextRange linesRange(int _fromPosition, int _toPosition) const;

    //
    // Операторы
    //

    void applyOperator(Operator _operator, const Vim::TextRange& _range, int _cursorPosition,
                       bool _toNumberedRegister);
    void yankRange(const Vim::TextRange& _range, QChar _registerName, int _cursorPosition);
    void deleteRange(const Vim::TextRange& _range, QChar _registerName, bool _toNumberedRegister);
    void changeRange(const Vim::TextRange& _range, QChar _registerName, bool _toNumberedRegister);
    void changeCaseRange(const Vim::TextRange& _range, Operator _operator);
    QString rangeText(const Vim::TextRange& _range) const;
    Vim::RegisterContent registerContent(const Vim::TextRange& _range) const;
    QVector<Vim::TextRange> lineRanges(const Vim::TextRange& _range) const;

    //
    // Редактирование
    //

    bool canEdit() const;
    void removeText(int _from, int _to);
    void insertText(int _position, const QString& _text);
    void replaceText(int _from, int _to, const QString& _text);
    bool openLine(bool _below);
    void deleteCharacters(int _count, bool _previous);
    void replaceCharacters(QChar _character, int _count);
    void toggleCase(int _count);
    void joinLines(int _count, bool _withSpaces);
    void paste(QChar _registerName, bool _before, int _count, bool _cursorAfter = false);
    void pasteContent(const Vim::RegisterContent& _content, bool _before, int _count,
                      bool _cursorAfter = false);
    void pasteLines(const QString& _text, bool _before, int _count, bool _cursorAfter = false);
    void replaceVisualWithRegister(QChar _registerName, bool _keepRegisters, int _count);
    void undo(int _count);
    void redo(int _count);
    void triggerEditorShortcut(QKeySequence::StandardKey _key);
    void sendKeyPressToEditor(int _key, const QString& _text);
    void commitUndoStep();

    //
    // Метки и поиск
    //

    void setMark(QChar _name, int _position);
    void rememberJumpPosition();
    int searchPosition(const QString& _pattern, int _fromPosition, bool _forward) const;
    void searchWordUnderCursor(bool _forward, bool _wholeWord, int _count);

    //
    // Прокрутка
    //

    void scrollHalfPage(bool _down, int _count);
    void scrollCursorTo(int _alignment);

    //
    // Строка ввода команд
    //

    void openCommandLine(const QString& _prefix);
    void closeCommandLine();
    void handleCommandLine(QKeyEvent* _event);
    void executeCommandLine(const QString& _command);
    void executeExCommand(const QString& _command);
    void substitute(QTextBlock _firstLine, QTextBlock _lastLine, const QString& _arguments);

    //
    // Макросы и повтор последнего изменения
    //

    void trackKeyPress(const RecordedKey& _key);
    void toggleMacroRecording(QChar _name);
    void playMacro(QChar _name, int _count);
    void repeatLastChange(int _count);
    void replayKeys(const QVector<RecordedKey>& _keys);

    /**
     * @brief Завершить обработку нажатия клавиши
     */
    void finishHandledEvent(QKeyEvent* _event);

    ScriptTextEdit* m_editor = nullptr;
    bool m_enabled = false;
    Mode m_mode = Mode::Normal;

    /**
     * @brief Состояние набираемой команды
     */
    Awaiting m_awaiting = Awaiting::None;
    Operator m_pendingOperator = Operator::None;
    QString m_count;
    int m_operatorCount = 0;
    QChar m_pendingRegister;
    QChar m_findCommand;
    bool m_textObjectAround = false;
    bool m_jumpToMarkExact = true;

    /**
     * @brief Предпочтительное положение курсора при перемещениях вверх и вниз
     */
    int m_preferredX = -1;
    bool m_preferLineEnd = false;

    /**
     * @brief Выделение режимов VISUAL, в том числе и последнее, для команды gv
     */
    int m_visualAnchor = -1;
    int m_visualCursor = -1;
    QTextCursor m_lastVisualAnchor;
    QTextCursor m_lastVisualCursor;
    Mode m_lastVisualMode = Mode::Visual;

    /**
     * @brief Метки
     */
    QHash<QChar, QTextCursor> m_marks;

    /**
     * @brief Набираемая пользователем команда
     */
    bool m_commandLineActive = false;
    QString m_commandLine;
    Operator m_commandLineOperator = Operator::None;
    int m_commandLineOperatorCount = 0;
    QChar m_commandLineRegister;

    /**
     * @brief Повтор вставленного текста для команд с количеством, например 3ia<Esc>
     */
    int m_insertRepeat = 1;
    QChar m_insertCommand;
    QVector<RecordedKey> m_insertKeys;
    QTextCursor m_lastInsertPosition;

    /**
     * @brief Строка, в которой после изменения остался только отступ
     */
    QTextCursor m_autoIndentLine;
    int m_autoIndentRevision = -1;

    /**
     * @brief Повтор последнего изменения
     */
    QVector<RecordedKey> m_commandKeys;
    QVector<RecordedKey> m_lastChangeKeys;
    int m_commandRevision = -1;
    bool m_commandIsChange = true;

    /**
     * @brief Макросы
     */
    QChar m_recordingRegister;
    QVector<RecordedKey> m_recordedKeys;
    bool m_skipMacroRecordKey = false;
    int m_replayDepth = 0;
    int m_replayedKeys = 0;
    bool m_replayingInsert = false;

    /**
     * @brief Идёт ли обработка нажатия, во время которой редактор меняется самим контроллером
     */
    int m_editingDepth = 0;

    /**
     * @brief Отправляет ли контроллер нажатие клавиши самому редактору
     */
    int m_passThroughDepth = 0;
};

} // namespace Ui
