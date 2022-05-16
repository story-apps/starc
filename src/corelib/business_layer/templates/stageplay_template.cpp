#include "stageplay_template.h"

#include <QCoreApplication>


namespace BusinessLayer {

StageplayTemplate::StageplayTemplate()
    : TextTemplate()
{
}

StageplayTemplate::StageplayTemplate(const QString& _fromFile)
    : TextTemplate(_fromFile)
{
}

QString StageplayTemplate::name() const
{
    if (id() == "bbc") {
        return QCoreApplication::translate("BusinessLayer::StageplayTemplate",
                                           "BBC stage template (page: A4; font: Arial)");
    } else if (id() == "us") {
        return QCoreApplication::translate("BusinessLayer::StageplayTemplate",
                                           "US template (page: Letter; font: Courier Prime)");
    } else {
        return TextTemplate::name();
    }
}

bool StageplayTemplate::canMergeParagraph() const
{
    return false;
}

} // namespace BusinessLayer
