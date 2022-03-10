#include "simple_text_template.h"

#include <QCoreApplication>


namespace BusinessLayer {


SimpleTextTemplate::SimpleTextTemplate()
    : TextTemplate()
{
}

SimpleTextTemplate::SimpleTextTemplate(const QString& _fromFile)
    : TextTemplate(_fromFile)
{
}

QString SimpleTextTemplate::name() const
{
    if (id() == "mono_cp_a4") {
        return QCoreApplication::translate("BusinessLayer::TextTemplate",
                                           "Monotype template (page: A4; font: Courier Prime)");
    } else if (id() == "mono_cn_a4") {
        return QCoreApplication::translate("BusinessLayer::TextTemplate",
                                           "Monotype template (page: A4; font: Courier New)");
    } else if (id() == "mono_cp_letter") {
        return QCoreApplication::translate("BusinessLayer::TextTemplate",
                                           "Monotype template (page: Letter; font: Courier Prime)");
    } else if (id() == "sans_a4") {
        return QCoreApplication::translate("BusinessLayer::TextTemplate",
                                           "Sans serif template (page: A4; font: Roboto)");
    } else if (id() == "sans_letter") {
        return QCoreApplication::translate("BusinessLayer::TextTemplate",
                                           "Sans serif template (page: Letter; font: Roboto)");
    } else {
        return name();
    }
}

} // namespace BusinessLayer
