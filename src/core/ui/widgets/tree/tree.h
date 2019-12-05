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

protected:
    /**
     * @brief Корректируем внешний вид виджета дерева и его делегата
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
