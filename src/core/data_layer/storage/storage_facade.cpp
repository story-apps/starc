#include "storage_facade.h"

#include "documents_storage.h"
#include "settings_storage.h"


namespace DataStorageLayer
{

void StorageFacade::clearStorages()
{
    documentsStorage()->clear();
}

DocumentsStorage* StorageFacade::documentsStorage()
{
    if (s_documentsStorage == nullptr) {
        s_documentsStorage = new DocumentsStorage;
    }

    return s_documentsStorage;
}

SettingsStorage* StorageFacade::settingsStorage()
{
    if (s_settingsStorage == nullptr) {
        s_settingsStorage = new SettingsStorage;
    }
    return s_settingsStorage;
}

DocumentsStorage* StorageFacade::s_documentsStorage = nullptr;
SettingsStorage* StorageFacade::s_settingsStorage = nullptr;

} // namespace DataStorageLayer
