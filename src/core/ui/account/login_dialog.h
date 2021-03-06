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
     * @brief Код подтверждения для регистрации
     */
    QString registractionConfirmationCode() const;

    /**
     * @brief Код подтверждения для смены пароля
     */
    QString restorePasswordConfirmationCode() const;

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
    void showChangePasswordFieldAndButton();

    /**
     * @brief Задать ошибку ввода пароля
     */
    void setPasswordError(const QString& _error);

signals:
    /**
     * @brief Пользователь ввёл свой email
     */
    void emailEntered();

    /**
     * @brief Пользователь хочет восстановить пароль
     */
    void restorePasswordRequested();

    /**
     * @brief Введён код подтверждения регистрации
     */
    void passwordRestoringConfirmationCodeEntered();

    /**
     * @brief Пользователь хочет сменить пароль
     */
    void changePasswordRequested();

    /**
     * @brief Пользователь хочет зарегистрироваться
     */
    void registrationRequested();

    /**
     * @brief Введён код подтверждения регистрации
     */
    void registrationConfirmationCodeEntered();

    /**
     * @brief Пользователь хочет авторизоваться
     */
    void loginRequested();

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
     * @brief Опеределим последний фокусируемый виджет в диалоге
     */
    QWidget* lastFocusableWidget() const override;

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
