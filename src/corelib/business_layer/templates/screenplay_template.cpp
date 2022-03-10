#include "screenplay_template.h"

#include <QCoreApplication>


namespace BusinessLayer {

ScreenplayTemplate::ScreenplayTemplate()
    : TextTemplate()
{
}

ScreenplayTemplate::ScreenplayTemplate(const QString& _fromFile)
    : TextTemplate(_fromFile)
{
}

QString ScreenplayTemplate::name() const
{
    if (id() == "world_cp") {
        return QCoreApplication::translate(
            "BusinessLayer::ScreenplayTemplate",
            "International template (page: A4; font: Courier Prime)");
    } else if (id() == "world_cn") {
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "International template (page: A4; font: Courier New)");
    } else if (id() == "ar") {
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "Arabic template (page: A4; font: Courier New)");
    } else if (id() == "he") {
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "Hebrew template (page: A4; font: Arial)");
    } else if (id() == "ru") {
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "Russian template (page: A4; font: Courier New)");
    } else if (id() == "tamil") {
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "Tamil template (page: A4; font: Mukta Malar)");
    } else if (id() == "us") {
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "US template (page: Letter; font: Courier Prime)");
    } else {
        return TextTemplate::name();
    }
}

} // namespace BusinessLayer
