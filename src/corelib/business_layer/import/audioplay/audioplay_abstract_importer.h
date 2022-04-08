#pragma once

#include <QVector>

#include <corelib_global.h>


namespace BusinessLayer {

struct AudioplayImportOptions;

/**
 * @brief Базовый класс для реализации импортера документов
 */
class CORE_LIBRARY_EXPORT AudioplayAbstractImporter
{
public:
    virtual ~AudioplayAbstractImporter() = default;

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
    virtual Documents importDocuments(const AudioplayImportOptions& _options) const = 0;

    /**
     * @brief Вспомогательная структура для хранения данных импортированного сценария
     */
    struct Audioplay {
        QString name;
        QString titlePage;
        QString synopsis;
        QString text;
    };

    /**
     * @brief Импорт сценариев из заданного документа
     */
    virtual QVector<Audioplay> importAudioplays(
        const AudioplayImportOptions& _options) const = 0;
};

} // namespace BusinessLayer
