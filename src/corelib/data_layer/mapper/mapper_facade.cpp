#include "mapper_facade.h"

#include "document_change_mapper.h"
#include "document_mapper.h"
#include "settings_mapper.h"


namespace DataMappingLayer
{

DocumentChangeMapper* MapperFacade::documentChangeMapper()
{
    if (s_documentChangeMapper == nullptr) {
        s_documentChangeMapper = new DocumentChangeMapper;
    }

    return s_documentChangeMapper;
}

DocumentMapper* MapperFacade::documentMapper()
{
    if (s_documentMapper == nullptr) {
        s_documentMapper = new DocumentMapper;
    }

    return s_documentMapper;
}

SettingsMapper* MapperFacade::settingsMapper()
{
    if (s_settingsMapper == nullptr) {
        s_settingsMapper = new SettingsMapper;
    }
    return s_settingsMapper;
}

DocumentChangeMapper* MapperFacade::s_documentChangeMapper = nullptr;
DocumentMapper* MapperFacade::s_documentMapper = nullptr;
SettingsMapper* MapperFacade::s_settingsMapper = nullptr;

}
