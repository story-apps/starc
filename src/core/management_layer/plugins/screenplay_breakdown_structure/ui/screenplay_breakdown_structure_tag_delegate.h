#pragma once

#include <QStyledItemDelegate>


namespace Ui {

/**
 * @brief Делегат для отрисовки списка сцен
 */
class ScreenplayBreakdownStructureTagDelegate : public QStyledItemDelegate
{
public:
    explicit ScreenplayBreakdownStructureTagDelegate(QObject* _parent = nullptr);
    ~ScreenplayBreakdownStructureTagDelegate() override;

    /**
     * @brief Реализуем собственную отрисовку
     */
    void paint(QPainter* _painter, const QStyleOptionViewItem& _option,
               const QModelIndex& _index) const override;
    QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
