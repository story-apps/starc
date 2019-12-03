#include "settings_manager.h"

#include <ui/settings/settings_navigator.h>
#include <ui/settings/settings_tool_bar.h>
#include <ui/settings/settings_view.h>


namespace ManagementLayer
{

class SettingsManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Ui::SettingsToolBar* toolBar = nullptr;
    Ui::SettingsNavigator* navigator = nullptr;
    Ui::SettingsView* view = nullptr;
};

SettingsManager::Implementation::Implementation(QWidget* _parent)
    : toolBar(new Ui::SettingsToolBar(_parent)),
      navigator(new Ui::SettingsNavigator(_parent)),
      view(new Ui::SettingsView(_parent))
{
    toolBar->hide();
    navigator->hide();
    view->hide();
}


// ****


SettingsManager::SettingsManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{
    connect(d->toolBar, &Ui::SettingsToolBar::backPressed, this, &SettingsManager::closeSettingsRequested);
}

SettingsManager::~SettingsManager() = default;

QWidget* SettingsManager::toolBar() const
{
    return d->toolBar;
}

QWidget* SettingsManager::navigator() const
{
    return d->navigator;
}

QWidget* SettingsManager::view() const
{
    return d->view;
}

}
