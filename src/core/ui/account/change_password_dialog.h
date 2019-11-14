#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui
{

/**
 * @brief Диалог смены пароля
 */
class ChangePasswordDialog : public AbstractDialog
{
    Q_OBJECT

public:
    ChangePasswordDialog(QWidget* _parent = nullptr);
    ~ChangePasswordDialog() override;

    /**
     * @brief Код подтверждения смены пароля, который ввёл пользователь
     */
    QString code() const;

    /**
     * @brief Пароль, который ввёл пользователь
     */
    QString password() const;

    /**
     * @brief Задать ошибку ввода проверочного кода для смены пароля
     */
    void setConfirmationError(const QString& _error);

    /**
     * @brief Показать поле ввода нового пароля и кнопку смены пароля
     */
    void showChangePasswordFieldAndButton();

signals:
    /**
     * @brief Введён код подтверждения регистрации
     */
    void confirmationCodeEntered();

    /**
     * @brief Пользователь хочет сменить пароль
     */
    void changePasswordRequested();

    /**
     * @brief Пользователь передумал менять пароль
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
