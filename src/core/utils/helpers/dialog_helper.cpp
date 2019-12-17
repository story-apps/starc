#include "dialog_helper.h"

#include <QApplication>


QString DialogHelper::starcProjectFilter()
{
    return QApplication::translate("DialogHelper", "Story Architect project")
            + QLatin1String(" (*.starc)");
}

QString DialogHelper::importFilters()
{
    QString filters;
    filters.append(QApplication::translate("DialogHelper", "All supported files")
                   + QLatin1String(" (*.starc *.kitsp *.fdx *.trelby *.docx *.doc *.odt *.fountain *.celtx)"));
    filters.append(";;");
    filters.append(QApplication::translate("DialogHelper", "Story Architect project")
                   + QLatin1String(" (*.starc)"));
    filters.append(";;");
    filters.append(QApplication::translate("DialogHelper", "KIT Scenarist project")
                   + QLatin1String(" (*.kitsp)"));
    filters.append(";;");
    filters.append(QApplication::translate("DialogHelper", "Final Draft screenplay")
                   + QLatin1String(" (*.fdx)"));
    filters.append(";;");
    filters.append(QApplication::translate("DialogHelper", "Final Draft template")
                   + QLatin1String(" (*.fdxt)"));
    filters.append(";;");
    filters.append(QApplication::translate("DialogHelper", "Trelby screenplay")
                   + QLatin1String(" (*.trelby)"));
    filters.append(";;");
    filters.append(QApplication::translate("DialogHelper", "Office Open XML")
                   + QLatin1String(" (*.docx *.doc)"));
    filters.append(";;");
    filters.append(QApplication::translate("DialogHelper", "OpenDocument text")
                   + QLatin1String(" (*.odt)"));
    filters.append(";;");
    filters.append(QApplication::translate("DialogHelper", "Fountain text")
                   + QLatin1String(" (*.fountain)"));
    filters.append(";;");
    filters.append(QApplication::translate("DialogHelper", "Celtx project")
                   + QLatin1String(" (*.celtx)"));
    return filters;
}
