#pragma once

#include "abstract_screenplay_importer.h"
#include "business_layer/import/abstract_qtextdocument_importer.h"

namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов Pdf
 */
class CORE_LIBRARY_EXPORT ScreenplayPdfImporter : public AbstractScreenplayImporter,
                                                  public AbstractQTextDocumentImporter
{
public:
    ScreenplayPdfImporter();
    ~ScreenplayPdfImporter() override;

    /**
     * @brief Импортировать сценарии
     */
    QVector<Screenplay> importScreenplays(const ScreenplayImportOptions& _options) const override;

protected:
    /**
     * @brief Получить имя персонажа
     */
    QString characterName(const QString& _text) const override;

    /**
     * @brief Получить документ для импорта
     * @return true, если получилось открыть заданный файл
     */
    bool documentForImport(const QString& _filePath, QTextDocument& _document) const override;

    /**
     * @brief Получить название локации
     */
    QString locationName(const QString& _text) const override;
};

} // namespace BusinessLayer
