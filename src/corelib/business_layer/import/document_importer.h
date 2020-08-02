#pragma once

#include "abstract_importer.h"


namespace BusinessLayer
{

/**
 * @brief Импортер сценария из файлов Trelby
 */
class CORE_LIBRARY_EXPORT DocumentImporter : public AbstractImporter
{
public:
    DocumentImporter() = default;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const ImportOptions& _options) const override;

    /**
     * @brief Сформировать xml-сценария во внутреннем формате
     */
    QVector<Screenplay> importScreenplays(const ImportOptions& _options) const override;
};

} // namespace BusinessLayer
