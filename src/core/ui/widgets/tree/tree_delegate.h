#pragma once

#include <QStyledItemDelegate>


/**
 * @brief Базовый делегат отрисовки элементов дерева
 */
class TreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit TreeDelegate(QObject* _parent = nullptr);

    void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const;
    QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;

};
