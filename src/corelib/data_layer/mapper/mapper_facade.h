#pragma once


namespace DataMappingLayer {
class DocumentChangeMapper;
class DocumentMapper;
class SettingsMapper;

/**
 * @brief Фасад для доступа ко всем отображателям данных
 */
class MapperFacade
{
public:
    static DocumentChangeMapper* documentChangeMapper();
    static DocumentMapper* documentMapper();
    static SettingsMapper* settingsMapper();

private:
    static DocumentChangeMapper* s_documentChangeMapper;
    static DocumentMapper* s_documentMapper;
    static SettingsMapper* s_settingsMapper;
};

} // namespace DataMappingLayer
