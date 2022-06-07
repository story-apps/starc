#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>

namespace Domain {
struct PaymentOption;
} // namespace Domain


namespace Ui {

/**
 * @brief Диалог покупки услуг
 */
class PurchaseDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit PurchaseDialog(QWidget* _parent = nullptr);
    ~PurchaseDialog() override;

    /**
     * @brief Задать доступные опции покупки
     */
    void setPaymentOptions(const QVector<Domain::PaymentOption>& _paymentOptions);

    /**
     * @brief Выбрать заданную опцию в диалоге
     */
    void selectOption(const Domain::PaymentOption& _option);

signals:
    /**
     * @brief Пользователь нажал кнопку оплаты
     */
    void purchasePressed(const Domain::PaymentOption& _option);

    /**
     * @brief Пользователь передумал вносить платёж
     */
    void canceled();

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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
