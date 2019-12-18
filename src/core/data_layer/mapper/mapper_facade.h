#pragma once


namespace DataMappingLayer
{
class DocumentsMapper;
class SettingsMapper;

/**
 * @brief Фасад для доступа ко всем отображателям данных
 */
class MapperFacade
{
public:
    static DocumentsMapper* documentsMapper();
    static SettingsMapper* settingsMapper();

private:
    static DocumentsMapper* s_documentsMapper;
    static SettingsMapper* s_settingsMapper;
};

} // namespace DataMappingLayer

