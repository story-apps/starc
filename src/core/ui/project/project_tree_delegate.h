#pragma once

#include <ui/widgets/tree/tree_delegate.h>


namespace Ui {

class ProjectTreeDelegate : public TreeDelegate
{
    Q_OBJECT

public:
    explicit ProjectTreeDelegate(QObject* _parent = nullptr);

    /**
     * @brief Добавляем отрисовку дополнительных элементов
     */
    void paint(QPainter* _painter, const QStyleOptionViewItem& _option,
               const QModelIndex& _index) const;
};

} // namespace Ui
