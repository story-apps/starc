#include "settings_storage.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/text_template.h>
#include <data_layer/mapper/mapper_facade.h>
#include <data_layer/mapper/settings_mapper.h>
#include <ui/design_system/design_system.h>

#include <QDir>
#include <QKeySequence>
#include <QLocale>
#include <QStandardPaths>

using DataMappingLayer::MapperFacade;


namespace DataStorageLayer {

namespace {
/**
 * @brief Имя пользователя из системы
 */
static QString systemUserName()
{
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
} // namespace

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
    defaultValues.insert(kApplicationThemeKey, static_cast<int>(Ui::ApplicationTheme::Light));
    defaultValues.insert(
        kApplicationCustomThemeColorsKey,
        "323740ffffff2ab177f8f8f2272b34f8f8f222262ef8f8f2ec3740f8f8f2000000f8f8f2");
    defaultValues.insert(kApplicationScaleFactorKey, 1.0);
    defaultValues.insert(kApplicationUseAutoSaveKey, true);
    defaultValues.insert(kApplicationSaveBackupsKey, true);
    defaultValues.insert(kApplicationBackupsFolderKey,
                         QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                             + "/starc/backups");
    defaultValues.insert(kApplicationUseTypewriterSoundKey, false);
    defaultValues.insert(kApplicationUseSpellCheckerKey, false);
    defaultValues.insert(kApplicationHighlightCurrentLineKey, false);
    defaultValues.insert(kProjectSaveFolderKey,
                         QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                             + "/starc/projects");
    defaultValues.insert(kProjectImportFolderKey,
                         QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    defaultValues.insert(kProjectExportFolderKey,
                         QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    //
    // Параметры редактора текста
    //
    {
        const QString kSimpleTextEditorKey = "simple-text/editor";
        auto addSimpleTextEditorStylesAction
            = [this, kSimpleTextEditorKey](const QString& _actionType, const QString& _actionKey,
                                           TextParagraphType _from, TextParagraphType _to) {
                  defaultValues.insert(
                      QString("%1/styles-%2/from-%3-by-%4")
                          .arg(kSimpleTextEditorKey, _actionType, toString(_from), _actionKey),
                      toString(_to));
              };
        auto addSimpleTextEditorStylesActionByTab
            = [addSimpleTextEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                                TextParagraphType _to) {
                  addSimpleTextEditorStylesAction(_actionType, "tab", _from, _to);
              };
        auto addSimpleTextEditorStylesActionByEnter
            = [addSimpleTextEditorStylesAction](const QString& _actionType, TextParagraphType _from,
                                                TextParagraphType _to) {
                  addSimpleTextEditorStylesAction(_actionType, "enter", _from, _to);
              };
        //
        auto addSimpleTextEditorStylesJumpByTab =
            [addSimpleTextEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addSimpleTextEditorStylesActionByTab("jumping", _from, _to);
            };
        auto addSimpleTextEditorStylesJumpByEnter
            = [addSimpleTextEditorStylesActionByEnter](TextParagraphType _from,
                                                       TextParagraphType _to) {
                  addSimpleTextEditorStylesActionByEnter("jumping", _from, _to);
              };
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Heading1,
                                           TextParagraphType::Heading2);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Heading1, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Heading2,
                                           TextParagraphType::Heading3);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Heading2, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Heading3,
                                           TextParagraphType::Heading4);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Heading3, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Heading4,
                                           TextParagraphType::Heading5);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Heading4, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Heading5,
                                           TextParagraphType::Heading6);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Heading5, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Heading6, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Heading6, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::Text, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::Text, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByTab(TextParagraphType::InlineNote, TextParagraphType::Text);
        addSimpleTextEditorStylesJumpByEnter(TextParagraphType::InlineNote,
                                             TextParagraphType::Text);
        //
        auto addSimpleTextEditorStylesChangeByTab =
            [addSimpleTextEditorStylesActionByTab](TextParagraphType _from, TextParagraphType _to) {
                addSimpleTextEditorStylesActionByTab("changing", _from, _to);
            };
        auto addSimpleTextEditorStylesChangeByEnter
            = [addSimpleTextEditorStylesActionByEnter](TextParagraphType _from,
                                                       TextParagraphType _to) {
                  addSimpleTextEditorStylesActionByEnter("changing", _from, _to);
              };
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Heading1,
                                             TextParagraphType::Heading2);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Heading1,
                                               TextParagraphType::Heading1);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Heading2,
                                             TextParagraphType::Heading3);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Heading2,
                                               TextParagraphType::Heading1);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Heading3,
                                             TextParagraphType::Heading4);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Heading3,
                                               TextParagraphType::Heading2);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Heading4,
                                             TextParagraphType::Heading5);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Heading4,
                                               TextParagraphType::Heading3);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Heading5,
                                             TextParagraphType::Heading6);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Heading5,
                                               TextParagraphType::Heading4);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Heading6, TextParagraphType::Text);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Heading6,
                                               TextParagraphType::Heading5);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::Text,
                                             TextParagraphType::InlineNote);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::Text,
                                               TextParagraphType::Heading6);
        addSimpleTextEditorStylesChangeByTab(TextParagraphType::InlineNote,
                                             TextParagraphType::Text);
        addSimpleTextEditorStylesChangeByEnter(TextParagraphType::InlineNote,
                                               TextParagraphType::InlineNote);
        //
        auto addShortcut = [this, kSimpleTextEditorKey](BusinessLayer::TextParagraphType _type,
                                                        const QString& _shortcut) {
            defaultValues.insert(QString("%1/shortcuts/%2")
                                     .arg(kSimpleTextEditorKey, BusinessLayer::toString(_type)),
                                 QKeySequence(_shortcut).toString(QKeySequence::NativeText));
        };
        addShortcut(BusinessLayer::TextParagraphType::Heading1, "Ctrl+1");
        addShortcut(BusinessLayer::TextParagraphType::Heading2, "Ctrl+2");
        addShortcut(BusinessLayer::TextParagraphType::Heading3, "Ctrl+3");
        addShortcut(BusinessLayer::TextParagraphType::Heading4, "Ctrl+4");
        addShortcut(BusinessLayer::TextParagraphType::Heading5, "Ctrl+5");
        addShortcut(BusinessLayer::TextParagraphType::Heading6, "Ctrl+6");
        addShortcut(BusinessLayer::TextParagraphType::Text, "Ctrl+7");
        addShortcut(BusinessLayer::TextParagraphType::InlineNote, "Ctrl+Esc");
        //
        defaultValues.insert(kComponentsSimpleTextEditorDefaultTemplateKey, "mono_cp_a4");
        //
        // Параметры навигатора сценария
        //
        defaultValues.insert(kComponentsSimpleTextNavigatorShowSceneTextKey, true);
        defaultValues.insert(kComponentsSimpleTextNavigatorSceneTextLinesKey, 1);
    }
    //
    // Параметры редактора сценария
    //
    {
        const QString kScreenplayEditorKey = "screenplay-editor";
        auto addSimpleTextEditorStylesAction
            = [this, kScreenplayEditorKey](const QString& _actionType, const QString& _actionKey,
                                           ScreenplayParagraphType _from,
                                           ScreenplayParagraphType _to) {
                  defaultValues.insert(
                      QString("%1/styles-%2/from-%3-by-%4")
                          .arg(kScreenplayEditorKey, _actionType, toString(_from), _actionKey),
                      toString(_to));
              };
        auto addSimpleTextEditorStylesActionByTab
            = [addSimpleTextEditorStylesAction](const QString& _actionType,
                                                ScreenplayParagraphType _from,
                                                ScreenplayParagraphType _to) {
                  addSimpleTextEditorStylesAction(_actionType, "tab", _from, _to);
              };
        auto addSimpleTextEditorStylesActionByEnter
            = [addSimpleTextEditorStylesAction](const QString& _actionType,
                                                ScreenplayParagraphType _from,
                                                ScreenplayParagraphType _to) {
                  addSimpleTextEditorStylesAction(_actionType, "enter", _from, _to);
              };
        //
        auto addSimpleTextEditorStylesJumpByTab
            = [addSimpleTextEditorStylesActionByTab](ScreenplayParagraphType _from,
                                                     ScreenplayParagraphType _to) {
                  addSimpleTextEditorStylesActionByTab("jumping", _from, _to);
              };
        auto addSimpleTextEditorStylesJumpByEnter
            = [addSimpleTextEditorStylesActionByEnter](ScreenplayParagraphType _from,
                                                       ScreenplayParagraphType _to) {
                  addSimpleTextEditorStylesActionByEnter("jumping", _from, _to);
              };
        addSimpleTextEditorStylesJumpByTab(ScreenplayParagraphType::UnformattedText,
                                           ScreenplayParagraphType::UnformattedText);
        addSimpleTextEditorStylesJumpByEnter(ScreenplayParagraphType::UnformattedText,
                                             ScreenplayParagraphType::UnformattedText);
        addSimpleTextEditorStylesJumpByTab(ScreenplayParagraphType::SceneHeading,
                                           ScreenplayParagraphType::SceneCharacters);
        addSimpleTextEditorStylesJumpByEnter(ScreenplayParagraphType::SceneHeading,
                                             ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesJumpByTab(ScreenplayParagraphType::SceneCharacters,
                                           ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesJumpByEnter(ScreenplayParagraphType::SceneCharacters,
                                             ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesJumpByTab(ScreenplayParagraphType::Action,
                                           ScreenplayParagraphType::Character);
        addSimpleTextEditorStylesJumpByEnter(ScreenplayParagraphType::Action,
                                             ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesJumpByTab(ScreenplayParagraphType::Character,
                                           ScreenplayParagraphType::Parenthetical);
        addSimpleTextEditorStylesJumpByEnter(ScreenplayParagraphType::Character,
                                             ScreenplayParagraphType::Dialogue);
        addSimpleTextEditorStylesJumpByTab(ScreenplayParagraphType::Parenthetical,
                                           ScreenplayParagraphType::Dialogue);
        addSimpleTextEditorStylesJumpByEnter(ScreenplayParagraphType::Parenthetical,
                                             ScreenplayParagraphType::Dialogue);
        addSimpleTextEditorStylesJumpByTab(ScreenplayParagraphType::Dialogue,
                                           ScreenplayParagraphType::Parenthetical);
        addSimpleTextEditorStylesJumpByEnter(ScreenplayParagraphType::Dialogue,
                                             ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesJumpByTab(ScreenplayParagraphType::Lyrics,
                                           ScreenplayParagraphType::Parenthetical);
        addSimpleTextEditorStylesJumpByEnter(ScreenplayParagraphType::Lyrics,
                                             ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesJumpByTab(ScreenplayParagraphType::Transition,
                                           ScreenplayParagraphType::SceneHeading);
        addSimpleTextEditorStylesJumpByEnter(ScreenplayParagraphType::Transition,
                                             ScreenplayParagraphType::SceneHeading);
        addSimpleTextEditorStylesJumpByTab(ScreenplayParagraphType::Shot,
                                           ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesJumpByEnter(ScreenplayParagraphType::Shot,
                                             ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesJumpByTab(ScreenplayParagraphType::InlineNote,
                                           ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesJumpByEnter(ScreenplayParagraphType::InlineNote,
                                             ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesJumpByTab(ScreenplayParagraphType::FolderHeader,
                                           ScreenplayParagraphType::SceneHeading);
        addSimpleTextEditorStylesJumpByEnter(ScreenplayParagraphType::FolderHeader,
                                             ScreenplayParagraphType::SceneHeading);
        //
        auto addSimpleTextEditorStylesChangeByTab
            = [addSimpleTextEditorStylesActionByTab](ScreenplayParagraphType _from,
                                                     ScreenplayParagraphType _to) {
                  addSimpleTextEditorStylesActionByTab("changing", _from, _to);
              };
        auto addSimpleTextEditorStylesChangeByEnter
            = [addSimpleTextEditorStylesActionByEnter](ScreenplayParagraphType _from,
                                                       ScreenplayParagraphType _to) {
                  addSimpleTextEditorStylesActionByEnter("changing", _from, _to);
              };
        addSimpleTextEditorStylesChangeByTab(ScreenplayParagraphType::UnformattedText,
                                             ScreenplayParagraphType::UnformattedText);
        addSimpleTextEditorStylesChangeByEnter(ScreenplayParagraphType::UnformattedText,
                                               ScreenplayParagraphType::UnformattedText);
        addSimpleTextEditorStylesChangeByTab(ScreenplayParagraphType::SceneHeading,
                                             ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesChangeByEnter(ScreenplayParagraphType::SceneHeading,
                                               ScreenplayParagraphType::SceneHeading);
        addSimpleTextEditorStylesChangeByTab(ScreenplayParagraphType::SceneCharacters,
                                             ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesChangeByEnter(ScreenplayParagraphType::SceneCharacters,
                                               ScreenplayParagraphType::SceneCharacters);
        addSimpleTextEditorStylesChangeByTab(ScreenplayParagraphType::Action,
                                             ScreenplayParagraphType::Character);
        addSimpleTextEditorStylesChangeByEnter(ScreenplayParagraphType::Action,
                                               ScreenplayParagraphType::SceneHeading);
        addSimpleTextEditorStylesChangeByTab(ScreenplayParagraphType::Character,
                                             ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesChangeByEnter(ScreenplayParagraphType::Character,
                                               ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesChangeByTab(ScreenplayParagraphType::Parenthetical,
                                             ScreenplayParagraphType::Dialogue);
        addSimpleTextEditorStylesChangeByEnter(ScreenplayParagraphType::Parenthetical,
                                               ScreenplayParagraphType::Parenthetical);
        addSimpleTextEditorStylesChangeByTab(ScreenplayParagraphType::Dialogue,
                                             ScreenplayParagraphType::Parenthetical);
        addSimpleTextEditorStylesChangeByEnter(ScreenplayParagraphType::Dialogue,
                                               ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesChangeByTab(ScreenplayParagraphType::Lyrics,
                                             ScreenplayParagraphType::Parenthetical);
        addSimpleTextEditorStylesChangeByEnter(ScreenplayParagraphType::Lyrics,
                                               ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesChangeByTab(ScreenplayParagraphType::Transition,
                                             ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesChangeByEnter(ScreenplayParagraphType::Transition,
                                               ScreenplayParagraphType::Transition);
        addSimpleTextEditorStylesChangeByTab(ScreenplayParagraphType::Shot,
                                             ScreenplayParagraphType::Action);
        addSimpleTextEditorStylesChangeByEnter(ScreenplayParagraphType::Shot,
                                               ScreenplayParagraphType::Shot);
        addSimpleTextEditorStylesChangeByTab(ScreenplayParagraphType::InlineNote,
                                             ScreenplayParagraphType::InlineNote);
        addSimpleTextEditorStylesChangeByEnter(ScreenplayParagraphType::InlineNote,
                                               ScreenplayParagraphType::InlineNote);
        addSimpleTextEditorStylesChangeByTab(ScreenplayParagraphType::FolderHeader,
                                             ScreenplayParagraphType::FolderHeader);
        addSimpleTextEditorStylesChangeByEnter(ScreenplayParagraphType::FolderHeader,
                                               ScreenplayParagraphType::FolderHeader);
        //
        auto addShortcut = [this,
                            kScreenplayEditorKey](BusinessLayer::ScreenplayParagraphType _type,
                                                  const QString& _shortcut) {
            defaultValues.insert(QString("%1/shortcuts/%2")
                                     .arg(kScreenplayEditorKey, BusinessLayer::toString(_type)),
                                 QKeySequence(_shortcut).toString(QKeySequence::NativeText));
        };
        addShortcut(BusinessLayer::ScreenplayParagraphType::UnformattedText, "Ctrl+0");
        addShortcut(BusinessLayer::ScreenplayParagraphType::SceneHeading, "Ctrl+1");
        addShortcut(BusinessLayer::ScreenplayParagraphType::SceneCharacters, "Ctrl+2");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Action, "Ctrl+3");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Character, "Ctrl+4");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Parenthetical, "Ctrl+5");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Dialogue, "Ctrl+6");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Lyrics, "Ctrl+7");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Shot, "Ctrl+8");
        addShortcut(BusinessLayer::ScreenplayParagraphType::Transition, "Ctrl+9");
        addShortcut(BusinessLayer::ScreenplayParagraphType::InlineNote, "Ctrl+Esc");
        addShortcut(BusinessLayer::ScreenplayParagraphType::FolderHeader, "Ctrl+Space");
        //
        defaultValues.insert(kComponentsScreenplayEditorDefaultTemplateKey, "world_cp");
        defaultValues.insert(kComponentsScreenplayEditorShowSceneNumberOnLeftKey, true);
        //
        // Параметры навигатора сценария
        //
        defaultValues.insert(kComponentsScreenplayNavigatorShowSceneNumberKey, true);
        defaultValues.insert(kComponentsScreenplayNavigatorShowSceneTextKey, true);
        defaultValues.insert(kComponentsScreenplayNavigatorSceneTextLinesKey, 1);
        //
        // Параметры хронометража сценария
        //
        defaultValues.insert(kComponentsScreenplayDurationTypeKey, 0);
        defaultValues.insert(kComponentsScreenplayDurationByPageDurationKey, 60);
        defaultValues.insert(kComponentsScreenplayDurationByCharactersCharactersKey, 1350);
        defaultValues.insert(kComponentsScreenplayDurationByCharactersIncludeSpacesKey, true);
        defaultValues.insert(kComponentsScreenplayDurationByCharactersDurationKey, 60);
    }


    defaultValues.insert(kSystemUsernameKey, systemUserName());
}

QVariant SettingsStorage::Implementation::cachedValue(const QString& _key,
                                                      SettingsStorage::SettingsPlace _settingsPlace,
                                                      bool& _ok) const
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

QVariant SettingsStorage::value(const QString& _key, SettingsStorage::SettingsPlace _settingsPlace,
                                const QVariant& _defaultValue) const
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

QVariantMap SettingsStorage::values(const QString& _valuesGroup,
                                    SettingsStorage::SettingsPlace _settingsPlace)
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
