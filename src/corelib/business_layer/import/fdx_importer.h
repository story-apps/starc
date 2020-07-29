#pragma once

#include "abstract_importer.h"


namespace BusinessLayer
{

/**
 * @brief Импортер сценария из файлов Final Draft
 */
class CORE_LIBRARY_EXPORT FdxImporter : public AbstractImporter
{
public:
    FdxImporter() = default;

    /**
     * @brief Сформировать xml-сценария во внутреннем формате
     */
    QVector<Screenplay> importScreenplays(const ImportOptions& _options) const override;
};

} // namespace BusinessLayer
