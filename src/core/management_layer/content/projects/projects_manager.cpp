#include "projects_manager.h"
#include <domain/project.h>

#include <ui/projects/create_project_dialog.h>
#include <ui/projects/projects_navigator.h>
#include <ui/projects/projects_tool_bar.h>
#include <ui/projects/projects_view.h>

#include <QTimer>
#include <QWidget>


namespace ManagementLayer
{

class ProjectsManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Domain::ProjectsModel* projects = nullptr;

    Ui::ProjectsToolBar* toolBar = nullptr;
    Ui::ProjectsNavigator* navigator = nullptr;
    Ui::ProjectsView* view = nullptr;
};

ProjectsManager::Implementation::Implementation(QWidget* _parent)
    : projects(new Domain::ProjectsModel(_parent)),
      toolBar(new Ui::ProjectsToolBar(_parent)),
      navigator(new Ui::ProjectsNavigator(_parent)),
      view(new Ui::ProjectsView(_parent))
{
    toolBar->hide();

    navigator->hide();

    view->setProjects(projects);
    view->hide();
}


// ****


ProjectsManager::ProjectsManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{
    connect(d->view, &Ui::ProjectsView::createStoryPressed, this, [this, _parentWidget] {
        Ui::CreateProjectDialog* dlg = new Ui::CreateProjectDialog(_parentWidget);
        dlg->showDialog();
//        QTimer::singleShot(3000, dlg, [dlg] {
//            dlg->hideDialog();
//        });
//        d->projects->addProject("test");
    });
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
