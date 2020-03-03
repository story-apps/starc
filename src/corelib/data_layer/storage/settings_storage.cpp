#include "settings_storage.h"

#include <business_layer/templates/screenplay_template.h>

#include <data_layer/mapper/mapper_facade.h>
#include <data_layer/mapper/settings_mapper.h>

#include <ui/design_system/design_system.h>

#include <QDir>
#include <QLocale>
#include <QStandardPaths>

using DataMappingLayer::MapperFacade;


namespace DataStorageLayer
{

namespace {
    /**
     * @brief Имя пользователя из системы
     */
    static QString systemUserName() {
        QString name = qgetenv("USER");
        if (name.isEmpty()) {
            //
            // Windows
            //
            name = QString::fromLocal8Bit(qgetenv("USERNAME"));
            if (name.isEmpty()) {
                name = "user";
            }
        }
        return name;
    }
}

class SettingsStorage::Implementation
{
public:
    Implementation();

    /**
     * @brief Загрузить параметр из кэша
     */
    QVariant cachedValue(const QString& _key, SettingsPlace _settingsPlace, bool& _ok) const;

    /**
     * @brief Сохранить параметр в кэше
     */
    void cacheValue(const QString& _key, const QVariant& _value, SettingsPlace _settingsPlace);


    /**
     * @brief Настройки приложения
     */
    QSettings appSettings;

    /**
     * @brief Значения параметров по умолчанию
     */
    QVariantMap defaultValues;

