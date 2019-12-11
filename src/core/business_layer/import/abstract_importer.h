#pragma once

class QString;


namespace BusinessLayer
{


/**
 * @brief Базовый класс для реализации импортера документов
 */
class AbstractImporter
{
public:
    /**
     * @brief Получить список фильтров файлов которые приложение может импортировать
     */
    static QString filters();

public:
    virtual ~AbstractImporter() = default;
};

} // namespace BusinessLayer
