#include "projects_manager.h"

#include <ui/projects/projects_navigator.h>
#include <ui/projects/projects_tool_bar.h>
#include <ui/projects/projects_view.h>

#include <QWidget>


namespace ManagementLayer
{

class ProjectsManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Ui::ProjectsToolBar* toolBar = nullptr;
    Ui::ProjectsNavigator* navigator = nullptr;
    Ui::ProjectsView* view = nullptr;
};

ProjectsManager::Implementation::Implementation(QWidget* _parent)
    : toolBar(new Ui::ProjectsToolBar(_parent)),
      navigator(new Ui::ProjectsNavigator(_parent)),
      view(new Ui::ProjectsView(_parent))
{
    toolBar->hide();
    navigator->hide();
    view->hide();
}


// ****


ProjectsManager::ProjectsManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{
}

ProjectsManager::~ProjectsManager() = default;

QWidget* ProjectsManager::toolBar() const
{
    return d->toolBar;
}

QWidget* ProjectsManager::navigator() const
{
    return d->navigator;
}

QWidget* ProjectsManager::view() const
{
    return d->view;
}

} // namespace ManagementLayer
