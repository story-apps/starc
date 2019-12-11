#include "abstract_importer.h"

#include <QApplication>


namespace BusinessLayer
{

QString AbstractImporter::filters()
{
    QString filters;
    filters.append(QApplication::translate("BusinessLogic::AbstractImporter", "All supported files")
                   + QLatin1String(" (*.starc *.kitsp *.fdx *.trelby *.docx *.doc *.odt *.fountain *.celtx)"));
    filters.append(";;");
    filters.append(QApplication::translate("BusinessLogic::AbstractImporter", "Story Architect project")
                   + QLatin1String(" (*.starc)"));
    filters.append(";;");
    filters.append(QApplication::translate("BusinessLogic::AbstractImporter", "KIT Scenarist project")
                   + QLatin1String(" (*.kitsp)"));
    filters.append(";;");
    filters.append(QApplication::translate("BusinessLogic::AbstractImporter", "Final Draft screenplay")
                   + QLatin1String(" (*.fdx)"));
    filters.append(";;");
    filters.append(QApplication::translate("BusinessLogic::AbstractImporter", "Final Draft template")
                   + QLatin1String(" (*.fdxt)"));
    filters.append(";;");
    filters.append(QApplication::translate("BusinessLogic::AbstractImporter", "Trelby screenplay")
                   + QLatin1String(" (*.trelby)"));
    filters.append(";;");
    filters.append(QApplication::translate("BusinessLogic::AbstractImporter", "Office Open XML")
                   + QLatin1String(" (*.docx *.doc)"));
    filters.append(";;");
    filters.append(QApplication::translate("BusinessLogic::AbstractImporter", "OpenDocument text")
                   + QLatin1String(" (*.odt)"));
    filters.append(";;");
    filters.append(QApplication::translate("BusinessLogic::AbstractImporter", "Fountain text")
                   + QLatin1String(" (*.fountain)"));
    filters.append(";;");
    filters.append(QApplication::translate("BusinessLogic::AbstractImporter", "Celtx project")
                   + QLatin1String(" (*.celtx)"));
    return filters;
}

} // namespace BusinessLayer
