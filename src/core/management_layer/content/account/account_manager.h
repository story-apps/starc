#pragma once

#include <QObject>

namespace Domain {
enum class SubscriptionType;
struct PaymentOption;
struct SessionInfo;
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
     * @brief Установить параметры аккаунта
     */
    void setAccountInfo(const QString& _email, const QString& _name, const QString& _description,
                        const QByteArray& _avatar, Domain::SubscriptionType _subscriptionType,
                        const QDateTime& _subscriptionEnds,
                        const QVector<Domain::PaymentOption>& _paymentOptions,
                        const QVector<Domain::SessionInfo>& _sessions);
    void clearAccountInfo();
    QString email() const;
    QString name() const;
    QPixmap avatar() const;

    /**
     * @brief Проапгрейдить учётную любым из способов
     */
    void upgradeAccount();

    /**
     * @brief Активировать бесплатный период для ПРО
     */
    bool tryProForFree();

    /**
     * @brief Оплатить ПРО версию
     */
    void upgradeToPro();
    void buyProLifetme();
    void renewPro();

    /**
     * @brief Показать сообщение активации промокода
     */
    void showPromocodeActivationMessage(const QString& _message);

    /**
     * @brief Задать ошибку промокода
     */
    void setPromocodeError(const QString& _error);

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
     * @brief Пользователь хочет получить информацию об аккаунте
     */
    void updateAccountInfoRequested(const QString& _name, const QString& _description,
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
     * @brief Пользователь хочет активировать промокод
     */
    void activatePromocodeRequested(const QString& _promocode);


    // =============================================
    // LEGACY

    /**
     * @brief Изменилась возможность создания проектов в облаке
     * @param _authorized - авторизован ли пользователь
     * @param _ableToCreate - может ли пользователь создавать новые проекты (активна ли подписка)
     */
    void cloudProjectsCreationAvailabilityChanged(bool _authorized, bool _ableToCreate);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
