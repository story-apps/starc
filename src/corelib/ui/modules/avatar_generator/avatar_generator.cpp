#include "avatar_generator.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/image_helper.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QPixmap>
#include <QPixmapCache>
#include <QStandardPaths>


namespace Ui {

namespace {

/**
 * @brief Получить ключ из имейла
 */
QString keyFromEmail(const QString& _email)
{
    return QCryptographicHash::hash(_email.toUtf8(), QCryptographicHash::Md5).toHex();
}

/**
 * @brief Сохранить аватарку с заданным ключём в кэш
 */
void storeAvatarInCache(const QString& _email, const QPixmap& _avatar)
{
    QPixmapCache::insert(keyFromEmail(_email), _avatar);
}

/**
 * @brief Загрузить аватар из кэша
 */
QPixmap loadAvatarFromCache(const QString& _email)
{
    QPixmap avatar;
    QPixmapCache::find(keyFromEmail(_email), &avatar);
    return avatar;
}

/**
 * @brief Путь к файлу с аватаркой для заданного имейл
 */
QString avatarPath(const QString& _email)
{
    const QString avatarDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/thumbnails/avatars/";
    QDir::root().mkpath(avatarDir);
    return avatarDir + keyFromEmail(_email) + ".png";
}

} // namespace

QPixmap AvatarGenerator::avatar(const QString& _name, const QString& _email)
{
    QPixmap avatar;

    //
    // Если ничего не задано, то возвращаем пустое изображение
    //
    if (_name.isEmpty() && _email.isEmpty()) {
        return avatar;
    }

    //
    // Если нет имейла, то только генерируем аватар из имени пользователя
    //
    if (_email.isEmpty()) {
        avatar = ImageHelper::makeAvatar(_name, Ui::DesignSystem::font().body1(),
                                         Ui::DesignSystem::treeOneLineItem().avatarSize().toSize(),
                                         Qt::white);
        return avatar;
    }

    //
    // Если имейл есть, то пробууем получить из кэша
    //
    avatar = loadAvatarFromCache(_email);
    if (!avatar.isNull()) {
        return avatar;
    }

    //
    // Если в кэше нет, то пробуем загрузить из сохранённого файла
    //
    const auto path = avatarPath(_email);
    if (QFile::exists(path)) {
        avatar.load(path);
        //
        // ... но использовать будем масштабированную версию
        //
        avatar = ImageHelper::makeAvatar(avatar,
                                         Ui::DesignSystem::treeOneLineItem().avatarSize().toSize());
        storeAvatarInCache(_email, avatar);
        return avatar;
    }

    //
    // Если нет подготовленного аватара, то
    //
    // ... положим в очередь событий запрос аватара у внешнего сервиса
    //
    if (!_email.isEmpty()) {
        QMetaObject::invokeMethod(
            instance(), [_email] { emit instance()->avatarRequested(_email); },
            Qt::QueuedConnection);
    }
    //
    // ... и временно сгенерируем аватар на основании текста
    //
    avatar = ImageHelper::makeAvatar(
        !_name.isEmpty() ? _name : _email, Ui::DesignSystem::font().body1(),
        Ui::DesignSystem::treeOneLineItem().avatarSize().toSize(), Qt::white);
    storeAvatarInCache(_email, avatar);
    return avatar;
}

void AvatarGenerator::setAvatar(const QString& _email, const QByteArray& _avatar)
{
    if (_email.isEmpty() || _avatar.isNull()) {
        return;
    }

    //
    // Загрузим изображение аватара
    //
    QPixmap avatar;
    avatar.loadFromData(_avatar);

    //
    // Сохраним полученный аватар в файл
    //
    avatar.save(avatarPath(_email));

    //
    // Использовать будем масштабированную версию
    //
    avatar = ImageHelper::makeAvatar(avatar,
                                     Ui::DesignSystem::treeOneLineItem().avatarSize().toSize());

    //
    // Положим в кэш
    //
    storeAvatarInCache(_email, avatar);

    //
    // И уведомим клиентов о его обновлении
    //
    emit instance()->avatarUpdated(_email, avatar);
}

AvatarGenerator* AvatarGenerator::instance()
{
    static AvatarGenerator instance;
    return &instance;
}

AvatarGenerator::AvatarGenerator(QObject* _parent)
    : QObject(_parent)
{
}

} // namespace Ui
