#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

namespace Domain {
struct AccountInfo;
struct PaymentOption;
struct SessionInfo;
struct SubscriptionInfo;
struct TeamInfo;
} // namespace Domain


namespace Ui {

/**
 * @brief Представление с данными личного кабинета
 */
class AccountView : public StackWidget
{
    Q_OBJECT

public:
    explicit AccountView(QWidget* _parent = nullptr);
    ~AccountView() override;

    /**
     * @brief Показать страницу с параметрами аккаунта
     */
    void showAccountPage();

    /**
     * @brief Показать страницу с параметрами команд
     */
    void showTeamPage();

    //
    // По возможности сфокусировать на экране заданный виджет
    //
    void showAccount();
    void showSubscription();
    void showSessions();

    /**
     * @brief Скорректировать интерфейс в зависимости от того есть ли подключение к серверу
     */
    void setConnected(bool _connected);

    /**
     * @brief Установить имейл пользователя
     */
    void setEmail(const QString& _email);

    /**
     * @brief Имя пользователя
     */
    void setName(const QString& _name);

    /**
     * @brief Био пользователя
     */
    void setDescription(const QString& _description);

    /**
     * @brief Подписан ли пользователь на рассылку
     */
    void setNewsletterSubscribed(bool _subscribed);

    /**
     * @brief Аватар пользователя
     */
    void setAvatar(const QPixmap& _avatar);

    /**
     * @brief Задать информацию о подписке
     */
    void setAccountInfo(const Domain::AccountInfo& _account);

    /**
     * @brief Задать текущие подписки пользователя
     */
    void setSubscriptions(const QVector<Domain::SubscriptionInfo>& _subscriptions);

    /**
     * @brief Задать опции покупки
     */
    void setPaymentOptions(const QVector<Domain::PaymentOption>& _paymentOptions);

    /**
     * @brief Очистить промокод
     */
    void clearPromocode();

    /**
     * @brief Задать ошибку промокода
     */
    void setPromocodeError(const QString& _error);

    /**
     * @brief Задать список сессий аккаунта
     */
    void setSessions(const QVector<Domain::SessionInfo>& _sessions);

    //

    /**
     * @brief Задать список команд пользоватля
     */
    void setAccountTeams(const QVector<Domain::TeamInfo>& _teams);

    /**
     * @brief Выбрать заданную команду
     */
    void showTeam(int _teamId);

signals:
    /**
     * @brief Пользователь изменил своё имя
     */
    void nameChanged(const QString& _name);

    /**
     * @brief Пользователь изменил своё био
     */
    void descriptionChanged(const QString& _description);

    /**
     * @brief Пользователь подписался/отписался от рассылки
     */
    void newsletterSubscriptionChanged(bool _subscribed);

    /**
     * @brief Пользователь изменил аватарку
     */
    void avatarChanged(const QPixmap& avatar);

    /**
     * @brief Пользователь нажал кнопку обновления подписки
     */
    void tryProForFreePressed();
    void buyProLifetimePressed();
    void renewProPressed();
    void giftProPressed();
    void tryCreatorForFreePressed();
    void renewCreatorPressed();
    void giftCreatorPressed();

    /**
     * @brief Пользователь нажал кнопку активировать промокод
     */
    void activatePromocodePressed(const QString& _promocode);

    /**
     * @brief Пользователь хочет завершить заданную сессию
     */
    void terminateSessionRequested(const QString& _sessionKey);

    //

    /**
     * @brief Пользователь хочет добавить участника в коменду
     */
    void addMemberRequested(int _teamId, const QString& _email, const QString& _nameForTeam,
                            const QColor& _color);

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

protected:
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
