#pragma once

#include <corelib_global.h>

#include <QStyledItemDelegate>


/**
 * @brief Базовый делегат отрисовки элементов дерева
 */
class CORE_LIBRARY_EXPORT TreeDelegate : public QStyledItemDelegate
{
public:
    explicit TreeDelegate(QObject* _parent = nullptr);

    /**
     * @brief Реализуем собственную отрисовку
     */
    void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const;
    QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;
};
