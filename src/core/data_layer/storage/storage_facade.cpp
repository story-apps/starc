#include "storage_facade.h"

//#include "ItemsStorage.h"
#include "settings_storage.h"


namespace DataStorageLayer
{

void StorageFacade::clearStorages()
{
//    itemsStorage()->clear();
}

//ItemsStorage*StorageFacade::itemsStorage()
//{
//    if (s_itemsStorage == nullptr) {
//        s_itemsStorage = new ItemsStorage;
//    }

//    return s_itemsStorage;
//}

SettingsStorage* StorageFacade::settingsStorage()
{
    if (s_settingsStorage == nullptr) {
        s_settingsStorage = new SettingsStorage;
    }
    return s_settingsStorage;
}

//ItemsStorage* StorageFacade::s_itemsStorage = nullptr;
SettingsStorage* StorageFacade::s_settingsStorage = nullptr;

} // namespace DataStorageLayer
