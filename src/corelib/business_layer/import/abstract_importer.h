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
    virtual ~AbstractImporter() = default;
};

} // namespace BusinessLayer
