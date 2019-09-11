#pragma once


namespace DataMappingLayer
{
class ItemsMapper;
class SettingsMapper;

/**
 * @brief Фасад для доступа ко всем отображателям данных
 */
class MapperFacade
{
public:
//    static ItemsMapper* itemsMapper();
    static SettingsMapper* settingsMapper();

private:
//    static ItemsMapper* s_itemsMapper;
    static SettingsMapper* s_settingsMapper;
};

} // namespace DataMappingLayer

