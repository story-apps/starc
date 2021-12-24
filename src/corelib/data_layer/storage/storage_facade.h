#pragma once

#include <corelib_global.h>

class QVariant;
typedef QMap<QString, QVariant> QVariantMap;

namespace DataStorageLayer {

class DocumentChangeStorage;
class DocumentStorage;
class SettingsStorage;

/**
 * @brief Фасад для доступа к хранилищам данных
 */
class CORE_LIBRARY_EXPORT StorageFacade
{
public:
    /**
     * @brief Очистить все хранилища
     */
    static void clearStorages();

    /**
     * @brief Получить хранилище документов проекта
     */
    static DocumentChangeStorage* documentChangeStorage();

    /**
     * @brief Получить хранилище документов проекта
     */
    static DocumentStorage* documentStorage();

    /**
     * @brief Получить хранилище настроек
     */
    static SettingsStorage* settingsStorage();

private:
    static DocumentChangeStorage* s_documentChangeStorage;
    static DocumentStorage* s_documentStorage;
    static SettingsStorage* s_settingsStorage;
};

} // namespace DataStorageLayer

/**
 * @brief Вспомогательные функции для удобного доступа к настройкам
 */
QVariant CORE_LIBRARY_EXPORT settingsValue(const QString& _key);
QVariant CORE_LIBRARY_EXPORT settingsValue(const QString& _key, const QVariant& _defaultValue);
QVariantMap CORE_LIBRARY_EXPORT settingsValues(const QString& _key);
void CORE_LIBRARY_EXPORT setSettingsValue(const QString& _key, const QVariant& _value);
void CORE_LIBRARY_EXPORT setSettingsValues(const QString& _key, const QVariantMap& _value);
