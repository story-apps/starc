#pragma once

#include <ui/widgets/widget/widget.h>

#include <QAbstractItemView>

class QHeaderView;


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
     * @brief Все строки модели (включая вложенные)
     */
    int recursiveRowCount(bool _onlyExpanded = false) const;

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
     * @brief Установить видимость заголовков
     */
    void setHeaderVisible(bool _visible);

    /**
     * @brief Установить ширину колонки
     */
    void setColumnWidth(int _column, int _width);

    /**
     * @brief Настроить видимость колонки
     */
    void setColumnVisible(int _column, bool _visible);

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
    void setItemDelegateForColumn(int _column, QAbstractItemDelegate* _delegate);

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
     * @brief Очистить выделение
     */
    void clearSelection();

    /**
     * @brief Задать возможность раскрывать узлы дерева двойным кликом
     */
    void setExpandsOnDoubleClick(bool _expand);

    /**
     * @brief Развёрнут ли заданный элемент
     */
    bool isExpanded(const QModelIndex& _index) const;

    /**
     * @brief Развернуть/свернуть заданный элемент
     */
    void expand(const QModelIndex& _index);
    void collapse(const QModelIndex& _index);

    /**
     * @brief Развернуть/свернуть все элементы
     */
    void expandAll();
    void collapseAll();

    /**
     * @brief Установить необходимость пересчитывать размер элементов в делегате
     */
    void setAutoAdjustSize(bool _auto);

    /**
     * @brief Получить область занимаемую элементом с заданным индексом
     */
    QRect visualRect(const QModelIndex& _index) const;

    /**
     * @brief Объединить колонки, для заданной строки
     */
    void setFirstColumnSpanned(int _row, const QModelIndex& _parent, bool _span);

    /**
     * @brief Представление заголовка дерева
     */
    void setHeader(QHeaderView* _headerView);
    QHeaderView* headerView() const;

    /**
     * @brief Получить размер вьюпорта
     */
    QSize viewportSizeHint() const;

    /**
     * @brief Получить вьюпорт дерева
     */
    QWidget* viewport() const;

    /**
     * @brief Задать действия приводящие к редактированию элементов дерева
     */
    void setEditTriggers(QAbstractItemView::EditTriggers _triggers);

    /**
     * @brief Активировать редактирование заданного элемента
     */
    void edit(const QModelIndex& _index);

    /**
     * @brief Загрузить состояние дерева
     */
    void restoreState(const QVariant& _state);

    /**
     * @brief Сохранить состояние дерева
     */
    QVariant saveState() const;

    /**
     * @brief Определить, находится ли заданная позиция над иконкой в конце элемента
     */
    bool isOnItemTrilingIcon(const QPoint& _position) const;

signals:
    /**
     * @brief Изменился текущий индекс
     */
    void currentIndexChanged(const QModelIndex& _index);

    /**
     * @brief Пользователь кликнул на заданном элементе
     */
    void clicked(const QModelIndex& _index, bool _firstClick);

    /**
     * @brief Пользователь сделал двойной клик на элемента
     */
    void doubleClicked(const QModelIndex& _index);

    /**
     * @brief Элемент был раскрыт
     */
    void expanded(const QModelIndex& _index);

    /**
     * @brief Элемент был свёрнут
     */
    void collapsed(const QModelIndex& _index);

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
