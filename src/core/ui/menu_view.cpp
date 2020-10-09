#include "menu_view.h"

#include <ui/design_system/design_system.h>

#include <utils/3rd_party/WAF/Animation/Animation.h>

#include <QAction>
#include <QApplication>
#include <QActionGroup>


namespace Ui
{

class MenuView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    QAction* projects = nullptr;
    QAction* createProject = nullptr;
    QAction* openProject = nullptr;
    QAction* project = nullptr;
    QAction* saveProject = nullptr;
    QAction* saveProjectAs = nullptr;
    QAction* exportProject = nullptr;
    QAction* importProject = nullptr;
    QAction* settings = nullptr;
    QAction* help = nullptr;
};

MenuView::Implementation::Implementation(QWidget* _parent)
{
    projects = new QAction;
    projects->setIconText(u8"\U000f024b");
    projects->setCheckable(true);
    projects->setChecked(true);
    //
    createProject = new QAction;
    createProject->setIconText(u8"\U000f0415");
    createProject->setCheckable(false);
    //
    openProject = new QAction;
    openProject->setIconText(u8"\U000f0770");
    openProject->setCheckable(false);
    //
    project = new QAction;
    project->setIconText(u8"\U000f00be");
    project->setCheckable(true);
    project->setVisible(false);
    project->setSeparator(true);
    //
    saveProject = new QAction;
    saveProject->setIconText(u8"\U000f0193");
    saveProject->setCheckable(false);
    saveProject->setEnabled(false);
    saveProject->setVisible(false);
    //
    saveProjectAs = new QAction;
    saveProjectAs->setIconText({});
    saveProjectAs->setCheckable(false);
    saveProjectAs->setEnabled(false);
    saveProjectAs->setVisible(false);
    //
    importProject = new QAction;
    importProject->setIconText(u8"\U000f02fa");
    importProject->setCheckable(false);
    importProject->setVisible(false);
    //
    exportProject = new QAction;
    exportProject->setIconText(u8"\U000f0207");
    exportProject->setCheckable(false);
    exportProject->setVisible(false);
    //
    settings = new QAction;
    settings->setIconText(u8"\U000f0493");
    settings->setCheckable(false);
    settings->setVisible(true);
    settings->setSeparator(true);
    //
    help = new QAction;
    help->setIconText(u8"\U000f02d7");
    help->setCheckable(false);
    help->setChecked(false);
    help->setSeparator(true);

    QActionGroup* actions = new QActionGroup(_parent);
    actions->addAction(projects);
    actions->addAction(project);
    actions->addAction(settings);
    actions->addAction(help);
}


// ****


MenuView::MenuView(QWidget* _parent)
    : Drawer(_parent),
      d(new Implementation(this))
{
    setTitle("STARC");
    setProVersion(false);

    addAction(d->projects);
    addAction(d->createProject);
    addAction(d->openProject);
    addAction(d->project);
    addAction(d->saveProject);
    addAction(d->saveProjectAs);
    addAction(d->importProject);
    addAction(d->exportProject);
    addAction(d->settings);
    addAction(d->help);

    connect(d->projects, &QAction::triggered, this, &MenuView::projectsPressed);
    connect(d->createProject, &QAction::triggered, this, &MenuView::createProjectPressed);
    connect(d->openProject, &QAction::triggered, this, &MenuView::openProjectPressed);
    connect(d->project, &QAction::triggered, this, &MenuView::projectPressed);
    connect(d->saveProject, &QAction::triggered, this, &MenuView::saveChangesPressed);
    connect(d->saveProjectAs, &QAction::triggered, this, &MenuView::saveProjectAsPressed);
    connect(d->importProject, &QAction::triggered, this, &MenuView::importPressed);
    connect(d->exportProject, &QAction::triggered, this, &MenuView::exportPressed);
    connect(d->settings, &QAction::triggered, this, &MenuView::settingsPressed);
    connect(d->help, &QAction::triggered, this, &MenuView::helpPressed);

    auto closeMenu = [this] { WAF::Animation::sideSlideOut(this); };
    connect(this, &MenuView::projectsPressed, this, closeMenu);
    connect(this, &MenuView::createProjectPressed, this, closeMenu);
    connect(this, &MenuView::openProjectPressed, this, closeMenu);
    connect(this, &MenuView::projectPressed, this, closeMenu);
    connect(this, &MenuView::saveProjectAsPressed, this, closeMenu);
    connect(this, &MenuView::importPressed, this, closeMenu);
    connect(this, &MenuView::exportPressed, this, closeMenu);
    connect(this, &MenuView::settingsPressed, this, closeMenu);
    connect(this, &MenuView::helpPressed, this, closeMenu);

    setVisible(false);
}

void MenuView::checkProjects()
{
    QSignalBlocker signalBlocker(this);
    d->projects->setChecked(true);
}

void MenuView::setProjectActionsVisible(bool _visible)
{
    d->project->setVisible(_visible);
    d->saveProject->setVisible(_visible);
    d->saveProjectAs->setVisible(_visible);
    d->exportProject->setVisible(_visible);
    d->importProject->setVisible(_visible);
}

void MenuView::checkProject()
{
    QSignalBlocker signalBlocker(this);
    d->project->setChecked(true);
}

void MenuView::setProjectTitle(const QString& _title)
{
    d->project->setText(_title);
}

void MenuView::checkSettings()
{
    QSignalBlocker signalBlocker(this);
    d->importProject->setChecked(true);
}

void MenuView::markChangesSaved(bool _saved)
{
    d->saveProject->setEnabled(!_saved);
    d->saveProject->setText(_saved ? tr("All changes saved") : tr("Save changes"));
    update();
}

void MenuView::setProVersion(bool _isPro)
{
    setSubtitle(QString("Story Architect v.%1 %2")
                .arg(QApplication::applicationVersion())
                .arg(_isPro ? "PRO" : "free"));
    d->help->setVisible(!_isPro);
}

void MenuView::updateTranslations()
{
    d->projects->setText(tr("Stories"));
    d->createProject->setText(tr("Create story"));
    d->openProject->setText(tr("Open story"));
    d->saveProject->setText(d->saveProject->isEnabled() ? tr("Save changes") : tr("All changes saved"));
    d->saveProject->setWhatsThis(QKeySequence(QKeySequence::Save).toString());
    d->saveProjectAs->setText(tr("Save current story as..."));
    d->importProject->setText(tr("Import..."));
    d->exportProject->setText(tr("Export..."));
    d->settings->setText(tr("Application settings"));
    d->help->setText(tr("How to use the application"));
}

MenuView::~MenuView() = default;

} // namespace Ui
