#pragma once

#include <QVariantMap>
#include <QSettings>
#include <QVariant>


namespace DataStorageLayer
{

namespace {
    //
    // Приложение
    //
    const QString kApplicationGroupKey = "application";
    //
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

    //
    // Система
    //
    const QString kSystemGroupKey = "system";
    //
    // системное имя пользователя
    const QString kSystemUsernameKey = kSystemGroupKey + "/username";
}

/**
 * @brief Хранилище настроек
 */
class SettingsStorage
{
public:
    enum class SettingsPlace {
        Application,
        Project
    };

public:
    ~SettingsStorage();

    /**
     * @brief Сохранить значение с заданным ключём
     */
    void setValue(const QString& _key, const QVariant& _value, SettingsPlace _settingsPlace);

    /**
     * @brief Сохранить карту параметров
     */
    void setValues(const QString& _valuesGroup, const QVariantMap& _values, SettingsPlace _settingsPlace);

    /**
     * @brief Получить значение по ключу
     */
    QVariant value(const QString& _key, SettingsPlace _settingsPlace, const QVariant& _defaultValue = {}) const;

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

