#pragma once

#include <ui/widgets/widget/widget.h>

namespace Domain {
struct AccountInfo;
} // namespace Domain


namespace Ui {

/**
 * @brief Навигатор личного кабинета
 */
class AccountNavigator : public Widget
{
    Q_OBJECT

public:
    explicit AccountNavigator(QWidget* _parent = nullptr);
    ~AccountNavigator() override;

    /**
     * @brief Скорректировать интерфейс в зависимости от того есть ли подключение к серверу
     */
    void setConnected(bool _connected);

    /**
     * @brief Задать информацию о подписке
     */
    void setAccountInfo(const Domain::AccountInfo& _account);

signals:
    /**
     * @brief Пользователь хочет перейти в отображению заданных настроек
     */
    void accountPressed();
    void subscriptionPressed();
    void sessionsPressed();

    /**
     * @brief Пользователь хочет проапгрейдить аккаунт
     */
    void tryProForFreePressed();
    void buyProLifetimePressed();
    void renewProPressed();
    void tryTeamForFreePressed();
    void renewTeamPressed();

    /**
     * @brief Пользователь хочет выйти из личного кабинета
     */
    void logoutPressed();

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем навигатор при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
