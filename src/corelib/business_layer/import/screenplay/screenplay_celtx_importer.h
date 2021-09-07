#pragma once

#include "screenplay_abstract_importer.h"


namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов Celtx
 */
class CORE_LIBRARY_EXPORT ScreenplayCeltxImporter : public ScreenplayAbstractImporter
{
public:
    ScreenplayCeltxImporter() = default;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const ScreenplayImportOptions& _options) const override;

    /**
     * @brief Сформировать xml-сценария во внутреннем формате
     */
    QVector<Screenplay> importScreenplays(const ScreenplayImportOptions& _options) const override;
};

} // namespace BusinessLayer
