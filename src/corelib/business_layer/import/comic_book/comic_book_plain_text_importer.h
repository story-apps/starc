#pragma once

#include "comic_book_abstract_importer.h"


namespace BusinessLayer {

/**
 * @brief Импортер комикса из текстового файла
 */
class CORE_LIBRARY_EXPORT ComicBookPlainTextImporter : public ComicBookAbstractImporter
{
public:
    ComicBookPlainTextImporter() = default;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Document importComicBook(const ComicBookImportOptions& _options) const override;
    Document importComicBook(const QString& _text) const;
};

} // namespace BusinessLayer
