#include "screenplay_pdf_importer.h"

#include "TableExtraction.h"
#include "screenplay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>

#include <QFileInfo>
#include <QTextDocument>


namespace BusinessLayer {

ScreenplayPdfImporter::ScreenplayPdfImporter()
    : AbstractScreenplayImporter()
    , AbstractDocumentImporter()
{
}

ScreenplayPdfImporter::~ScreenplayPdfImporter() = default;


QVector<AbstractScreenplayImporter::Screenplay> ScreenplayPdfImporter::importScreenplays(
    const ScreenplayImportOptions& _options) const
{
    Screenplay screenplay;
    screenplay.name = QFileInfo(_options.filePath).completeBaseName();

    if (_options.importText == false) {
        return { screenplay };
    }

    //
    // Преобразуем заданный документ в QTextDocument и парсим его
    //
    if (QTextDocument document; documentForImport(_options.filePath, document)) {
        screenplay.text = parseDocument(_options, document);
    }

    return { screenplay };
}

bool ScreenplayPdfImporter::documentForImport(const QString& _filePath,
                                              QTextDocument& _document) const
{
    QFile documentFile(_filePath);
    if (documentFile.open(QIODevice::ReadOnly)) {
        //
        // Используем TableExtraction, чтобы извлечь не только текст, но и линии
        //
        TableExtraction tableExtractor;
        tableExtractor.ExtractTables(_filePath.toStdString(), 0, -1, true);
        tableExtractor.GetResultsAsDocument(_document);
        return true;
    }
    return false;
}

bool ScreenplayPdfImporter::shouldKeepSceneNumbers(const ImportOptions& _options) const
{
    const auto& options = static_cast<const ScreenplayImportOptions&>(_options);
    return options.keepSceneNumbers;
}

QString ScreenplayPdfImporter::characterName(const QString& _text) const
{
    return ScreenplayCharacterParser::name(_text);
}

QString ScreenplayPdfImporter::locationName(const QString& _text) const
{
    return ScreenplaySceneHeadingParser::location(_text);
}

} // namespace BusinessLayer
