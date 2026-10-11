#pragma once

#include <ui/widgets/text_edit/base/base_text_edit.h>

class QMimeData;

namespace Domain {
struct CursorInfo;
}


namespace Ui {

/**
 * @brief Базовый класс текстовых редакторов сценария
 */
class CORE_LIBRARY_EXPORT ScriptTextEdit : public BaseTextEdit
{
    Q_OBJECT

public:
    explicit ScriptTextEdit(QWidget* _parent = nullptr);
    ~ScriptTextEdit() override;

    /**
     * @brief Использовать Vim-режим редактирования
     */
    bool isVimModeEnabled() const;
    void setVimModeEnabled(bool _enabled);

    /**
     * @brief Прокрутить редактор по вертикали на заданное количество пикселей
     */
    void scrollVerticallyBy(int _delta);

    /**
     * @brief Показывать ли автодополнения в пустых блоках
     */
    bool showSuggestionsInEmptyBlocks() const;
    void setShowSuggestionsInEmptyBlocks(bool _show);

    /**
     * @brief Своя реализация установки курсора
     */
    void setTextCursorAndKeepScrollBars(const QTextCursor& _cursor);

    /**
     * @brief Задать курсоры соавторов
     */
    void setCollaboratorsCursors(const QVector<Domain::CursorInfo>& _cursors);

    /**
     * @brief Вставить данные в позиции курсора так же, как из буфера обмена
     * @note Нужно Vim-командам, чтобы вставлять текст вместе с форматированием и типами абзацев
     */
    void insertMimeData(const QMimeData* _source);

    /**
     * @brief Включён ли режим отслеживания изменений, в котором удалённый текст лишь помечается
     * @note Нужно Vim-командам, которые не набирают текст, а заменяют его
     */
    virtual bool isTrackChangesEnabled() const;

    /**
     * @brief Пометить текст, вставленный не с клавиатуры, если включено автоматическое добавление
     *        редакторских заметок
     */
    virtual void markInsertedText(int _from, int _to);

protected:
    /**
     * @brief Обновить положение курсоров соавторов
     */
    void updateCollaboratorsCursors(int _position, int _charsRemoved, int _charsAdded);

    /**
     * @brief Нарисовать курсоры соавторов
     */
    void paintCollaboratorsCursors(QPainter& _painter, const QUuid& _documentUuid,
                                   const QTextBlock& _topBlock,
                                   const QTextBlock& _bottomBlock) const;

    /**
     * @brief Отслеживаем движение мыши, чтобы всегда иметь под рукой её последнюю координату
     * @note QCursor::pos() нам не подходит, т.к. текстовые редакторы находятся внутри
     *       ScalableWrapper и координаты мыши могут несколько искажаться
     */
    void mouseMoveEvent(QMouseEvent* _event) override;

    /**
     * @brief Щелчок мышью отменяет набираемую Vim-команду
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Обновить положение элементов Vim-режима
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Не даём вводить текст через методы ввода, когда активен Vim-режим
     */
    void inputMethodEvent(QInputMethodEvent* _event) override;

    /**
     * @brief Нарисовать дополнительные элементы поверх текста
     */
    void paintOverText(QPainter* _painter) override;

    /**
     * @brief Обрабатываем смену параметров текстового редактора и Vim-команды
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Дополнительная функция для обработки нажатий самим редактором
     * @return Обработано ли событие
     * @note В наследниках необходимо вызывать вручную при необходимости
     */
    bool keyPressEventReimpl(QKeyEvent* _event) override;

    /**
     * @brief Обрабатываем специфичные ситуации для редактора сценария
     */
    bool updateEnteredText(const QKeyEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
