#pragma once

#include <QObject>

class PaymentInfo;
class Widget;


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
                        const QByteArray& _avatar);
    QString email() const;
    QString name() const;
    void setName(const QString& _name);
    void setDescription(const QString& _description);
    QPixmap avatar() const;
    void setAvatar(const QByteArray& _avatar);
    void setAvatar(const QPixmap& _avatar);
    void removeAvatar();


    // ========================
    // LEGACY


    /**
     * @brief Завершить выход из аккаунта
     */
    void completeLogout();

    void setPaymentInfo(const PaymentInfo& _info);
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


    // =============================================
    // LEGACY

    //
    // Оплата услуг
    //

    /**
     * @brief Пользователь хочет продлить подписку на облако
     */
    void renewSubscriptionRequested(int _month, int _paymentType);

    //
    // Работа с аккаунтом
    //

    /**
     * @brief Пользователь хочет выйти из аккаунта
     */
    void logoutRequested();

    /**
     * @brief Пользователь хочет сменить имя пользователя
     */
    void changeUserNameRequested(const QString& _userName);

    /**
     * @brief Пользователь хочет отключить/включить получение уведомлений по почте
     */
    void changeReceiveEmailNotificationsRequested(bool _receive);

    /**
     * @brief Пользователь хочет сменить аватар
     */
    void changeAvatarRequested(const QByteArray& _avatar);

    //
    // Информирование о параметрах аккаунта
    //

    /**
     * @brief Изменилась возможность создания проектов в облаке
     * @param _authorized - авторизован ли пользователь
     * @param _ableToCreate - может ли пользователь создавать новые проекты (активна ли подписка)
     */
    void cloudProjectsCreationAvailabilityChanged(bool _authorized, bool _ableToCreate);

private:
    /**
     * @brief Настроить соединения зависящие от действий пользователя в интерфейсе
     */
    void initToolBarConnections();
    void initNavigatorConnections();
    void initViewConnections();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
