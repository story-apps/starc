#pragma once

#include <ui/widgets/widget/widget.h>

#include <QAbstractItemView>


/**
 * @brief Виджет дерева элементов
 */
class CORE_LIBRARY_EXPORT Tree : public Widget
{
    Q_OBJECT

public:
    explicit Tree(QWidget* _parent = nullptr);
    ~Tree() override;

    /**
     * @brief Установить модель для отображения
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Получить установленную модель
     */
    QAbstractItemModel* model() const;

    /**
     * @brief Задать необходимость отображения декорации у корневых элементов
     */
    void setRootIsDecorated(bool _decorated);

    /**
     * @brief Установить видимость полос прокрутки
     */
    void setScrollBarVisible(bool _visible);

    /**
     * @brief Получить вертикальную полосу прокрутки виджета
     */
    QScrollBar* verticalScrollBar() const;

    /**
     * @brief Включить/отключить возможность перетаскивания элементов
     */
    void setDragDropEnabled(bool _enabled);

    /**
     * @brief Установить режим выделения элементов в дереве
     */
    void setSelectionMode(QAbstractItemView::SelectionMode _mode);

    /**
     * @brief Получить ширину заданной колонки
     */
    int sizeHintForColumn(int _column) const;

    /**
     * @brief Задать делегат для отрисовки элементов
     */
    void setItemDelegate(QAbstractItemDelegate* _delegate);

    /**
     * @brief Задать текущий индекс
     */
    void setCurrentIndex(const QModelIndex& _index);

    /**
     * @brief Получить текущий индекс дерева
     */
    QModelIndex currentIndex() const;

    /**
     * @brief Получить индекс элемента в заданной позиции
     */
    QModelIndex indexAt(const QPoint& _pos) const;

    /**
     * @brief Получить список выделенных элементов
     */
    QModelIndexList selectedIndexes() const;

    /**
     * @brief Развернуть все элементы
     */
    void expandAll();

    /**
     * @brief Установить необходимость пересчитывать размер элементов в делегате
     */
    void setAutoAdjustSize(bool _auto);

    /**
     * @brief Загрузить состояние дерева
     */
    void restoreState(const QVariant& _state);

    /**
     * @brief Сохранить состояние дерева
     */
    QVariant saveState() const;

signals:
    /**
     * @brief Изменился текущий индекс
     */
    void currentIndexChanged(const QModelIndex& _index);

    /**
     * @brief Пользователь кликнул на заданном элементе
     */
    void clicked(const QModelIndex& _index);

    /**
     * @brief Пользователь сделал двойной клик на элемента
     */
    void doubleClicked(const QModelIndex& _index);

protected:
    /**
     * @brief После смены цвета фона или цвета, нужно перенастроить палитру дерева
     */
    void processBackgroundColorChange() override;
    void processTextColorChange() override;

    /**
     * @brief Корректируем внешний вид виджета дерева и его делегата
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
