#pragma once

#include <ui/widgets/scalable_graphics_view/scalable_graphics_view.h>

class QAbstractItemModel;


namespace Ui {

class AbstractCardItem;
class CardsGraphicsScene;

/**
 * @brief Тип компоновки карточек
 */
enum class CORE_LIBRARY_EXPORT CardsGraphicsViewType {
    Rows,
    Columns,
    HorizontalLines,
    VerticalLines,
};

/**
 * @brief Представление сцены отношений персонажей
 */
class CORE_LIBRARY_EXPORT CardsGraphicsView : public ScalableGraphicsView
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
     * @brief Добавлять дополнительную прокрутку
     */
    void setAdditionalScrollingAvailable(bool _available);

    /**
     * @brief Задать способ компоновки карточек
     */
    void setCardsViewType(CardsGraphicsViewType _type);

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
     * @brief Масштабирование вида линий
     */
    void setLinesScale(int _scale);

    /**
     * @brief Необходимое количество карточек для отображения в ряду
     * @note -1 - для отображения по границам экрана
     */
    void setCardsInRow(int _cardsInRow);

    /**
     * @brief Видимый интервал позиций карточек в линейном виде
     */
    QPair<qreal, qreal> cardsPositionsInterval() const;

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

    /**
     * @brief Загрузить состояние
     * @note Если запланировано упорядочивание карточек, то загрузка произойдёт после упорядочивания
     */
    void restoreState(const QByteArray& _state) override;

signals:
    /**
     * @brief Запрос на отображение (когда появились элементы модели)
     */
    void showRequested();

    /**
     * @brief Запрос на скрытие (когда не осталось элементов модели)
     */
    void hideRequested();

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
     * @brief Упорядочиваем карты при изменении размера вьюпорта
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Перенастраиваем виджет при обновлении дизайн системы
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Отлавливаем комбинации клавиш отмены и повтора последнего действия
     */
    void keyPressEvent(QKeyEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
