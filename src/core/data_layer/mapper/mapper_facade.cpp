#include "mapper_facade.h"

#include "documents_mapper.h"
#include "settings_mapper.h"


namespace DataMappingLayer
{

DocumentsMapper* MapperFacade::documentsMapper()
{
    if (s_documentsMapper == nullptr) {
        s_documentsMapper = new DocumentsMapper;
    }

    return s_documentsMapper;
}

SettingsMapper* MapperFacade::settingsMapper()
{
    if (s_settingsMapper == nullptr) {
        s_settingsMapper = new SettingsMapper;
    }
    return s_settingsMapper;
}

DocumentsMapper* MapperFacade::s_documentsMapper = nullptr;
SettingsMapper* MapperFacade::s_settingsMapper = nullptr;

}
