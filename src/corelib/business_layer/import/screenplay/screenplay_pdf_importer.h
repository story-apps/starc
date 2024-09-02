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
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const ImportOptions& _options) const override;

    /**
     * @brief Импортировать сценарии
     */
    QVector<Screenplay> importScreenplays(const ScreenplayImportOptions& _options) const override;
};

} // namespace BusinessLayer
