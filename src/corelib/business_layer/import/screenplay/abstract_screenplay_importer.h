#pragma once

#include <domain/document_object.h>

#include <QVector>

#include <corelib_global.h>


namespace BusinessLayer {

struct ScreenplayImportOptions;

/**
 * @brief Базовый класс для реализации импортера документов
 */
class CORE_LIBRARY_EXPORT AbstractScreenplayImporter
{
public:
    virtual ~AbstractScreenplayImporter() = default;

    /**
     * @brief Вспомогательные структуры для хранения данных импортируемых документов
     */
    struct Document {
        Domain::DocumentObjectType type = Domain::DocumentObjectType::Undefined;
        QString name;
        QString content;
        QVector<Document> children;
    };
    struct Documents {
        //
        // <Название, контент>
        //
        QVector<Document> characters;
        QVector<Document> locations;
        QVector<Document> research;
    };

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    virtual Documents importDocuments(const ScreenplayImportOptions& _options) const = 0;

    /**
     * @brief Вспомогательная структура для хранения данных импортированного сценария
     */
    struct Screenplay {
        QString name;
        QString header;
        QString footer;
        QString logline;
        QString titlePage;
        QString synopsis;
        QString treatment;
        QString text;
    };

    /**
     * @brief Импорт сценариев из заданного документа
     */
    virtual QVector<Screenplay> importScreenplays(
        const ScreenplayImportOptions& _options) const = 0;
};

} // namespace BusinessLayer
