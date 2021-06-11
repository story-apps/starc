#pragma once

#include "abstract_text_importer.h"


namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов Trelby
 */
class CORE_LIBRARY_EXPORT MarkdownImporter : public AbstractTextImporter
{
public:
    MarkdownImporter() = default;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Document importDocument(const TextImportOptions& _options) const override;
    Document importDocument(const QString& _text) const;
};

} // namespace BusinessLayer
