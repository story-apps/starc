#pragma once

#include <QObject>

namespace Domain {
struct PaymentOption;
struct AccountInfo;
struct TeamInfo;
} // namespace Domain


namespace ManagementLayer {

/**
 * @brief Менеджер личного кабинета пользователя
 */
class AccountManager : public QObject
{
    Q_OBJECT

public:
    AccountManager(QObject* _parent, QWidget* _parentWidget);
    ~AccountManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

    /**
     * @brief Скорректировать интерфейс в зависимости от того есть ли подключение к серверу
     */
    void setConnected(bool _connected);

    /**
     * @brief Войти в личный кабинет
     */
    void signIn();

    /**
     * @brief Задать параметры кода активации
     */
    void setConfirmationCodeInfo(int _codeLength);

    /**
     * @brief Завершить авторизацию (если это новый пользователь, то необходимо перейти в кабинет)
     */
    void completeSignIn(bool _openAccount);

    /**
     * @brief Параметры аккаунта
     */
    const Domain::AccountInfo& accountInfo() const;
    void setAccountInfo(const Domain::AccountInfo& _accountInfo);
    void clearAccountInfo();

    /**
     * @brief Проапгрейдить учётную любым из способов
     */
    void upgradeAccountToPro();
    void upgradeAccountToCloud();

    /**
     * @brief Активировать бесплатный период для ПРО
     */
    bool tryProForFree();

    /**
     * @brief Оплатить ПРО версию
     */
    void buyProLifetme();
    void renewPro();

    /**
     * @brief Активировать бесплатный период для ПРО
     */
    bool tryCloudForFree();

    /**
     * @brief Оплатить ПРО версию
     */
    void renewCloud();

    /**
     * @brief Докупить кредиты
     */
    void buyCredits();

    /**
     * @brief Показать сообщение отправки подарка
     */
    void showGiftSentMessage(const QString& _message);

    /**
     * @brief Показать сообщение активации промокода
     */
    void showPromocodeActivationMessage(const QString& _message);

    /**
     * @brief Задать ошибку промокода
     */
    void setPromocodeError(const QString& _error);

    //

    /**
     * @brief Задать список команд пользоватля
     */
    void setAccountTeams(const QVector<Domain::TeamInfo>& _teams);

    /**
     * @brief Добавить команду пользователя
     */
    void addAccountTeam(const Domain::TeamInfo& _team);

    /**
     * @brief Обновить команду пользователя
     */
    void updateAccountTeam(const Domain::TeamInfo& _team);

    /**
     * @brief Удалить команду пользователя
     */
    void removeAccountTeam(int _teamId);


signals:
    //
    // Диалог авторизации
    //

    /**
     * @brief Email для авторизации был введён пользователем
     */
    void askConfirmationCodeRequested(const QString& _email);

    /**
     * @brief Код проверки авторизации был введён пользователем
     */
    void checkConfirmationCodeRequested(const QString& _code);

    //
    // Отображение/скрытие личного кабинета
    //

    /**
     * @brief Пользователь хочет перейти в личный кабинет
     */
    void showAccountRequested();

    /**
     * @brief Пользователь хочет закрыть личный кабинет
     */
    void closeAccountRequested();

    //
    // Работа с аккаунтом
    //

    /**
     * @brief Запросить информацию об аккаунте
     */
    void askAccountInfoRequested();

    /**
     * @brief Пользователь хочет обновить информацию об аккаунте
     */
    void updateAccountInfoRequested(const QString& _email, const QString& _name,
                                    const QString& _description,
                                    const QString& _subscriptionLanguage, bool _subscribed,
                                    const QByteArray& _avatar);

    /**
     * @brief Пользователь хочет завершить заданную сессию
     */
    void terminateSessionRequested(const QString& _sessionKey);

    /**
     * @brief Пользователь хочет выйти из аккаунта
     */
    void logoutRequested();

    /**
     * @brief Пользователь хочет применить заданную опцию оплаты
     */
    void activatePaymentOptionRequested(const Domain::PaymentOption& _paymentOption);

    /**
     * @brief Пользователь хочет купить заданную опцию оплаты в подарок
     */
    void activatePaymentOptionAsGiftRequested(const Domain::PaymentOption& _paymentOption,
                                              const QString& _email, const QString& _greeting);

    /**
     * @brief Пользователь хочет активировать промокод
     */
    void activatePromocodeRequested(const QString& _promocode);

    //
    // Работа с командами
    //

    /**
     * @brief Пользователь хочет создать команду
     */
    void createTeamRequested(const QString& _name, const QString& _description,
                             const QByteArray& _avatar);

    /**
     * @brief Пользователь хочет изменить информацию о команде
     */
    void updateTeamRequested(int _teamId, const QString& _name, const QString& _description,
                             const QByteArray& _avatar);

    /**
     * @brief Пользоваталь хочет удалить команду
     */
    void removeTeamRequested(int _teamId);

    /**
     * @brief Пользователь хочет выйти из команды
     */
    void exitFromTeamRequested(int _teamId);

    /**
     * @brief Пользователь хочет добавить участника в коменду
     */
    void addMemberRequested(int _teamId, const QString& _email, const QString& _nameForTeam,
                            int _role);

    /**
     * @brief Пользователь хочет изменить параметры участника
     */
    void changeMemberRequested(int _teamId, const QString& _email, const QString& _nameForTeam,
                               int _role, bool _hasAccessToAllProjects,
                               bool _allowGrantAccessToProjects);

    /**
     * @brief Пользователь хочет отписать соавтора от проекта
     */
    void removeMemberRequested(int _teamId, const QString& _email);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
