#pragma once

#include "novel_abstract_importer.h"


namespace BusinessLayer {

/**
 * @brief Импортер текста из markdown файла
 */
class CORE_LIBRARY_EXPORT NovelMarkdownImporter : public NovelAbstractImporter
{
public:
    NovelMarkdownImporter() = default;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Document importNovels(const NovelImportOptions& _options) const override;
    Document importNovel(const QString& _text) const;
};

} // namespace BusinessLayer
