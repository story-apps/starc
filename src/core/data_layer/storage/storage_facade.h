#pragma once


namespace DataStorageLayer
{

class DocumentsStorage;
class SettingsStorage;

/**
 * @brief Фасад для доступа к хранилищам данных
 */
class StorageFacade
{
public:
    /**
     * @brief Очистить все хранилища
     */
    static void clearStorages();

    /**
     * @brief Получить хранилище документов проекта
     */
    static DocumentsStorage* documentsStorage();

    /**
     * @brief Получить хранилище настроек
     */
    static SettingsStorage* settingsStorage();

private:
    static DocumentsStorage* s_documentsStorage;
    static SettingsStorage* s_settingsStorage;
};

} // namespace DataStorageLayer
