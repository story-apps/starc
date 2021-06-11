#pragma once

#include <QSettings>
#include <QVariant>
#include <QVariantMap>

#include <corelib_global.h>


/**
 * Правила именования настроек
 *
 * Приложение
 * application/key
 *
 * Проект
 * project/key - общее для всех
 * project/concrete/path/ - для конкретного проекта
 *
 * Система
 * system/key
 *
 * Виджеты
 * widgets/widget-name/key
 */

namespace DataStorageLayer {

namespace {
//
// Приложение
//
const QString kApplicationGroupKey = "application";
//
// уникальный индентификатор приложения
const QString kApplicationUuidKey = kApplicationGroupKey + "/uuid";
// первый запуск приложения (false) или оно уже сконфигурировано (true)
const QString kApplicationConfiguredKey = kApplicationGroupKey + "/configured";
// язык приложения
const QString kApplicationLanguagedKey = kApplicationGroupKey + "/language";
// тема приложения
const QString kApplicationThemeKey = kApplicationGroupKey + "/theme";
// параметры кастомной тема приложения
const QString kApplicationCustomThemeColorsKey = kApplicationGroupKey + "/custom-theme";
// масштаб приложения
const QString kApplicationScaleFactorKey = kApplicationGroupKey + "/scale-factor";
// состояние и геометрия основного окна приложения
const QString kApplicationViewStateKey = kApplicationGroupKey + "-view/";
// включены ли звуки печатной машинки при наборе текста
const QString kApplicationUseTypewriterSoundKey = kApplicationGroupKey + "/typewriter-sound";
// включена ли проверка орфографии
const QString kApplicationUseSpellCheckerKey = kApplicationGroupKey + "/use-spell-checker";
// словарь для проверки орфографии
const QString kApplicationSpellCheckerLanguageKey = kApplicationGroupKey + "/use-spell-checker";
// включено ли автосохранение
const QString kApplicationUseAutoSaveKey = kApplicationGroupKey + "/autosave";
// включено ли сохранение резервных копий
const QString kApplicationSaveBackupsKey = kApplicationGroupKey + "/save-backups";
// папка в которую будут сохраняться резервные копии
const QString kApplicationBackupsFolderKey = kApplicationGroupKey + "/backups-folder";
// список недавних проектов
const QString kApplicationProjectsKey = kApplicationGroupKey + "/projects";

//
// Проект
//
const QString kProjectKey = "project";
//
// папка сохранения проектов
const QString kProjectSaveFolderKey = kProjectKey + "/save-folder";
// папка открытия проектов
const QString kProjectOpenFolderKey = kProjectKey + "/open-folder";
// папка импорта проектов
const QString kProjectImportFolderKey = kProjectKey + "/import-folder";
// папка экспорта проектов
const QString kProjectExportFolderKey = kProjectKey + "/export-folder";
// путь до свойств конкретного проекта
QString projectKey(const QString& _path)
{
    return kProjectKey + "/concrete/" + _path;
}
// путь до структуры проекта
QString projectStructureKey(const QString& _path)
{
    return projectKey(_path) + "/structure";
}
// был ли октрыт ли навигатор по проекту
QString projectStructureVisibleKey(const QString& _path)
{
    return projectKey(_path) + "/structure-visible";
}

//
// Система
//
const QString kSystemGroupKey = "system";
//
// системное имя пользователя
const QString kSystemUsernameKey = kSystemGroupKey + "/username";

//
// Компоненты
//
const QString kComponentsGroupKey = QStringLiteral("components");
//
// текст
const QString kComponentsSimpleTextKey = kComponentsGroupKey + QStringLiteral("/simple-text");
// ... редактор
const QString kComponentsSimpleTextEditorKey = kComponentsSimpleTextKey + QStringLiteral("/editor");
const QString kComponentsSimpleTextEditorDefaultTemplateKey
    = kComponentsSimpleTextEditorKey + QStringLiteral("/default-template");
const QString kComponentsSimpleTextEditorHighlightCurrentLineKey
    = kComponentsSimpleTextEditorKey + QStringLiteral("/highlight-current-line");
// ... навигатор
const QString kComponentsSimpleTextNavigatorKey
    = kComponentsSimpleTextKey + QStringLiteral("/navigator");
const QString kComponentsSimpleTextNavigatorShowSceneTextKey
    = kComponentsSimpleTextNavigatorKey + QStringLiteral("/show-scene-text");
const QString kComponentsSimpleTextNavigatorSceneTextLinesKey
    = kComponentsSimpleTextNavigatorKey + QStringLiteral("/scene-text-lines");
//
// сценарий
const QString kComponentsScreenplayKey = kComponentsGroupKey + QStringLiteral("/screenplay");
// ... редактор
const QString kComponentsScreenplayEditorKey = kComponentsScreenplayKey + QStringLiteral("/editor");
const QString kComponentsScreenplayEditorDefaultTemplateKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/default-template");
const QString kComponentsScreenplayEditorShowSceneNumbersKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-scene-numbers");
const QString kComponentsScreenplayEditorShowSceneNumbersOnRightKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-scene-numbers-on-left");
const QString kComponentsScreenplayEditorShowSceneNumberOnLeftKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-scene-number-on-right");
const QString kComponentsScreenplayEditorShowDialogueNumberKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/show-dialogue-number");
const QString kComponentsScreenplayEditorHighlightCurrentLineKey
    = kComponentsScreenplayEditorKey + QStringLiteral("/highlight-current-line");
// ... навигатор
const QString kComponentsScreenplayNavigatorKey
    = kComponentsScreenplayKey + QStringLiteral("/navigator");
const QString kComponentsScreenplayNavigatorShowSceneNumberKey
    = kComponentsScreenplayNavigatorKey + QStringLiteral("/show-scene-number");
const QString kComponentsScreenplayNavigatorShowSceneTextKey
    = kComponentsScreenplayNavigatorKey + QStringLiteral("/show-scene-text");
const QString kComponentsScreenplayNavigatorSceneTextLinesKey
    = kComponentsScreenplayNavigatorKey + QStringLiteral("/scene-text-lines");
// ... хронометраж
const QString kComponentsScreenplayDurationKey
    = kComponentsScreenplayKey + QStringLiteral("/duration");
const QString kComponentsScreenplayDurationTypeKey
    = kComponentsScreenplayDurationKey + QStringLiteral("/type");
const QString kComponentsScreenplayDurationByPageDurationKey
    = kComponentsScreenplayDurationKey + QStringLiteral("/by-page-duration");
const QString kComponentsScreenplayDurationByCharactersCharactersKey
    = kComponentsScreenplayDurationKey + QStringLiteral("/by-characters-characters");
const QString kComponentsScreenplayDurationByCharactersIncludeSpacesKey
    = kComponentsScreenplayDurationKey + QStringLiteral("/by-characters-include-spaces");
const QString kComponentsScreenplayDurationByCharactersDurationKey
    = kComponentsScreenplayDurationKey + QStringLiteral("/by-characters-duration");
} // namespace

/**
 * @brief Хранилище настроек
 */
class CORE_LIBRARY_EXPORT SettingsStorage
{
public:
    enum class SettingsPlace { Application, Project };

public:
    ~SettingsStorage();

