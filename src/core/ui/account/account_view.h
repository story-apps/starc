#pragma once

#include <ui/widgets/widget/widget.h>

namespace Domain {
enum class SubscriptionType;
struct PaymentOption;
struct SessionInfo;
} // namespace Domain


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
     * @brief Аватар пользователя
     */
    void setAvatar(const QPixmap& _avatar);

    /**
     * @brief Задать информацию о подписке
     */
    void setSubscriptionInfo(Domain::SubscriptionType _subscriptionType,
                             const QDateTime& _subscriptionEnds,
                             const QVector<Domain::PaymentOption>& _paymentOptions);

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
    void tryForFreePressed();
    void upgradeToProPressed();
    void buyProLifetimePressed();
    void renewProPressed();

    /**
     * @brief Пользователь нажал кнопку активировать промокод
     */
    void activatePromocodePressed(const QString& _promocode);

    /**
     * @brief ПОльзователь хочет завершить заданную сессию
     */
    void terminateSessionRequested(const QString& _sessionKey);

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
