#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui
{

/**
 * @brief Диалог создания нового проекта
 */
class CreateProjectDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit CreateProjectDialog(QWidget* _parent = nullptr);
    ~CreateProjectDialog() override;

    /**
     * @brief Настроить возможность создавать проекты в облаке
     */
    void configureCloudProjectCreationAbility(bool _isLogged, bool _isSubscriptionActive);

signals:
    /**
     * @brief Пользователь нажал ссылку для авторизации
     */
    void loginPressed();

    /**
     * @brief Пользователь нажал ссылку для продления подписки
     */
    void renewSubscriptionPressed();

    /**
     * @brief Пользователь нажал на кнопку создания проекта
     */
    void createProjectPressed();

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
