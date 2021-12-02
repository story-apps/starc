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
     * @brief Пользователь изменил аватарку
     */
    void avatarChanged(const QPixmap& avatar);

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
