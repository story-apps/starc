#include "application_view_model.h"

namespace {
    const int kSystemLanguage = -1;
}

class ApplicationViewModel::Implementation
{
public:
    explicit Implementation(const QString& _initialTabBar, const QString& _initialNavigator,
        const QString& _initialEditor);

    const QString initialToolBar;
    QString currentToolBar;
    //
    const QString initialNavigator;
    QString currentNavigator;
    //
    const QString initialView;
    QString currentVew;

    int language = kSystemLanguage;
    bool useDarkTheme = false;
};

ApplicationViewModel::Implementation::Implementation(const QString& _initialTabBar,
    const QString& _initialNavigator, const QString& _initialEditor)
    : initialToolBar(_initialTabBar),
      initialNavigator(_initialNavigator),
      initialView(_initialEditor)
{
}

ApplicationViewModel::ApplicationViewModel(const QString& _initialTabBar,
    const QString& _initialNavigator, const QString& _initialView, QObject* _parent)
    : QObject(_parent),
      d(new Implementation(_initialTabBar, _initialNavigator, _initialView))
{
}

QString ApplicationViewModel::initialToolBar() const
{
    return d->initialToolBar;
}

QString ApplicationViewModel::currentToolBar() const
{
    return d->currentToolBar;
}

void ApplicationViewModel::setCurrentToolBar(const QString& _toolBar)
{
    if (d->currentToolBar == _toolBar) {
        return;
    }

    d->currentToolBar = _toolBar;
    emit currentToolBarChanged(d->currentToolBar);
}

QString ApplicationViewModel::initialNavigator() const
{
    return d->initialNavigator;
}

QString ApplicationViewModel::currentNavigator() const
{
    return d->currentNavigator;
}

void ApplicationViewModel::setCurrentNavigator(const QString& _navigator)
{
    if (d->currentNavigator == _navigator) {
        return;
    }

    d->currentNavigator = _navigator;
    emit currentNavigatorChanged(d->currentNavigator);
}

QString ApplicationViewModel::initialView() const
{
    return d->initialView;
}

ApplicationViewModel::~ApplicationViewModel() = default;

QString ApplicationViewModel::currentView() const
{
    return d->currentVew;
}

void ApplicationViewModel::setCurrentView(const QString& _view)
{
    if (d->currentVew == _view) {
        return;
    }

    d->currentVew = _view;
    emit currentViewChanged(d->currentVew);
}

int ApplicationViewModel::language() const
{
    return d->language;
}

void ApplicationViewModel::setLanguage(int _language)
{
    if (d->language == _language) {
        return;
    }

    d->language = _language;
    emit languageChanged(d->language);
}

bool ApplicationViewModel::useDarkTheme() const
{
    return d->useDarkTheme;
}

void ApplicationViewModel::setUseDarkTheme(bool _use)
{
    if (d->useDarkTheme == _use) {
        return;
    }

    d->useDarkTheme = _use;
    emit useDarkThemeChanged(d->useDarkTheme);
}
