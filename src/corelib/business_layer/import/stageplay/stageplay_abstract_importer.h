#pragma once

#include <QVector>

#include <corelib_global.h>


namespace BusinessLayer {

struct StageplayImportOptions;

/**
 * @brief Базовый класс для реализации импортера документов
 */
class CORE_LIBRARY_EXPORT StageplayAbstractImporter
{
public:
    virtual ~StageplayAbstractImporter() = default;

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
    virtual Documents importDocuments(const StageplayImportOptions& _options) const = 0;

    /**
     * @brief Вспомогательная структура для хранения данных импортированного сценария
     */
    struct Stageplay {
        QString name;
        QString titlePage;
        QString synopsis;
        QString text;
    };

    /**
     * @brief Импорт сценариев из заданного документа
     */
    virtual QVector<Stageplay> importStageplays(const StageplayImportOptions& _options) const = 0;
};

} // namespace BusinessLayer
