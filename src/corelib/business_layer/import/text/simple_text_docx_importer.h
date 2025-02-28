#pragma once

#include "abstract_simple_text_importer.h"
#include "business_layer/import/abstract_document_importer.h"

class QTextCursor;

namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов Docx
 */
class CORE_LIBRARY_EXPORT SimpleTextDocxImporter : public AbstractSimpleTextImporter,
                                                   public AbstractDocumentImporter
{
public:
    SimpleTextDocxImporter();
    ~SimpleTextDocxImporter() override;

    /**
     * @brief Сформировать xml-документа во внутреннем формате
     */
    SimpleText importSimpleText(const ImportOptions& _options) const override;

protected:
    /**
     * @brief Получить документ для импорта
     * @return true, если получилось открыть заданный файл
     */
    bool documentForImport(const QString& _filePath, QTextDocument& _document) const override;

    /**
     * @brief Определить тип блока в текущей позиции курсора
     *        с указанием предыдущего типа и количества предшествующих пустых строк
     */
    TextParagraphType typeForTextCursor(const QTextCursor& _cursor,
                                        TextParagraphType _lastBlockType, int _prevEmptyLines,
                                        int _minLeftMargin) const override;

    /**
     * @brief Записать редакторские заметки
     */
    void writeReviewMarks(QXmlStreamWriter& _writer, QTextCursor& _cursor) const override;
};

} // namespace BusinessLayer
