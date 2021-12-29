#include "menu_view.h"

#include <ui/design_system/design_system.h>
#include <utils/3rd_party/WAF/Animation/Animation.h>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QKeyEvent>


namespace Ui {

class MenuView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QAction* signIn = nullptr;
    QAction* projects = nullptr;
    QAction* createProject = nullptr;
    QAction* openProject = nullptr;
    QAction* project = nullptr;
    QAction* saveProject = nullptr;
    QAction* saveProjectAs = nullptr;
    QAction* exportCurrentDocument = nullptr;
    QAction* importProject = nullptr;
    QAction* fullScreen = nullptr;
    QAction* settings = nullptr;
};

MenuView::Implementation::Implementation(QWidget* _parent)
{
    signIn = new QAction;
    signIn->setIconText(u8"\U000F0004");
    signIn->setCheckable(false);
    signIn->setVisible(false);
    //
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
    saveProjectAs->setIconText(" ");
    saveProjectAs->setCheckable(false);
    saveProjectAs->setVisible(false);
    //
    importProject = new QAction;
    importProject->setIconText(u8"\U000f02fa");
    importProject->setCheckable(false);
    importProject->setVisible(false);
    //
    exportCurrentDocument = new QAction;
    exportCurrentDocument->setIconText(u8"\U000f0207");
    exportCurrentDocument->setCheckable(false);
    exportCurrentDocument->setEnabled(false);
    exportCurrentDocument->setVisible(false);
    //
    fullScreen = new QAction;
    fullScreen->setIconText(u8"\U000F0293");
    fullScreen->setCheckable(false);
    fullScreen->setVisible(false);
    fullScreen->setSeparator(true);
    //
    settings = new QAction;
    settings->setIconText(u8"\U000f0493");
    settings->setCheckable(false);
    settings->setVisible(true);
    settings->setSeparator(true);

    QActionGroup* actions = new QActionGroup(_parent);
    actions->addAction(projects);
    actions->addAction(project);
    actions->addAction(settings);
}

// ****


MenuView::MenuView(QWidget* _parent)
    : Drawer(_parent)
    , d(new Implementation(this))
{

#ifdef CLOUD_SERVICE_MANAGER
    d->signIn->setVisible(true);
    d->projects->setSeparator(true);
#endif

    setProVersion(false);

    addAction(d->signIn);
    addAction(d->projects);
    addAction(d->createProject);
    addAction(d->openProject);
    addAction(d->project);
    addAction(d->saveProject);
    addAction(d->saveProjectAs);
    addAction(d->importProject);
    addAction(d->exportCurrentDocument);
    addAction(d->fullScreen);
    addAction(d->settings);

    connect(d->signIn, &QAction::triggered, this, &MenuView::signInPressed);
    connect(d->projects, &QAction::triggered, this, &MenuView::projectsPressed);
    connect(d->createProject, &QAction::triggered, this, &MenuView::createProjectPressed);
    connect(d->openProject, &QAction::triggered, this, &MenuView::openProjectPressed);
    connect(d->project, &QAction::triggered, this, &MenuView::projectPressed);
    connect(d->saveProject, &QAction::triggered, this, &MenuView::saveProjectChangesPressed);
    connect(d->saveProjectAs, &QAction::triggered, this, &MenuView::saveProjectAsPressed);
    connect(d->importProject, &QAction::triggered, this, &MenuView::importPressed);
    connect(d->exportCurrentDocument, &QAction::triggered, this,
            &MenuView::exportCurrentDocumentPressed);
    connect(d->fullScreen, &QAction::triggered, this, &MenuView::fullscreenPressed);
    connect(d->settings, &QAction::triggered, this, &MenuView::settingsPressed);

    connect(this, &MenuView::accountPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::projectsPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::createProjectPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::openProjectPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::projectPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::saveProjectAsPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::importPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::exportCurrentDocumentPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::fullscreenPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::settingsPressed, this, &MenuView::closeMenu);
    connect(this, &MenuView::helpPressed, this, &MenuView::closeMenu);

    setVisible(false);
}

void MenuView::setSignInVisible(bool _visible)
{
    d->signIn->setVisible(_visible);
}

MenuView::~MenuView() = default;

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
    d->exportCurrentDocument->setVisible(_visible);
    d->importProject->setVisible(_visible);
    d->fullScreen->setVisible(_visible);
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
    //    setSubtitle(QString("Story Architect v.%1 %2")
    //                    .arg(QApplication::applicationVersion(), (_isPro ? "PRO" : "free")));
}

void MenuView::setCurrentDocumentExportAvailable(bool _available)
{
    d->exportCurrentDocument->setEnabled(_available);
}

void MenuView::closeMenu()
{
    WAF::Animation::sideSlideOut(this);
}

void MenuView::updateTranslations()
{
    d->signIn->setText(tr("Sign in"));
    d->projects->setText(tr("Stories"));
    d->createProject->setText(tr("Create story"));
    d->openProject->setText(tr("Open story"));
    d->saveProject->setText(d->saveProject->isEnabled() ? tr("Save changes")
                                                        : tr("All changes saved"));
    d->saveProject->setWhatsThis(
        QKeySequence(QKeySequence::Save).toString(QKeySequence::NativeText));
    d->saveProjectAs->setText(tr("Save current story as..."));
    d->importProject->setText(tr("Import..."));
    d->importProject->setWhatsThis(QKeySequence("Alt+I").toString(QKeySequence::NativeText));
    d->exportCurrentDocument->setText(tr("Export current document..."));
    d->exportCurrentDocument->setWhatsThis(
        QKeySequence("Alt+E").toString(QKeySequence::NativeText));
    d->fullScreen->setText(tr("Toggle full screen"));
    d->fullScreen->setWhatsThis(
        QKeySequence(QKeySequence::FullScreen).toString(QKeySequence::NativeText));
    d->settings->setText(tr("Application settings"));
}

void MenuView::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Escape) {
        closeMenu();
    }

    Drawer::keyPressEvent(_event);
}

} // namespace Ui
