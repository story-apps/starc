#include "screenplay_pdf_importer.h"

#include "TableExtraction.h"
#include "screenplay_import_options.h"

#include <QFileInfo>
#include <QTextDocument>


namespace BusinessLayer {

ScreenplayPdfImporter::ScreenplayPdfImporter()
    : AbstractScreenplayImporter()
    , AbstractQTextDocumentImporter()
{
}

ScreenplayPdfImporter::~ScreenplayPdfImporter() = default;


AbstractImporter::Documents ScreenplayPdfImporter::importDocuments(
    const ImportOptions& _options) const
{
    Documents documents;
    return documents;
}

QVector<AbstractScreenplayImporter::Screenplay> ScreenplayPdfImporter::importScreenplays(
    const ScreenplayImportOptions& _options) const
{
    Screenplay screenplay;
    screenplay.name = QFileInfo(_options.filePath).completeBaseName();

    if (_options.importText == false) {
        return { screenplay };
    }

    //
    // Открываем файл
    //
    QFile fountainFile(_options.filePath);
    if (!fountainFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Преобразовать заданный документ в QTextDocument
    // Используем TableExtraction, чтобы извлечь не только текст, но и линии
    //
    TableExtraction tableExtractor;
    tableExtractor.ExtractTables(_options.filePath.toStdString(), 0, -1, true);
    QTextDocument document;
    tableExtractor.GetResultsAsDocument(document);

    //
    // Парсим QTextDocument
    //
    screenplay.text = parseDocument(_options, document);
    return { screenplay };
}

} // namespace BusinessLayer
