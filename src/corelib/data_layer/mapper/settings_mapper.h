#pragma once

class QString;


namespace DataMappingLayer {

/**
 * @brief Отображатель данных в таблицу проекта
 */
class SettingsMapper
{
public:
    /**
     * @brief Сохранить значение с заданным ключём
     */
    void setValue(const QString& _key, const QString& _value);

    /**
     * @brief Получить значение по ключу
     */
    QString value(const QString& _key);

private:
    SettingsMapper();
    friend class MapperFacade;
};

} // namespace DataMappingLayer
