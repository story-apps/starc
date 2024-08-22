#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

class CompareSubscriptionsDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit CompareSubscriptionsDialog(QWidget* _parent = nullptr);
    ~CompareSubscriptionsDialog() override;

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
    void purchasePressed();

    /**
     * @brief Пользователь передумал вносить платёж
     */
    void canceled();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
