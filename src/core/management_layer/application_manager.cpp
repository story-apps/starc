#include "application_manager.h"

#include <QWidget>


namespace ManagementLayer {

class ApplicationManager::ApplicationManagerPrivate
{
public:
    ApplicationManagerPrivate();

    QScopedPointer<QWidget> applicationView;
};

ApplicationManager::ApplicationManagerPrivate::ApplicationManagerPrivate()
    : applicationView(new QWidget)
{
}


// ****


ApplicationManager::ApplicationManager(QObject* _parent)
    : QObject(_parent),
      IApplicationManager(),
      d(new ApplicationManagerPrivate)
{
}

ApplicationManager::~ApplicationManager() = default;

void ApplicationManager::exec()
{
    d->applicationView->show();
}

} // namespace ManagementLayer
