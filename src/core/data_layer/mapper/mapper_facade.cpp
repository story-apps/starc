#include "mapper_facade.h"

//#include "ItemsMapper.h"
#include "settings_mapper.h"


namespace DataMappingLayer
{

//ItemsMapper*MapperFacade::itemsMapper()
//{
//    if (s_itemsMapper == nullptr) {
//        s_itemsMapper = new ItemsMapper;
//    }

//    return s_itemsMapper;
//}

SettingsMapper* MapperFacade::settingsMapper()
{
    if (s_settingsMapper == nullptr) {
        s_settingsMapper = new SettingsMapper;
    }
    return s_settingsMapper;
}

//ItemsMapper* MapperFacade::s_itemsMapper = nullptr;
SettingsMapper* MapperFacade::s_settingsMapper = nullptr;

}
