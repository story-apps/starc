#pragma once

#include <ui/widgets/widget/widget.h>

class QAbstractItemModel;


/**
 * @brief Виджет дерева элементов
 */
class Tree : public Widget
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
     * @brief Включить/отключить возможность перетаскивания элементов
     */
    void setDragDropEnabled(bool _enabled);

    /**
     * @brief Задать текущий индекс
     */
    void setCurrentIndex(const QModelIndex& _index);

    /**
     * @brief Развернуть все элементы
     */
    void expandAll();

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

protected:
    /**
     * @brief Корректируем внешний вид виджета дерева и его делегата
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
