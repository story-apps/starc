#pragma once

#include <ui/widgets/widget/widget.h>

namespace Domain {
struct PaymentOption;
}

namespace Ui {

/**
 * @brief Виджет с параметрами платёжной опции
 */
class PurchaseDialogOptionWidget : public Widget
{
    Q_OBJECT

public:
    explicit PurchaseDialogOptionWidget(QWidget* _parent = nullptr);
    ~PurchaseDialogOptionWidget() override;

    void setPaymentOption(const Domain::PaymentOption& _paymentOption);
    const Domain::PaymentOption& paymentOption() const;

    void setWide(bool _isWide);
    void setShowTotal(bool _showTotal);

    bool isChecked() const;
    void setChecked(bool _checked);

    QSize sizeHint() const override;

signals:
    void checkedChanged(bool _checked);

protected:
    void paintEvent(QPaintEvent* _event) override;

    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
