#pragma once

#include "comic_book_exporter.h"

#include <business_layer/export/abstract_docx_exporter.h>

#include <QScopedPointer>


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT ComicBookDocxExporter : public ComicBookExporter,
                                                  public AbstractDocxExporter
{
public:
    ComicBookDocxExporter();
    ~ComicBookDocxExporter() override;

protected:
    /**
     * @brief Определить список стилей для экспорта
     */
    QVector<TextParagraphType> paragraphTypes() const override;
};

} // namespace BusinessLayer
