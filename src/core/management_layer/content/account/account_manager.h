#pragma once

#include <QObject>

namespace Domain {
enum class SubscriptionType;
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
                        const QVector<Domain::SessionInfo>& _sessions);
    QString email() const;
    QString name() const;
    QPixmap avatar() const;


    // ========================
    // LEGACY


    /**
     * @brief Завершить выход из аккаунта
     */
    void completeLogout();

    //    void setPaymentInfo(const PaymentInfo& _info);
    void setSubscriptionEnd(const QString& _subscriptionEnd);
    void setReceiveEmailNotifications(bool _receive);

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
