#pragma once

#include <QStyledItemDelegate>


namespace Ui {

/**
 * @brief Делегат отрисовки имен файлов в списке на импорт
 */
class ImportFileDelegate : public QStyledItemDelegate
{
public:
    explicit ImportFileDelegate(QObject* _parent = nullptr);
    ~ImportFileDelegate() override;

    /**
     * @brief Реализуем собственную отрисовку
     */
    void paint(QPainter* _painter, const QStyleOptionViewItem& _option,
               const QModelIndex& _index) const override;
    QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;
};

} // namespace Ui
