#pragma once

#include <QStyledItemDelegate>


namespace Ui {

/**
 * @brief Делегат для отрисовки списка глав
 */
class SimpleTextStructureDelegate : public QStyledItemDelegate
{
public:
    explicit SimpleTextStructureDelegate(QObject* _parent = nullptr);
    ~SimpleTextStructureDelegate() override;

    /**
     * @brief Задать количество строк текста для отображения
     */
    void setTextLinesSize(int _size);

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
