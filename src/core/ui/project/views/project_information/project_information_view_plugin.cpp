#include "project_information_view_plugin.h"

#include "project_information_view.h"


namespace Ui
{

ProjectInformationViewPlugin::ProjectInformationViewPlugin(QObject* _parent)
    : QObject(_parent)
{
}

QWidget* ProjectInformationViewPlugin::createWidget()
{
    return new ProjectInformationView;
}

}