    /**
     * @brief Кэшированные значения параметров
     */
    /** @{ */
    QVariantMap cachedValuesApp;
    QVariantMap cachedValuesDb;
    /** @} */
};

SettingsStorage::Implementation::Implementation()
{
    using namespace BusinessLayer;

    //
    // Настроим значения параметров по умолчанию
    //

    //
    // Для приложения
    //
    defaultValues.insert(kApplicationConfiguredKey, false);
    defaultValues.insert(kApplicationLanguagedKey, QLocale::AnyLanguage);
    defaultValues.insert(kApplicationThemeKey, static_cast<int>(Ui::ApplicationTheme::DarkAndLight));
    defaultValues.insert(kApplicationCustomThemeColorsKey,
                         "e4e4e438393a448affffffffffffff000000ffffff000000b00020ffffff000000c8c8c8");
    defaultValues.insert(kApplicationScaleFactorKey, 1.0);
    defaultValues.insert(kApplicationTypewriterSoundEnabledKey, false);
    defaultValues.insert(kApplicationUseAutoSaveKey, true);
    defaultValues.insert(kApplicationSaveBackupsKey, true);
    defaultValues.insert(kApplicationBackupsFolderKey,
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/starc/backups");
    defaultValues.insert(kProjectSaveFolderKey,
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/starc/projects");
    defaultValues.insert(kProjectImportFolderKey,
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    //
    // Параметры редактора сценария
    //
    auto addScreenplayEditorStylesAction
            = [this] (const QString& _actionType, const QString& _actionKey,
                      ScreenplayParagraphType _from, ScreenplayParagraphType _to) {
        defaultValues.insert(
                    QString("screenplay-editor/styles-%1/from-%2-by-%3").arg(_actionType, toString(_from), _actionKey),
                    toString(_to));
    };
    auto addScreenplayEditorStylesActionByTab
            = [addScreenplayEditorStylesAction] (const QString& _actionType,
                                                 ScreenplayParagraphType _from, ScreenplayParagraphType _to) {
        addScreenplayEditorStylesAction(_actionType, "tab", _from, _to);
    };
    auto addScreenplayEditorStylesActionByEnter
            = [addScreenplayEditorStylesAction] (const QString& _actionType,
                                                 ScreenplayParagraphType _from, ScreenplayParagraphType _to) {
        addScreenplayEditorStylesAction(_actionType, "enter", _from, _to);
    };
    //
    auto addScreenplayEditorStylesJumpByTab
            = [addScreenplayEditorStylesActionByTab] (ScreenplayParagraphType _from, ScreenplayParagraphType _to) {
        addScreenplayEditorStylesActionByTab("jumping", _from, _to);
    };
    auto addScreenplayEditorStylesJumpByEnter
            = [addScreenplayEditorStylesActionByEnter] (ScreenplayParagraphType _from, ScreenplayParagraphType _to) {
        addScreenplayEditorStylesActionByEnter("jumping", _from, _to);
    };
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::UnformattedText,
                                       ScreenplayParagraphType::UnformattedText);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::UnformattedText,
                                         ScreenplayParagraphType::UnformattedText);
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::SceneName,
                                       ScreenplayParagraphType::SceneCharacters);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::SceneName,
                                         ScreenplayParagraphType::SceneDescription);
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::SceneHeading,
                                       ScreenplayParagraphType::SceneCharacters);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::SceneHeading,
                                         ScreenplayParagraphType::Action);
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::SceneCharacters,
                                       ScreenplayParagraphType::Action);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::SceneCharacters,
                                         ScreenplayParagraphType::Action);
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Action,
                                       ScreenplayParagraphType::Character);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Action,
                                         ScreenplayParagraphType::Action);
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Character,
                                       ScreenplayParagraphType::Parenthetical);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Character,
                                         ScreenplayParagraphType::Dialogue);
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Parenthetical,
                                       ScreenplayParagraphType::Dialogue);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Parenthetical,
                                         ScreenplayParagraphType::Dialogue);
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Dialogue,
                                       ScreenplayParagraphType::Parenthetical);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Dialogue,
                                         ScreenplayParagraphType::Action);
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Lyrics,
                                       ScreenplayParagraphType::Parenthetical);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Lyrics,
                                         ScreenplayParagraphType::Action);
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Transition,
                                       ScreenplayParagraphType::SceneHeading);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Transition,
                                         ScreenplayParagraphType::SceneHeading);
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::Shot,
                                       ScreenplayParagraphType::Action);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::Shot,
                                         ScreenplayParagraphType::Action);
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::InlineNote,
                                       ScreenplayParagraphType::Action);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::InlineNote,
                                         ScreenplayParagraphType::Action);
    addScreenplayEditorStylesJumpByTab(ScreenplayParagraphType::FolderHeader,
                                       ScreenplayParagraphType::SceneHeading);
    addScreenplayEditorStylesJumpByEnter(ScreenplayParagraphType::FolderHeader,
                                         ScreenplayParagraphType::SceneHeading);
    //
    auto addScreenplayEditorStylesChangeByTab
            = [addScreenplayEditorStylesActionByTab] (ScreenplayParagraphType _from, ScreenplayParagraphType _to) {
        addScreenplayEditorStylesActionByTab("changing", _from, _to);
    };
    auto addScreenplayEditorStylesChangeByEnter
            = [addScreenplayEditorStylesActionByEnter] (ScreenplayParagraphType _from, ScreenplayParagraphType _to) {
        addScreenplayEditorStylesActionByEnter("changing", _from, _to);
    };
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::UnformattedText,
                                         ScreenplayParagraphType::UnformattedText);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::UnformattedText,
                                           ScreenplayParagraphType::UnformattedText);
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::SceneName,
                                         ScreenplayParagraphType::SceneDescription);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::SceneName,
                                           ScreenplayParagraphType::SceneName);
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::SceneHeading,
                                         ScreenplayParagraphType::Action);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::SceneHeading,
                                           ScreenplayParagraphType::SceneHeading);
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::SceneCharacters,
                                         ScreenplayParagraphType::Action);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::SceneCharacters,
                                           ScreenplayParagraphType::SceneCharacters);
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Action,
                                         ScreenplayParagraphType::Character);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Action,
                                           ScreenplayParagraphType::SceneHeading);
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Character,
                                         ScreenplayParagraphType::Action);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Character,
                                           ScreenplayParagraphType::Action);
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Parenthetical,
                                         ScreenplayParagraphType::Dialogue);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Parenthetical,
                                           ScreenplayParagraphType::Parenthetical);
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Dialogue,
                                         ScreenplayParagraphType::Parenthetical);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Dialogue,
                                           ScreenplayParagraphType::Action);
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Lyrics,
                                         ScreenplayParagraphType::Parenthetical);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Lyrics,
                                           ScreenplayParagraphType::Action);
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Transition,
                                         ScreenplayParagraphType::Action);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Transition,
                                           ScreenplayParagraphType::Transition);
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::Shot,
                                         ScreenplayParagraphType::Action);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::Shot,
                                           ScreenplayParagraphType::Shot);
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::InlineNote,
                                         ScreenplayParagraphType::InlineNote);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::InlineNote,
                                           ScreenplayParagraphType::InlineNote);
    addScreenplayEditorStylesChangeByTab(ScreenplayParagraphType::FolderHeader,
                                         ScreenplayParagraphType::FolderHeader);
    addScreenplayEditorStylesChangeByEnter(ScreenplayParagraphType::FolderHeader,
                                           ScreenplayParagraphType::FolderHeader);
    //

    defaultValues.insert(kSystemUsernameKey, systemUserName());
}

QVariant SettingsStorage::Implementation::cachedValue(const QString& _key,
    SettingsStorage::SettingsPlace _settingsPlace, bool& _ok) const
{
    const QVariantMap& cachedValues
            = _settingsPlace == SettingsPlace::Application ? cachedValuesApp : cachedValuesDb;
    _ok = cachedValues.contains(_key);
    return cachedValuesApp.value(_key);
}

void SettingsStorage::Implementation::cacheValue(const QString& _key, const QVariant& _value,
    SettingsStorage::SettingsPlace _settingsPlace)
{
    QVariantMap& cachedValues
            = _settingsPlace == SettingsPlace::Application ? cachedValuesApp : cachedValuesDb;
    cachedValues.insert(_key, _value);
}


// ****


SettingsStorage::~SettingsStorage() = default;

void SettingsStorage::setValue(const QString& _key, const QVariant& _value,
    SettingsStorage::SettingsPlace _settingsPlace)
{
    //
    // Кэшируем значение
    //
    d->cacheValue(_key, _value, _settingsPlace);

    //
    // Сохраняем его в заданное хранилище
    //
    if (_settingsPlace == SettingsPlace::Application) {
        d->appSettings.setValue(_key.toUtf8().toHex(), _value);
        d->appSettings.sync();
    } else {
        MapperFacade::settingsMapper()->setValue(_key, _value.toString());
    }
}

