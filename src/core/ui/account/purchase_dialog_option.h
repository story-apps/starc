#pragma once

#include <ui/widgets/widget/widget.h>

namespace Domain {
struct PaymentOption;
}

namespace Ui {

/**
 * @brief Виджет с параметрами платёжной опции
 */
class PurchaseDialogOption : public Widget
{
    Q_OBJECT

public:
    explicit PurchaseDialogOption(const Domain::PaymentOption& _option, QWidget* _parent = nullptr);
    ~PurchaseDialogOption() override;

    const Domain::PaymentOption& paymentOption() const;

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
