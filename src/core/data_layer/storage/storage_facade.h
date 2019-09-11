#pragma once


namespace DataStorageLayer
{

class ItemsStorage;
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

//    /**
//     * @brief Получить хранилище элементов проекта
//     */
//    static ItemsStorage* itemsStorage();

    /**
     * @brief Получить хранилище настроек
     */
    static SettingsStorage* settingsStorage();

private:
//    static ItemsStorage* s_itemsStorage;
    static SettingsStorage* s_settingsStorage;
};

} // namespace DataStorageLayer
