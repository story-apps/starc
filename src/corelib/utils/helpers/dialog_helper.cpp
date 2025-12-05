#include "dialog_helper.h"

#include "extension_helper.h"

#include <QApplication>


namespace {
/**
 * @brief Сформировать строку-фильтр для файла с заданными заголовком и расширением
 */
/** @{ */
QString makeFilter(const QString& _title, const QStringList& _extensions)
{
    QString extensionsValue;
    for (const auto& extension : _extensions) {
        if (!extensionsValue.isEmpty()) {
            extensionsValue.append(' ');
        }
        extensionsValue.append(QString("*.%1").arg(extension));
    }
    return QString("%1 (%2)").arg(_title, extensionsValue);
}
QString makeFilter(const QString& _title, const QString& _extension)
{
    return makeFilter(_title, QStringList{ _extension });
}
/** @} */
} // namespace

QString DialogHelper::starcProjectFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "Story Architect project"),
                      ExtensionHelper::starc());
}

QString DialogHelper::starcTemplateFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "Story Architect template"),
                      ExtensionHelper::starct());
}

QString DialogHelper::kitScenaristFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "KIT Scenarist project"),
                      ExtensionHelper::kitScenarist());
}

QString DialogHelper::finalDraftFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "Final Draft screenplay"),
                      ExtensionHelper::finalDraft());
}

QString DialogHelper::finalDraftTemplateFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "Final Draft template"),
                      ExtensionHelper::finalDraftTemplate());
}

QString DialogHelper::trelbyFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "Trelby screenplay"),
                      ExtensionHelper::trelby());
}

QString DialogHelper::msWordFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "Office Open XML"),
                      { ExtensionHelper::msOfficeOpenXml(), ExtensionHelper::msOfficeBinary() });
}

QString DialogHelper::openDocumentXmlFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "OpenDocument text"),
                      ExtensionHelper::openDocumentXml());
}

QString DialogHelper::fountainFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "Fountain text"),
                      ExtensionHelper::fountain());
}

QString DialogHelper::celtxFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "Celtx project"),
                      ExtensionHelper::celtx());
}

QString DialogHelper::plainTextFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "Plain text"),
                      ExtensionHelper::plainText());
}

QString DialogHelper::pdfFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "Portable document format"),
                      ExtensionHelper::pdf());
}

QString DialogHelper::pngFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "PNG image"), ExtensionHelper::png());
}

QString DialogHelper::xlsxFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "MS Excel spreadsheet"),
                      ExtensionHelper::xlsx());
}

QString DialogHelper::markdownFilter()
{
    return makeFilter(QApplication::translate("DialogHelper", "Markdown"),
                      ExtensionHelper::markdown());
}

QString DialogHelper::filtersForOpenProject()
{
    QString filters = makeFilter(QApplication::translate("DialogHelper", "All supported files"),
                                 {
                                     ExtensionHelper::starc(),
                                     ExtensionHelper::fountain(),
                                     ExtensionHelper::plainText(),
                                 });
    for (const auto& filter : {
             starcProjectFilter(),
             fountainFilter(),
             plainTextFilter(),
         }) {
        filters.append(";;");
        filters.append(filter);
    }
    return filters;
}

QString DialogHelper::filtersForImport()
{
    QString filters = makeFilter(QApplication::translate("DialogHelper", "All supported files"),
                                 {
                                     ExtensionHelper::kitScenarist(),
                                     ExtensionHelper::finalDraft(),
                                     ExtensionHelper::finalDraftTemplate(),
                                     ExtensionHelper::trelby(),
                                     ExtensionHelper::msOfficeBinary(),
                                     ExtensionHelper::msOfficeOpenXml(),
                                     ExtensionHelper::openDocumentXml(),
                                     ExtensionHelper::fountain(),
                                     ExtensionHelper::celtx(),
                                     ExtensionHelper::plainText(),
                                     ExtensionHelper::markdown(),
                                     ExtensionHelper::pdf(),
                                 });
    for (const auto& filter : {
             kitScenaristFilter(),
             finalDraftFilter(),
             finalDraftTemplateFilter(),
             trelbyFilter(),
             msWordFilter(),
             openDocumentXmlFilter(),
             fountainFilter(),
             celtxFilter(),
             plainTextFilter(),
             pdfFilter(),
         }) {
        filters.append(";;");
        filters.append(filter);
    }
    return filters;
}

QString DialogHelper::filtersForImportScreenplay()
{
    return filtersForImport();
}

QString DialogHelper::filtersForImportComicBook()
{
    QString filters = makeFilter(QApplication::translate("DialogHelper", "All supported files"),
                                 {
                                     ExtensionHelper::fountain(),
                                     ExtensionHelper::plainText(),
                                 });
    for (const auto& filter : {
             fountainFilter(),
             plainTextFilter(),
         }) {
        filters.append(";;");
        filters.append(filter);
    }
    return filters;
}

QString DialogHelper::filtersForImportAudioplay()
{
    return filtersForImportComicBook();
}

QString DialogHelper::filtersForImportStageplay()
{
    return filtersForImportComicBook();
}

QString DialogHelper::filtersForImportNovel()
{
    QString filters = makeFilter(QApplication::translate("DialogHelper", "All supported files"),
                                 {
                                     ExtensionHelper::markdown(),
                                     ExtensionHelper::plainText(),
                                 });
    for (const auto& filter : {
             markdownFilter(),
             plainTextFilter(),
         }) {
        filters.append(";;");
        filters.append(filter);
    }
    return filters;
}

QString DialogHelper::filtersForSceneImage()
{
    return pngFilter();
}

QString DialogHelper::filtersForReport()
{
    QString filters = pdfFilter();
    filters.append(";;");
    filters.append(xlsxFilter());
    return filters;
}

QString DialogHelper::filtersForPlot()
{
    return xlsxFilter();
}
