#pragma once

#include "novel_abstract_importer.h"

#include <corelib/business_layer/import/abstract_markdown_importer.h>


namespace BusinessLayer {

/**
 * @brief Импортер текста из markdown файла
 */
class CORE_LIBRARY_EXPORT NovelMarkdownImporter : public NovelAbstractImporter,
                                                  public AbstractMarkdownImporter
{
public:
    NovelMarkdownImporter();
    ~NovelMarkdownImporter() override;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Document importNovels(const ImportOptions& _options) const override;
    Document importNovel(const QString& _text) const;
};

} // namespace BusinessLayer
