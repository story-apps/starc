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
    SimpleText importSimpleText(const ImportOptions& _options) const override;
    SimpleText importSimpleText(const QString& _text) const;
};

} // namespace BusinessLayer
