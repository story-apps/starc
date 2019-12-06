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

    QAction* stories = nullptr;
    QAction* createStory = nullptr;
    QAction* openStory = nullptr;
    QAction* story = nullptr;
    QAction* saveStory = nullptr;
    QAction* saveStoryAs = nullptr;
    QAction* exportStory = nullptr;
    QAction* importStory = nullptr;
    QAction* settings = nullptr;
    QAction* help = nullptr;
};

MenuView::Implementation::Implementation(QWidget* _parent)
{
    stories = new QAction;
    stories->setIconText("\uf24b");
    stories->setCheckable(true);
    stories->setChecked(true);
    //
    createStory = new QAction;
    createStory->setIconText("\uf415");
    createStory->setCheckable(false);
    //
    openStory = new QAction;
    openStory->setIconText("\uf76f");
    openStory->setCheckable(false);
    //
    story = new QAction;
    story->setIconText("\uf0be");
    story->setCheckable(true);
    story->setVisible(false);
    story->setSeparator(true);
    //
    saveStory = new QAction;
    saveStory->setIconText("\uf193");
    saveStory->setCheckable(false);
    saveStory->setEnabled(false);
    saveStory->setVisible(false);
    //
    saveStoryAs = new QAction;
    saveStoryAs->setIconText({});
    saveStoryAs->setCheckable(false);
    saveStoryAs->setEnabled(false);
    saveStoryAs->setVisible(false);
    //
    importStory = new QAction;
    importStory->setIconText("\uf2fa");
    importStory->setCheckable(true);
    importStory->setVisible(false);
    //
    exportStory = new QAction;
    exportStory->setIconText("\uf207");
    exportStory->setCheckable(false);
    exportStory->setVisible(false);
    //
    settings = new QAction;
    settings->setIconText("\uf493");
    settings->setCheckable(false);
    settings->setVisible(true);
    settings->setSeparator(true);
    //
    help = new QAction;
    help->setIconText("\uf2d7");
    help->setCheckable(false);
    help->setChecked(false);
    help->setSeparator(true);

    QActionGroup* actions = new QActionGroup(_parent);
    actions->addAction(stories);
    actions->addAction(story);
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

    addAction(d->stories);
    addAction(d->createStory);
    addAction(d->openStory);
    addAction(d->story);
    addAction(d->saveStory);
    addAction(d->saveStoryAs);
    addAction(d->importStory);
    addAction(d->exportStory);
    addAction(d->settings);
    addAction(d->help);

    connect(d->stories, &QAction::triggered, this, &MenuView::storiesPressed);
    connect(d->createStory, &QAction::triggered, this, &MenuView::createStoryPressed);
    connect(d->openStory, &QAction::triggered, this, &MenuView::openStoryPressed);
    connect(d->story, &QAction::triggered, this, &MenuView::storyPressed);
    connect(d->saveStory, &QAction::triggered, this, &MenuView::saveChangesPressed);
    connect(d->saveStoryAs, &QAction::triggered, this, &MenuView::saveStoryAsPressed);
    connect(d->importStory, &QAction::triggered, this, &MenuView::importPressed);
    connect(d->exportStory, &QAction::triggered, this, &MenuView::exportPressed);
    connect(d->settings, &QAction::triggered, this, &MenuView::settingsPressed);
    connect(d->help, &QAction::triggered, this, &MenuView::helpPressed);

    auto closeMenu = [this] { WAF::Animation::sideSlideOut(this); };
    connect(this, &MenuView::storiesPressed, this, closeMenu);
    connect(this, &MenuView::createStoryPressed, this, closeMenu);
    connect(this, &MenuView::openStoryPressed, this, closeMenu);
    connect(this, &MenuView::storyPressed, this, closeMenu);
    connect(this, &MenuView::saveStoryAsPressed, this, closeMenu);
    connect(this, &MenuView::importPressed, this, closeMenu);
    connect(this, &MenuView::exportPressed, this, closeMenu);
    connect(this, &MenuView::settingsPressed, this, closeMenu);
    connect(this, &MenuView::helpPressed, this, closeMenu);

    setVisible(false);
}

void MenuView::checkStories()
{
    QSignalBlocker signalBlocker(this);
    d->stories->setChecked(true);
}

void MenuView::setStoryActionsVisible(bool _visible)
{
    d->story->setVisible(_visible);
    d->saveStory->setVisible(_visible);
    d->saveStoryAs->setVisible(_visible);
    d->exportStory->setVisible(_visible);
    d->importStory->setVisible(_visible);
}

void MenuView::checkStory()
{
    QSignalBlocker signalBlocker(this);
    d->story->setChecked(true);
}

void MenuView::setStoryTitle(const QString& _title)
{
    d->story->setText(_title);
}

void MenuView::checkSettings()
{
    QSignalBlocker signalBlocker(this);
    d->importStory->setChecked(true);
}

void MenuView::markChangesSaved(bool _saved)
{
    d->saveStory->setEnabled(!_saved);
    d->saveStory->setText(_saved ? tr("All changes saved") : tr("Save changes"));
}

void MenuView::setProVersion(bool _isPro)
{
    setSubtitle(QString("Story Architect %1 version")
                .arg(_isPro ? "Pro" : "Free"));
    d->help->setVisible(!_isPro);
}

void MenuView::updateTranslations()
{
    d->stories->setText(tr("Stories"));
    d->createStory->setText(tr("Create story"));
    d->openStory->setText(tr("Open story"));
    d->saveStory->setText(tr("Save current story"));
    d->saveStoryAs->setText(d->saveStory->isEnabled() ? tr("Save changes") : tr("All changes saved"));
    d->importStory->setText(tr("Import..."));
    d->exportStory->setText(tr("Export..."));
    d->settings->setText(tr("Application settings"));
    d->help->setText(tr("How to use the application"));
}

MenuView::~MenuView() = default;

} // namespace Ui
