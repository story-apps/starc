#include "projects_manager.h"

#include <QWidget>


namespace ManagementLayer
{

class ProjectsManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    QWidget* toolBar = nullptr;
    QWidget* navigator = nullptr;
    QWidget* view = nullptr;
};

ProjectsManager::Implementation::Implementation(QWidget* _parent)
    : toolBar(new QWidget(_parent)),
      navigator(new QWidget(_parent)),
      view(new QWidget(_parent))
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
