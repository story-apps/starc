#include "projects_view.h"

#include "projects_cards.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <ui/widgets/label/label.h>

#include <QAction>
#include <QResizeEvent>
#include <QVBoxLayout>


namespace Ui
{

class ProjectsView::Implementation
{
public:
    explicit Implementation(ProjectsView* _parent);

    /**
     * @brief Настроить страницу без проектов
     */
    void initEmptyPage();

    /**
     * @brief Обновить настройки UI страницы без проктов
     */
    void updateEmptyPageUi();

    /**
     * @brief Настроить страницу со списом проектов
     */
    void initProjectsPage();

    /**
     * @brief Обновить настройки UI страницы со списком проектов
     */
    void updateProjectsPageUi();

    /**
     * @brief Обновить настройки UI панели инструментов
     */
    void updateToolBarsUi();


    ProjectsView* q = nullptr;

    FloatingToolBar* toolBar = nullptr;

    Widget* emptyPage = nullptr;
    H6Label* emptyPageTitleLabel = nullptr;
    Button* emptyPageCreateProjectButton = nullptr;

    ProjectsCards* projectsPage = nullptr;
};

ProjectsView::Implementation::Implementation(ProjectsView* _parent)
    : q(_parent),
      toolBar(new FloatingToolBar(_parent)),
      emptyPage(new Widget(_parent)),
      projectsPage(new ProjectsCards(_parent))
{
    initEmptyPage();
    initProjectsPage();
}

void ProjectsView::Implementation::initEmptyPage()
{
    emptyPageTitleLabel = new H6Label(emptyPage);
    emptyPageCreateProjectButton = new Button(emptyPage);


    QVBoxLayout* layout = new QVBoxLayout(emptyPage);
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(emptyPageTitleLabel, 0, Qt::AlignHCenter);
    layout->addWidget(emptyPageCreateProjectButton, 0, Qt::AlignHCenter);
    layout->addStretch();

    emptyPage->hide();
}

void ProjectsView::Implementation::updateEmptyPageUi()
{
    emptyPage->setBackgroundColor(DesignSystem::color().surface());

    emptyPageTitleLabel->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    emptyPageTitleLabel->setBackgroundColor(DesignSystem::color().surface());
    emptyPageTitleLabel->setTextColor(DesignSystem::color().onSurface());
    emptyPageCreateProjectButton->setBackgroundColor(DesignSystem::color().secondary());
    emptyPageCreateProjectButton->setTextColor(DesignSystem::color().secondary());
}

void ProjectsView::Implementation::initProjectsPage()
{
    projectsPage->hide();
}

void ProjectsView::Implementation::updateProjectsPageUi()
{
    projectsPage->setBackgroundColor(Ui::DesignSystem::color().surface());
}

void ProjectsView::Implementation::updateToolBarsUi()
{
    toolBar->resize(toolBar->sizeHint());
    toolBar->move(QPointF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px24()).toPoint());
    toolBar->setBackgroundColor(Ui::DesignSystem::color().primary());
    toolBar->setTextColor(Ui::DesignSystem::color().onPrimary());
    toolBar->raise();
}


// ****


ProjectsView::ProjectsView(QWidget* _parent)
    : StackWidget(_parent),
      d(new Implementation(this))
{
    QAction* createProjectAction = new QAction;
    createProjectAction->setIconText(u8"\U000f0415");
    d->toolBar->addAction(createProjectAction);
    connect(createProjectAction, &QAction::triggered, this, &ProjectsView::createProjectPressed);
    QAction* openProjectAction = new QAction;
    openProjectAction->setIconText(u8"\U000f0256");
    d->toolBar->addAction(openProjectAction);
    connect(openProjectAction, &QAction::triggered, this, &ProjectsView::openProjectPressed);

    connect(d->emptyPageCreateProjectButton, &Button::clicked, this, &ProjectsView::createProjectPressed);

    connect(d->projectsPage, &ProjectsCards::hideRequested, this, &ProjectsView::showEmptyPage);
    connect(d->projectsPage, &ProjectsCards::showRequested, this, &ProjectsView::showProjectsPage);
    connect(d->projectsPage, &ProjectsCards::openProjectRequested, this, &ProjectsView::openProjectRequested);
    connect(d->projectsPage, &ProjectsCards::moveProjectToCloudRequested, this, &ProjectsView::moveProjectToCloudRequested);
    connect(d->projectsPage, &ProjectsCards::hideProjectRequested, this, &ProjectsView::hideProjectRequested);
    connect(d->projectsPage, &ProjectsCards::changeProjectNameRequested, this, &ProjectsView::changeProjectNameRequested);
    connect(d->projectsPage, &ProjectsCards::removeProjectRequested, this, &ProjectsView::removeProjectRequested);

    showEmptyPage();

    designSystemChangeEvent(nullptr);
}

ProjectsView::~ProjectsView() = default;

void ProjectsView::setProjects(ManagementLayer::ProjectsModel* _projects)
{
    d->projectsPage->setProjects(_projects);
}

void ProjectsView::showEmptyPage()
{
    setCurrentWidget(d->emptyPage);
}

void ProjectsView::showProjectsPage()
{
    setCurrentWidget(d->projectsPage);
}

void ProjectsView::resizeEvent(QResizeEvent* _event)
{
    StackWidget::resizeEvent(_event);

    d->toolBar->move(QPointF(Ui::DesignSystem::layout().px24(),
                             Ui::DesignSystem::layout().px24()).toPoint());
}

void ProjectsView::updateTranslations()
{
    d->emptyPageTitleLabel->setText(tr("Here will be a list of your stories."));
    d->emptyPageCreateProjectButton->setText(tr("It's time to create the first story!"));
    d->toolBar->actions().at(0)->setToolTip(tr("Create story"));
    d->toolBar->actions().at(1)->setToolTip(tr("Open story"));
}

void ProjectsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    setBackgroundColor(DesignSystem::color().surface());

    d->updateEmptyPageUi();
    d->updateProjectsPageUi();
    d->updateToolBarsUi();
}

} // namespace Ui
