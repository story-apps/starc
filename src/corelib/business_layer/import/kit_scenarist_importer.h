#pragma once

#include "abstract_importer.h"


namespace BusinessLayer
{

/**
 * @brief Импортер сценария из файлов КИТа
 */
class KitScenaristImporter : public AbstractImporter
{
public:
    KitScenaristImporter() = default;

    /**
     * @brief Сформировать xml-сценария во внутреннем формате
     */
    QVector<Screenplay> importScreenplays(const ImportOptions& _options) const override;
};

} // namespace BusinessLayer
