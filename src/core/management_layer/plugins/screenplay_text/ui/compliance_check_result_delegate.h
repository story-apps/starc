#pragma once

#include <QStyledItemDelegate>


namespace Ui {

/**
 * @brief Делегат для отрисовки списка результатов проверки требований к сценарию
 */
class ComplianceCheckResultDelegate : public QStyledItemDelegate
{
public:
    explicit ComplianceCheckResultDelegate(QObject* _parent = nullptr);
    ~ComplianceCheckResultDelegate() override;

    /**
     * @brief Задать необходимость отображать длительность сцены в восьмушках
     */
    void setUseEighths(bool _use);

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
