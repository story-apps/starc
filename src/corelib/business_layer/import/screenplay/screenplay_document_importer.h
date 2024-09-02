#pragma once

#include "abstract_screenplay_importer.h"
#include "business_layer/import/abstract_qtextdocument_importer.h"

class QTextCursor;

namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов Docx
 */
class CORE_LIBRARY_EXPORT ScreenplayDocumentImporter : public AbstractScreenplayImporter,
                                                       public AbstractQTextDocumentImporter
{
public:
    ScreenplayDocumentImporter() = default;

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
     * @brief Обработать блок сцены
     */
    QString processSceneHeading(const ImportOptions& _options, QTextCursor& _cursor) const override;

    /**
     * @brief Записать редакторские заметки
     */
    void writeReviewMarks(QXmlStreamWriter& _writer, QTextCursor& _cursor) const override;
};

} // namespace BusinessLayer