void SettingsStorage::setValues(const QString& _valuesGroup, const QVariantMap& _values,
    SettingsStorage::SettingsPlace _settingsPlace)
{
    //
    // Кэшируем значение
    //
    d->cacheValue(_valuesGroup, _values, _settingsPlace);

    //
    // Сохраняем его в заданное хранилище
    //
    if (_settingsPlace == SettingsPlace::Application) {
        //
        // Очистим группу
        //
        {
            d->appSettings.beginGroup(_valuesGroup);
            d->appSettings.remove("");
            d->appSettings.endGroup();
        }

        //
        // Откроем группу
        //
        d->appSettings.beginGroup(_valuesGroup);

        //
        // Сохраним значения
        //
        for (const QString& key : _values.keys()) {
            d->cacheValue(key, _values.value(key), _settingsPlace);
            d->appSettings.setValue(key.toUtf8().toHex(), _values.value(key));
        }

        //
        // Закроем группу
        //
        d->appSettings.endGroup();

        //
        // Сохраняем изменения в файл
        //
        d->appSettings.sync();
    }
    //
    // В базу данных карта параметров не умеет сохраняться
    //
    else {
        Q_ASSERT_X(0, Q_FUNC_INFO, "Database settings can't save group of settings");
    }

}

QVariant SettingsStorage::value(const QString& _key, SettingsStorage::SettingsPlace _settingsPlace, const QVariant& _defaultValue) const
{
    //
    // Пробуем получить значение из кэша
    //
    bool hasCachedValue = false;
    QVariant value = d->cachedValue(_key, _settingsPlace, hasCachedValue);

    //
    // Если в кэше нашлось нужное значение, вернём его
    //
    if (hasCachedValue) {
        return value;
    }

    //
    // Если в кэше нет, то загружаем из указанного места
    //
    if (_settingsPlace == SettingsPlace::Application) {
        value = d->appSettings.value(_key.toUtf8().toHex(), QVariant());
    } else {
        value = MapperFacade::settingsMapper()->value(_key);
    }

    //
    // Если удалось загрузить
    //
    if (!value.isNull()) {
        //
        // Сохраняем значение в кэш
        //
        d->cacheValue(_key, value, _settingsPlace);

        return value;
    }

    //
    // Если параметр не задан, то используем заданное значение по умолчанию
    //
    if (!_defaultValue.isNull()) {
        return _defaultValue;
    }

    //
    // В конце концов пробуем вытащить что-нибудь из предустановленных умолчальных значений
    //
    return d->defaultValues.value(_key);
}

QVariantMap SettingsStorage::values(const QString& _valuesGroup, SettingsStorage::SettingsPlace _settingsPlace)
{
    //
    // Пробуем получить значение из кэша
    //
    bool hasCachedValue = false;
    QVariantMap values = d->cachedValue(_valuesGroup, _settingsPlace, hasCachedValue).toMap();

    if (hasCachedValue) {
        return values;
    }

    //
    // Если в кэше нет, то загружаем из указанного места
    //
    if (_settingsPlace == SettingsPlace::Application) {
        //
        // Откроем группу для считывания
        //
        d->appSettings.beginGroup(_valuesGroup);

        //
        // Получим все ключи
        //
        QStringList keys = d->appSettings.childKeys();

        //
        // Получим все значения
        //
        for (const QString& key : keys) {
             values.insert(QByteArray::fromHex(key.toUtf8()), d->appSettings.value(key));
        }

        //
        // Закроем группу
        //
        d->appSettings.endGroup();
    }
    //
    // Из базы данных карта параметров не умеет загружаться
    //
    else {
        Q_ASSERT_X(0, Q_FUNC_INFO, "Database settings can't load group of settings");
    }

    //
    // Сохраняем значение в кэш
    //
    d->cacheValue(_valuesGroup, values, _settingsPlace);

    return values;
}

QString SettingsStorage::userName() const
{
    //
    // TODO: получать имя пользователя если он авторизован
    //

    return value(kSystemUsernameKey, SettingsPlace::Application).toString();
}

QString SettingsStorage::userEmail() const
{
    //
    // TODO
    //

    return {};
}

QString SettingsStorage::documentFolderPath(const QString& _key) const
{
    QString folderPath = value(_key, SettingsPlace::Application).toString();
    if (folderPath.isEmpty()) {
        folderPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    return folderPath;
}

QString SettingsStorage::documentFilePath(const QString& _key, const QString& _fileName) const
{
    const QString filePath = documentFolderPath(_key) + QDir::separator() + _fileName;
    return QDir::toNativeSeparators(filePath);
}

void SettingsStorage::setDocumentFolderPath(const QString& _key, const QString& _filePath)
{
    setValue(_key, QFileInfo(_filePath).absoluteDir().absolutePath(), SettingsPlace::Application);
}

SettingsStorage::SettingsStorage()
    : d(new Implementation)
{
}

} // namespace DataStorageLayer
