#pragma once

#include <QVariantMap>
#include <QSettings>
#include <QVariant>


namespace DataStorageLayer
{

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
    QVariant value(const QString& _key, SettingsPlace _settingsPlace) const;

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

