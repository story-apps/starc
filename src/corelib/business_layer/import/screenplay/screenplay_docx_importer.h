#pragma once

#include "abstract_screenplay_importer.h"
#include "business_layer/import/abstract_document_importer.h"

class QTextCursor;

namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов Docx
 */
class CORE_LIBRARY_EXPORT ScreenplayDocxImporter : public AbstractScreenplayImporter,
                                                   public AbstractDocumentImporter
{
public:
    ScreenplayDocxImporter() = default;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const ImportOptions& _options) const override;

    /**
     * @brief Сформировать xml-сценария во внутреннем формате
     */
    QVector<Screenplay> importScreenplays(const ScreenplayImportOptions& _options) const override;

protected:
    /**
     * @brief Извлечь номер сцены из заголовка
     */
    QString extractSceneNumber(const ImportOptions& _options, QTextCursor& _cursor) const override;

    /**
     * @brief Записать редакторские заметки
     */
    void writeReviewMarks(QXmlStreamWriter& _writer, QTextCursor& _cursor) const override;
};

} // namespace BusinessLayer
