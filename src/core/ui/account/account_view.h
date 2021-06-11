#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Представление с данными личного кабинета
 */
class AccountView : public Widget
{
    Q_OBJECT

public:
    explicit AccountView(QWidget* _parent = nullptr);
    ~AccountView() override;

    /**
     * @brief Установить имейл пользователя
     */
    void setEmail(const QString& _email);

    /**
     * @brief Установить имя пользователя
     */
    void setUserName(const QString& _userName);

    /**
     * @brief Установить необходимость получать уведомления по почте
     */
    void setReceiveEmailNotifications(bool _receive);

    /**
     * @brief Установить аватар пользователя
     */
    void setAvatar(const QPixmap& _avatar);

signals:
    /**
     * @brief Пользователь хочет сменить пароль
     */
    void changePasswordPressed();

    /**
     * @brief Пользователь хочет выйти из аккаунта
     */
    void logoutPressed();

    /**
     * @brief Пользователь изменил своё имя
     */
    void userNameChanged(const QString& _userName);

    /**
     * @brief Пользователь изменил желание получать уведомления по почте
     */
    void receiveEmailNotificationsChanged(bool _receive);

    /**
     * @brief Пользователь хочет сменить аватарку
     */
    void avatarChoosePressed();

protected:
    /**
     * @brief Переопределяем для корректировки положения тулбара действий над проектами
     */
    void resizeEvent(QResizeEvent* _event) override;

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
