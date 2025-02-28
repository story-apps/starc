#pragma once

#include <QVector>

#include <corelib_global.h>


namespace BusinessLayer {

struct ImportOptions;

/**
 * @brief Базовый класс для реализации импортера документов
 */
class CORE_LIBRARY_EXPORT AbstractSimpleTextImporter
{
public:
    virtual ~AbstractSimpleTextImporter() = default;

    /**
     * @brief Вспомогательные структуры для хранения данных импортируемых документов
     */
    struct SimpleText {
        QString name;
        QString text;
    };

    /**
     * @brief Импорт сценариев из заданного документа
     */
    virtual SimpleText importSimpleText(const ImportOptions& _options) const = 0;
};

} // namespace BusinessLayer
