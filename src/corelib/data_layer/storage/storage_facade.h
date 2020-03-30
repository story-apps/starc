#pragma once

#include <corelib_global.h>


namespace DataStorageLayer
{

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
