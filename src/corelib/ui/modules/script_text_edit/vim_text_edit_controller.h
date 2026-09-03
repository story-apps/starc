#pragma once

#include "vim_motions.h"
#include "vim_registers.h"

#include <QChar>
#include <QHash>
#include <QKeySequence>
#include <QString>
#include <QTextCursor>
#include <QVector>

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
        Motion,
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
        NextBigWord,
        PreviousWord,
        PreviousBigWord,
        EndOfWord,
        EndOfBigWord,
        NextParagraph,
        PreviousParagraph,
        MatchingBracket,
    };

    /**
     * @brief Положение курсора в области просмотра после прокрутки
     */
    enum class ScrollAlignment {
        Center,
        Top,
        Bottom,
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
     * @brief Находимся ли в одном из режимов выделения
     */
    bool isVisualMode() const;

    /**
     * @brief Ожидает ли контроллер продолжения набираемой команды
     */
    bool hasPendingInput() const;

    /**
     * @brief Переключение между режимами работы
     */
    void resetPendingCommand();
    void enterInsertMode();
    void enterNormalMode(bool _fromInsert = false);
    void enterVisualMode(Mode _mode);
    void leaveVisualMode();
    void restoreLastVisualSelection();

    /**
     * @brief Работа с выделением режимов VISUAL
     */
    void resetVisualSelection();
    void updateVisualSelection();
    Vim::TextRange visualRange() const;

    /**
     * @brief Работа с количеством повторений команды
     */
    void appendCount(const QString& _digit);
    int takeCount(int _default = 1);
    int takeOptionalCount();
    int combinedOperatorCount();

    /**
     * @brief Обработка нажатий клавиш
     */
    bool handleKeyPressImpl(QKeyEvent* _event);
    bool handleNormalMode(QKeyEvent* _event);
    bool handleVisualMode(QKeyEvent* _event);
    bool handleCommonMotion(QKeyEvent* _event);
    bool handleCommonCommand(QKeyEvent* _event);
    bool handleAwaitingInput(QKeyEvent* _event);
    bool handleMotionTarget(QKeyEvent* _event);
    bool handleTextObject(QKeyEvent* _event);
    bool handleFindCharacter(QKeyEvent* _event);
    bool handleReplaceCharacter(QKeyEvent* _event);
    bool handleGoCommand(QKeyEvent* _event);
    bool handleScrollCommand(QKeyEvent* _event);

    /**
     * @brief Перемещение курсора
     */
    Motion motionFromKey(QKeyEvent* _event) const;
    bool isInclusiveMotion(Motion _motion) const;
    bool isLinewiseMotion(Motion _motion) const;
    int motionTargetPosition(Motion _motion, int _fromPosition, int _count,
                             bool _forOperator) const;
    void applyMotion(int _targetPosition, bool _inclusive, bool _linewise);
    void goToPosition(int _position);
    void setCursorPosition(int _position);

    /**
     * @brief Вычисление позиций и диапазонов в документе
     */
    int currentPosition() const;
    int lastLinePosition(int _fromPosition, int _count) const;
    int boundedPosition(int _position) const;
    int visualCursorPosition(int _position) const;
    int visualSelectionEndPosition(int _position) const;
    Vim::TextRange linesRange(int _fromPosition, int _toPosition) const;
    QTextCursor rangeCursor(const Vim::TextRange& _range, bool _includeSeparator) const;

    /**
     * @brief Выполнение операторов над диапазоном текста
     */
    void applyOperator(Operator _operator, const Vim::TextRange& _range);
    void yankRange(const Vim::TextRange& _range, QChar _registerName);
    void deleteRange(const Vim::TextRange& _range, QChar _registerName);
    void changeRange(const Vim::TextRange& _range, QChar _registerName);
    void changeCaseRange(const Vim::TextRange& _range, Operator _operator);

    /**
     * @brief Отмена и повтор последнего действия силами самого редактора
     */
    void undo();
    void redo();
    void triggerEditorShortcut(QKeySequence::StandardKey _key);

    /**
     * @brief Редактирование текста
     */
    void deleteCharacters(int _count, bool _previous, QChar _registerName);
    void deleteToEndOfLine(bool _change, QChar _registerName);
    void replaceCharacters(const QString& _text, int _count);
    void replaceRangeCharacters(const Vim::TextRange& _range, const QString& _text);
    void toggleCaseUnderCursor(int _count);
    void joinLines(int _count, bool _withSpace);
    void openLineBelow();
    void openLineAbove();

    /**
     * @brief Вставка из регистра
     */
    void paste(bool _before, int _count, QChar _registerName);
    void pasteContent(const Vim::RegisterContent& _content, bool _before, int _count);
    void replaceSelectionWithRegister(QChar _registerName);

    /**
     * @brief Метки
     */
    void setMark(QChar _name);
    void jumpToMark(QChar _name, bool _exact);
    void rememberJumpPosition();

    /**
     * @brief Поиск по документу
     */
    void search(const QString& _pattern, bool _forward);
    void searchNext(bool _reverse, int _count);
    void searchWordUnderCursor(bool _forward);
    int searchPosition(const QString& _pattern, int _fromPosition, bool _forward) const;

    /**
     * @brief Прокрутка документа
     */
    void scrollPages(qreal _pages);
    void scrollCursorTo(ScrollAlignment _alignment);

    /**
     * @brief Строка ввода команд
     */
    void openCommandLine(const QString& _prefix);
    void closeCommandLine();
    bool handleCommandLine(QKeyEvent* _event);
    void executeCommandLine(const QString& _command);
    void executeExCommand(const QString& _command);
    void substitute(const QString& _arguments, bool _wholeDocument);

    /**
     * @brief Макросы и повтор последнего изменения
     */
    void trackKeyPress(const RecordedKey& _key, bool _wasInsertMode, int _revisionBefore,
                       bool _trackChange);
    void skipChangeTracking();
    void toggleMacroRecording(QChar _name);
    void playMacro(QChar _name, int _count);
    void repeatLastChange(int _count);
    void replayKeys(QVector<RecordedKey> _keys);

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
     * @brief Текст выделения в том виде, в котором он попадает в регистр
     */
    QString selectedTextForClipboard(const QTextCursor& _cursor) const;

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
    int m_operatorCount = 0;
    QString m_count;
    QChar m_pendingRegister;
    bool m_textObjectAround = false;
    bool m_jumpToMarkExact = true;

    /**
     * @brief Последний поиск символа в строке - для повтора по клавишам ; и ,
     */
    QChar m_findCharacter;
    bool m_findForward = true;
    bool m_findTill = false;

    /**
     * @brief Выделение режимов VISUAL, в том числе и последнее, для команды gv
     */
    int m_visualAnchorPosition = -1;
    int m_visualCursorPosition = -1;
    int m_lastVisualAnchorPosition = -1;
    int m_lastVisualCursorPosition = -1;
    Mode m_lastVisualMode = Mode::Visual;

    /**
     * @brief Регистры и метки
     */
    Vim::Registers m_registers;
    QHash<QChar, QTextCursor> m_marks;

    /**
     * @brief Последний поиск по документу
     */
    QString m_searchPattern;
    bool m_searchForward = true;

    /**
     * @brief Набираемая пользователем команда
     */
    bool m_commandLineActive = false;
    QString m_commandLine;

    /**
     * @brief Макросы и повтор последнего изменения
     */
    QHash<QChar, QVector<RecordedKey>> m_macros;
    QChar m_recordingRegister;
    QChar m_lastPlayedRegister;
    QVector<RecordedKey> m_recordedKeys;
    QVector<RecordedKey> m_changeKeys;
    QVector<RecordedKey> m_lastChangeKeys;
    bool m_collectingChange = false;
    bool m_skipMacroRecordKey = false;
    int m_replayDepth = 0;

    /**
     * @brief Флаг текущего нажатия, отвечающий за то, запоминать ли команду для повтора
     */
    bool* m_trackChangeTarget = nullptr;
};

} // namespace Ui
