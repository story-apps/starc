#pragma once

#include <ui/widgets/scalable_graphics_view/scalable_graphics_view.h>

class QAbstractItemModel;


namespace Ui {

class AbstractCardItem;
class CardsGraphicsScene;

/**
 * @brief Представление сцены отношений персонажей
 */
class CardsGraphicsView : public ScalableGraphicsView
{
    Q_OBJECT

public:
    explicit CardsGraphicsView(CardsGraphicsScene* _scene, QWidget* _parent = nullptr);
    ~CardsGraphicsView() override;

    /**
     * @brief Задать цвет фона
     */
    void setBackgroundColor(const QColor& _color);

    /**
     * @brief Задать способ компоновки карточек
     */
    void setCardsRowView(bool _isRowView);

    /**
     * @brief Размер карточек
     */
    void setCardsSize(int _size);

    /**
     * @brief Соотноешние сторон карточек
     */
    void setCardsRatio(int _ratio);

    /**
     * @brief Расстояние между карточками
     */
    void setCardsSpacing(int _spacing);

    /**
     * @brief Необходимое количество карточек для отображения в ряду
     * @note -1 - для отображения по границам экрана
     */
    void setCardsInRow(int _cardsInRow);

    /**
     * @brief Модель карточек
     */
    QAbstractItemModel* model() const;
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Индекс элемента модели выбранной карточки
     */
    QModelIndex selectedCardItemIndex() const;

    /**
     * @brief Выбрать карточку заданного элемента
     */
    void selectCardItemIndex(const QModelIndex& _index);

    /**
     * @brief Удалить выбранный элемент
     */
    void removeSelectedItem();

    /**
     * @brief Применить заданный фильтр
     */
    void setFilter(const QString& _text, bool _caseSensitive, int _filterType);

    /**
     * @brief Настроить возможность редактирования контента
     */
    void setReadOnly(bool _readOnly);

signals:
    /**
     * @brief Пользователь снял выделение со всех элементов
     */
    void selectionCanceled();

    /**
     * @brief Пользователь хочет отменить изменение
     */
    void undoPressed();
    void redoPressed();

    /**
     * @brief Пользователь выбрал карточку
     */
    void itemSelected(const QModelIndex& _index);

    /**
     * @brief Пользователь сделал двойной клик на элементе
     */
    void itemDoubleClicked(const QModelIndex& _index);

    /**
     * @brief Была изменена карточка
     */
    void itemChanged(const QModelIndex& _index);

protected:
    /**
     * @brief Исключить ли заданный индекс при рассчёте плоского индекса элемента дерева
     * @note Должен быть true, если на сцене не должны отображаться некоторые элементы
     */
    virtual bool excludeFromFlatIndex(const QModelIndex& _index) const;

    /**
     * @brief Создать новую карточку на базе элемента модели
     */
    virtual AbstractCardItem* createCardFor(const QModelIndex& _index) const = 0;

    /**
     * @brief Перенастраиваем виджет при обновлении дизайн системы
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Упорядочиваем карты при изменении размера вьюхи
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Отлавливаем комбинации клавиш отмены и повтора последнего действия
     */
    void keyPressEvent(QKeyEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
