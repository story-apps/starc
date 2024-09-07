#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

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
     * @brief Показать страницу ввода имейла
     */
    void showEmailStep();

    /**
     * @brief Email, который ввёл пользователь
     */
    QString email() const;

    /**
     * @brief Задать ошибку авторизации
     */
    void setAuthorizationError(const QString& _error);

    /**
     * @brief Показать страницу ввода кода подтверждения
     */
    void showConfirmationCodeStep();

    /**
     * @brief Код подтверждения авторизации
     */
    QString confirmationCode() const;

signals:
    /**
     * @brief Пользователь нажал кнопку входа в аккаунт
     */
    void signInPressed();

    /**
     * @brief Пользователь ввёл код подтверждения
     */
    void confirmationCodeChanged(const QString& _code);

    /**
     * @brief Пользователь передумал авторизовываться
     */
    void cancelPressed();

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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
