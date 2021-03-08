#pragma once

#include <corelib_global.h>

#include <QVector>


namespace BusinessLayer
{

struct ImportOptions;

/**
 * @brief Базовый класс для реализации импортера документов
 */
class CORE_LIBRARY_EXPORT AbstractImporter
{
public:
    virtual ~AbstractImporter() = default;

    /**
     * @brief Вспомогательные структуры для хранения данных импортируемых документов
     */
    struct Document {
        QString name;
        QString content;
    };
    struct Documents {
        //
        // <Название, контент>
        //
        QVector<Document> characters;
        QVector<Document> locations;
    };

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    virtual Documents importDocuments(const ImportOptions& _options) const = 0;

    /**
     * @brief Вспомогательная структура для хранения данных импортированного сценария
     */
    struct Screenplay {
        QString name;
        QString titlePage;
        QString synopsis;
        QString treatment;
        QString text;
    };

    /**
     * @brief Импорт сценариев из заданного документа
     */
    virtual QVector<Screenplay> importScreenplays(const ImportOptions& _options) const = 0;
};

} // namespace BusinessLayer
