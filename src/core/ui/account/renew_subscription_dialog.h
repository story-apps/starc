#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

class RenewSubscriptionDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit RenewSubscriptionDialog(QWidget* _parent = nullptr);
    ~RenewSubscriptionDialog() override;

signals:
    /**
     * @brief Пользователь передумал авторизовываться
     */
    void canceled();

protected:
    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

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
