#pragma once

#include <QVector>

#include <corelib_global.h>


namespace BusinessLayer {

struct ComicBookImportOptions;

/**
 * @brief Базовый класс для реализации импортера документов
 */
class CORE_LIBRARY_EXPORT ComicBookAbstractImporter
{
public:
    virtual ~ComicBookAbstractImporter() = default;

    /**
     * @brief Вспомогательные структуры для хранения данных импортируемых документов
     */
    struct Document {
        QString name;
        QString text;
    };

    /**
     * @brief Импорт сценариев из заданного документа
     */
    virtual Document importComicBook(const ComicBookImportOptions& _options) const = 0;
};

} // namespace BusinessLayer
