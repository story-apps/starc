#include "novel_template.h"

#include <QCoreApplication>


namespace BusinessLayer {


NovelTemplate::NovelTemplate()
    : TextTemplate()
{
}

NovelTemplate::NovelTemplate(const QString& _fromFile)
    : TextTemplate(_fromFile)
{
}

QString NovelTemplate::name() const
{
    if (id() == "manuscript_cp_a4") {
        return QCoreApplication::translate("BusinessLayer::TextTemplate",
                                           "Manuscript template (page: A4; font: Courier Prime)");
    } else if (id() == "manuscript_cn_a4") {
        return QCoreApplication::translate("BusinessLayer::TextTemplate",
                                           "Manuscript template (page: A4; font: Courier New)");
    } else if (id() == "manuscript_t_a4") {
        return QCoreApplication::translate("BusinessLayer::TextTemplate",
                                           "Manuscript template (page: A4; font: Times New Roman)");
    } else if (id() == "manuscript_cp_letter") {
        return QCoreApplication::translate(
            "BusinessLayer::TextTemplate",
            "Manuscript template (page: Letter; font: Courier Prime)");
    } else if (id() == "manuscript_t_letter") {
        return QCoreApplication::translate(
            "BusinessLayer::TextTemplate",
            "Manuscript template (page: Letter; font: Times New Roman)");
    } else if (id() == "modern_a4") {
        return QCoreApplication::translate("BusinessLayer::TextTemplate",
                                           "Modern template (page: A4; font: Arial)");
    } else if (id() == "modern_letter") {
        return QCoreApplication::translate("BusinessLayer::TextTemplate",
                                           "Modern template (page: Letter; font: Arial)");
    } else {
        return TextTemplate::name();
    }
}

} // namespace BusinessLayer
