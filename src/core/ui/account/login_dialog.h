#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui
{

/**
 * @brief Диалог регистрации/авторизации в личном кабинете пользователя
 */
class LoginDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* _parent = nullptr);
    ~LoginDialog() override;

    /**
     * @brief Email, который ввёл пользователь
     */
    QString email() const;

    /**
     * @brief Пароль, который ввёл пользователь
     */
    QString password() const;

    /**
     * @brief Показать кнопку регистрации пользователя
     */
    void showRegistrationButton();

    /**
     * @brief Показать поле для ввода кода подтвержения регистрации
     */
    void showRegistrationConfirmationCodeField();

    /**
     * @brief Задать ошибку ввода проверочного кода для регистрации
     */
    void setRegistrationConfirmationError(const QString& _error);

    /**
     * @brief Показать кнопки авторизации
     */
    void showLoginButtons();

    /**
     * @brief Показать поле ввода кода подтврждения смены пароля
     */
    void showRestorePasswordConfirmationCodeField();

    /**
     * @brief Задать ошибку ввода проверочного кода для смены пароля
     */
    void setRestorePasswordConfirmationError(const QString& _error);

    /**
     * @brief Показать поле ввода нового пароля и кнопку смены пароля
     */
    void showChangePasswordFiledAndButton();

    /**
     * @brief Задать ошибку ввода пароля
     */
    void setPasswordError(const QString& _error);

signals:
    /**
     * @brief Пользователь ввёл свой email
     */
    void emailEntered(const QString& _email);

    /**
     * @brief Пользователь хочет восстановить пароль
     */
    void restorePasswordRequired(const QString& _email);

    /**
     * @brief Введён код подтверждения регистрации
     */
    void passwordRestoringConfirmationCodeEntered(const QString& _email, const QString& _code);

    /**
     * @brief Пользователь хочет сменить пароль
     */
    void changePasswordRequested(const QString& _email, const QString& _code, const QString& _password);

    /**
     * @brief Пользователь хочет зарегистрироваться
     */
    void registrationRequired(const QString& _email, const QString& _password);

    /**
     * @brief Введён код подтверждения регистрации
     */
    void registrationConfirmationCodeEntered(const QString& _email, const QString& _code);

    /**
     * @brief Пользователь хочет авторизоваться
     */
    void loginRequired(const QString& _email, const QString& _password);

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

    /**
     * @brief Переопределяем, для ручной корректировки цепочки фокусирования виджетов
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
