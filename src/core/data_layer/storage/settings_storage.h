#pragma once

#include <QVariantMap>
#include <QSettings>
#include <QVariant>


namespace DataStorageLayer
{

const QString kApplicationGroupKey = "application/";
const QString kSystemGroupKey = "system/";

// первый запуск приложения (false) или оно уже сконфигурировано (true)
const QString kApplicationConfiguredKey = kApplicationGroupKey + "configured";
// язык приложения
const QString kApplicationLanguagedKey = kApplicationGroupKey + "language";
// тема приложения
const QString kApplicationThemeKey = kApplicationGroupKey + "theme";
// масштаб приложения
const QString kApplicationScaleFactorKey = kApplicationGroupKey + "scale-factor";
// включено ли автосохранение
const QString kApplicationAutosaveKey = kApplicationGroupKey + "autosave";
// интервал автосохранения в минутах
const QString kApplicationAutosaveIntervalKey = kApplicationGroupKey + "autosave-interval";

// системное имя пользователя
const QString kSystemUsernameKey = kSystemGroupKey + "username";

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
    void setValues(const QVariantMap& _values, const QString& _valuesGroup, SettingsPlace _settingsPlace);

    /**
     * @brief Получить значение по ключу
     */
    QVariant value(const QString& _key, SettingsPlace _settingsPlace, const QVariant& _defaultValue = {}) const;

    /**
     * @brief Получить группу значений
     */
    QVariantMap values(const QString& _valuesGroup, SettingsPlace _settingsPlace);


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
    class Implememntation;
    QScopedPointer<Implememntation> d;
};

} // namespace DataStorageLayer