    /**
     * @brief Сохранить значение с заданным ключём
     */
    void setValue(const QString& _key, const QVariant& _value, SettingsPlace _settingsPlace);

    /**
     * @brief Сохранить карту параметров
     */
    void setValues(const QString& _valuesGroup, const QVariantMap& _values,
                   SettingsPlace _settingsPlace);

    /**
     * @brief Получить значение по ключу
     */
    QVariant value(const QString& _key, SettingsPlace _settingsPlace,
                   const QVariant& _defaultValue = {}) const;

    /**
     * @brief Получить группу значений
     */
    QVariantMap values(const QString& _valuesGroup, SettingsPlace _settingsPlace);

    //
    // Вспомогательные функции для работы с данными о пользователе
    //

    /**
     * @brief Получить имя пользователя (если авторизован - имя пользователя в облаке,
     *        если не авторизован - имя пользователя в системе)
     */
    QString userName() const;

    /**
     * @brief Получить имейл пользователя (если не авторизован возвращает пустое значение)
     */
    QString userEmail() const;


    //
    // Вспомогательные функции для работы с путями к специальным папкам и файлам в них
    //

    /**
     * @brief Получить путь к папке с документами для сохранения по заданному ключу
     */
    QString documentFolderPath(const QString& _key) const;

    /**
     * @brief Получить путь к файлу в папке с документами для заданного ключа и имени файла
     */
    QString documentFilePath(const QString& _key, const QString& _fileName) const;

    /**
     * @brief Сохранить путь к папке с документами по заданному ключу и пути файла из этой папки
     */
    void setDocumentFolderPath(const QString& _key, const QString& _filePath);

private:
    SettingsStorage();
    friend class StorageFacade;
    //
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace DataStorageLayer
