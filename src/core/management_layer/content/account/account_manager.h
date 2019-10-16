#pragma once

#include <QObject>

class Widget;


namespace ManagementLayer
{

/**
 * @brief Менеджер личного кабинета пользователя
 */
class AccountManager : public QObject
{
    Q_OBJECT

public:
    explicit AccountManager(QObject* _parent, QWidget* _parentWidget);
    ~AccountManager() override;

    Widget* accountBar() const;
    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

    /**
     * @brief Войти в личный кабинет
     */
    void login();

    /**
     * @brief Разрешить пользователю зарегистрироваться
     */
    void allowRegistration();

    /**
     * @brief Подготовить диалог авторизации ко вводу кода подтверждения
     */
    void prepareToEnterRegistrationConfirmationCode();

    /**
     * @brief Задать ошибку ввода проверочного кода при регистрации
     */
    void setRegistrationConfirmationError(const QString& _error);

    /**
     * @brief Разрешить пользователю авторизоваться
     */
    void allowLogin();

    /**
     * @brief Задать ошибку ввода пароля при авторизации
     */
    void setLoginPasswordError(const QString& _error);

    /**
     * @brief Завершить авторизацию
     */
    void completeLogin();

    /**
     * @brief Установить параметры аккаунта
     */
    void setAccountParameters(qint64 _availableSpace, const QString& _email, qint64 _monthPrice,
        bool _needNotify, const QString& _username, const QByteArray& _avatar);

    /**
     * @brief Уведомить о появившемся сетевом соединении
     */
    void notifyConnected();

    /**
     * @brief Уведомить о пропавшем сетевом соединении
     */
    void notifyDisconnected();

signals:
    /**
     * @brief Email для авторизации был введён пользователем
     */
    void emailEntered(const QString& _email);

    /**
     * @brief Пользователь хочет зарегистрироваться
     */
    void registrationRequired(const QString& _email, const QString& _password);

    /**
     * @brief Введён код подтверждения регистрации
     */
    void registrationConfirmationCodeEntered(const QString& email, const QString& _code);

    /**
     * @brief Пользователь хочет авторизоваться
     */
    void loginRequired(const QString& _email, const QString& _password);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
