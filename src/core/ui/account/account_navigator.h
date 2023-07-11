#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

namespace Domain {
struct AccountInfo;
struct TeamInfo;
} // namespace Domain


namespace Ui {

/**
 * @brief Навигатор личного кабинета
 */
class AccountNavigator : public StackWidget
{
    Q_OBJECT

public:
    explicit AccountNavigator(QWidget* _parent = nullptr);
    ~AccountNavigator() override;

    /**
     * @brief Показать страницу с параметрами аккаунта
     */
    void showAccountPage();

    /**
     * @brief Показать страницу с параметрами команд
     */
    void showTeamPage();

    /**
     * @brief Скорректировать интерфейс в зависимости от того есть ли подключение к серверу
     */
    void setConnected(bool _connected);

    /**
     * @brief Задать информацию о подписке
     */
    void setAccountInfo(const Domain::AccountInfo& _account);

    //

    /**
     * @brief Задать список команд пользоватля
     */
    void setAccountTeams(const QVector<Domain::TeamInfo>& _teams);

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
    void tryCloudForFreePressed();
    void renewCloudPressed();

    /**
     * @brief Пользователь нажал кнопку покупки кредитов
     */
    void buyCreditsPressed();

    /**
     * @brief Пользователь хочет выйти из личного кабинета
     */
    void logoutPressed();

    /**
     * @brief Пользователь выбрал команду с заданным идентификатором
     */
    void teamSelected(int _teamId);

    /**
     * @brief Нажата кнопка изменения команды
     */
    void editTeamPressed(int _teamId);

    /**
     * @brief Нажата кнопка удаления команды
     */
    void removeTeamPressed(int _teamId);

    /**
     * @brief Нажата кнопка выхода из команды
     */
    void exitTeamPressed(int _teamId);

    /**
     * @brief Пользователь хочет создать команду
     */
    void createTeamPressed();

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
