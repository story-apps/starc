#pragma once

#include "abstract_simple_text_importer.h"


namespace BusinessLayer {

/**
 * @brief Импортер текста из markdown файла
 */
class CORE_LIBRARY_EXPORT SimpleTextMarkdownImporter : public AbstractSimpleTextImporter
{
public:
    SimpleTextMarkdownImporter() = default;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Document importDocument(const SimpleTextImportOptions& _options) const override;
    Document importDocument(const QString& _text) const;
};

} // namespace BusinessLayer
