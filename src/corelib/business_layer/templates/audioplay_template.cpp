#include "audioplay_template.h"

#include <QCoreApplication>


namespace BusinessLayer {

AudioplayTemplate::AudioplayTemplate()
    : TextTemplate()
{
}

AudioplayTemplate::AudioplayTemplate(const QString& _fromFile)
    : TextTemplate(_fromFile)
{
}

QString AudioplayTemplate::name() const
{
    if (id() == "bbc_scene") {
        return QCoreApplication::translate("BusinessLayer::AudioplayTemplate",
                                           "BBC scene style template (page: A4; font: Arial)");
    } else if (id() == "us") {
        return QCoreApplication::translate("BusinessLayer::AudioplayTemplate",
                                           "US template (page: Letter; font: Courier Prime)");
    } else {
        return TextTemplate::name();
    }
}

bool AudioplayTemplate::canMergeParagraph() const
{
    return false;
}

} // namespace BusinessLayer
