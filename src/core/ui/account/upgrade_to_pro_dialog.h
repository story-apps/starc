#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

/**
 * @brief Диалог покупки лицензии
 */
class UpgradeToProDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit UpgradeToProDialog(QWidget* _parent = nullptr);
    ~UpgradeToProDialog() override;

signals:
    /**
     * @brief Пользователь нажал кнопку продлить подписку
     */
    void upgradePressed();

    /**
     * @brief Пользователь нажал кнопку восстановления платежа
     */
    void restorePressed();

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
