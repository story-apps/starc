#pragma once

#include <QStyledItemDelegate>


namespace Ui {

/**
 * @brief Делегат для отрисовки закладки
 */
class BookmarkDelegate : public QStyledItemDelegate
{
public:
    explicit BookmarkDelegate(QObject* _parent = nullptr);

    /**
     * @brief Реализуем собственную отрисовку
     */
    void paint(QPainter* _painter, const QStyleOptionViewItem& _option,
               const QModelIndex& _index) const override;
    QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;
};

} // namespace Ui
