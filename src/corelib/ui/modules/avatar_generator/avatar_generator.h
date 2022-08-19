#pragma once

#include <QObject>

#include <corelib_global.h>


namespace Ui {

/**
 * @brief Класс управляющий аватарками пользователей
 */
class CORE_LIBRARY_EXPORT AvatarGenerator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Получить аватар для заданного имени пользователя и имейла
     */
    static QPixmap avatar(const QString& _name, const QString& _email);

    /**
     * @brief Сохранить заданный аватар для пользователя
     */
    static void setAvatar(const QString& _email, const QByteArray& _avatar);

signals:
    /**
     * @brief Запросить аватар по имейлу из внешнего хранилища
     */
    void avatarRequested(const QString& _email);

    /**
     * @brief Аватар пользователя с заданным имейлом был обновлён
     */
    void avatarUpdated(const QString& _email, const QPixmap& _avatar);


    //
    // Синглтон
    //

public:
    static AvatarGenerator* instance();

private:
    explicit AvatarGenerator(QObject* _parent = nullptr);
};

} // namespace Ui
