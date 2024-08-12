#pragma once

#include "screenplay_fountain_importer.h"


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT ScreenplayPdfImporter : public ScreenplayFountainImporter
{
public:
    ScreenplayPdfImporter();
    ~ScreenplayPdfImporter() override;

    /**
     * @brief Импортировать сценарии
     */
    QVector<Screenplay> importScreenplays(const ScreenplayImportOptions& _options) const override;
};

} // namespace BusinessLayer
