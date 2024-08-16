#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>

namespace Domain {
struct PaymentOption;
} // namespace Domain


namespace Ui {

/**
 * @brief Диалог оформления подписки в подарок
 */
class PurchaseGiftDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit PurchaseGiftDialog(QWidget* _parent = nullptr);
    ~PurchaseGiftDialog() override;

    /**
     * @brief Задать выбранную в подарок опцию покупки
     */
    void setPaymentOption(const Domain::PaymentOption& _paymentOption);

protected:
    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

    /**
     * @brief Опеределим последний фокусируемый виджет в диалоге
     */
    QWidget* lastFocusableWidget() const override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

signals:
    /**
     * @brief Пользователь нажал кнопку оплаты
     */
    void purchasePressed(const Domain::PaymentOption& _option, const QString& _email,
                         const QString& _greeting);

    /**
     * @brief Пользователь передумал вносить платёж
     */
    void canceled();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
