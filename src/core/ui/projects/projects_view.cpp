#include "projects_view.h"

#include "projects_cards_graphics_view.h"

#include <management_layer/content/projects/projects_model.h>
#include <management_layer/content/projects/projects_model_project_item.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/color_helper.h>

#include <QAction>
#include <QResizeEvent>
#include <QVBoxLayout>


namespace Ui {

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
    void updateToolbarUi();
    void updateToolbarPositon();


    ProjectsView* q = nullptr;

    FloatingToolBar* toolbar = nullptr;

    Widget* emptyPage = nullptr;
    H6Label* emptyPageTitleLabel = nullptr;
    Button* emptyPageCreateProjectButton = nullptr;

    ProjectsCardsGraphicsView* projectsPage = nullptr;
};

ProjectsView::Implementation::Implementation(ProjectsView* _parent)
    : q(_parent)
    , toolbar(new FloatingToolBar(_parent))
    , emptyPage(new Widget(_parent))
    , emptyPageTitleLabel(new H6Label(emptyPage))
    , emptyPageCreateProjectButton(new Button(emptyPage))
    , projectsPage(new ProjectsCardsGraphicsView(_parent))
{
    toolbar->setCurtain(true);

    initEmptyPage();
    initProjectsPage();
}

void ProjectsView::Implementation::initEmptyPage()
{
    QVBoxLayout* layout = new QVBoxLayout(emptyPage);
    layout->setContentsMargins({});
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

    emptyPageTitleLabel->setContentsMargins(DesignSystem::label().margins().toMargins());
    emptyPageTitleLabel->setBackgroundColor(DesignSystem::color().surface());
    emptyPageTitleLabel->setTextColor(DesignSystem::color().onSurface());
    emptyPageCreateProjectButton->setBackgroundColor(DesignSystem::color().accent());
    emptyPageCreateProjectButton->setTextColor(DesignSystem::color().accent());
}

void ProjectsView::Implementation::initProjectsPage()
{
    projectsPage->hide();
    projectsPage->setScaleAvailable(false);
    projectsPage->setCardsRowView(true);
    projectsPage->setCardsInRow(-1);
    projectsPage->setCardsRatio(2);
    projectsPage->setCardsSize(60);
    projectsPage->setCardsSpacing(33);
}

void ProjectsView::Implementation::updateProjectsPageUi()
{
    projectsPage->setBackgroundColor(DesignSystem::color().surface());
}

void ProjectsView::Implementation::updateToolbarUi()
{
    toolbar->resize(toolbar->sizeHint());
    updateToolbarPositon();
    toolbar->setBackgroundColor(ColorHelper::nearby(DesignSystem::color().background()));
    toolbar->setTextColor(DesignSystem::color().onBackground());
    toolbar->raise();
}

void ProjectsView::Implementation::updateToolbarPositon()
{
    toolbar->move(
        QPointF((q->width() - toolbar->width()) / 2.0, -DesignSystem::card().shadowMargins().top())
            .toPoint());
}


// ****


ProjectsView::ProjectsView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    QAction* createProjectAction = new QAction;
    createProjectAction->setIconText(u8"\U000f0415");
    d->toolbar->addAction(createProjectAction);
    connect(createProjectAction, &QAction::triggered, this, &ProjectsView::createProjectPressed);
    QAction* openProjectAction = new QAction;
    openProjectAction->setIconText(u8"\U000f0256");
    d->toolbar->addAction(openProjectAction);
    connect(openProjectAction, &QAction::triggered, this, &ProjectsView::openProjectPressed);

    connect(d->emptyPageCreateProjectButton, &Button::clicked, this,
            &ProjectsView::createProjectPressed);

    connect(d->projectsPage, &ProjectsCardsGraphicsView::hideRequested, this,
            &ProjectsView::showEmptyPage);
    connect(d->projectsPage, &ProjectsCardsGraphicsView::showRequested, this,
            &ProjectsView::showProjectsPage);
    connect(d->projectsPage, &ProjectsCardsGraphicsView::openProjectRequested, this,
            &ProjectsView::openProjectRequested);
    connect(d->projectsPage, &ProjectsCardsGraphicsView::projectContextMenuRequested, this,
            &ProjectsView::projectContextMenuRequested);

    showEmptyPage();
}

ProjectsView::~ProjectsView() = default;

void ProjectsView::setProjects(BusinessLayer::ProjectsModel* _projects)
{
    d->projectsPage->setModel(_projects);
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

    d->updateToolbarPositon();
}

void ProjectsView::updateTranslations()
{
    d->toolbar->actions().at(0)->setToolTip(tr("Create story"));
    d->toolbar->actions().at(1)->setToolTip(tr("Open story"));
    d->emptyPageTitleLabel->setText(tr("Here will be a list of your stories."));
    d->emptyPageCreateProjectButton->setText(tr("It's time to create the first story!"));
}

void ProjectsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    setBackgroundColor(DesignSystem::color().surface());

    d->updateEmptyPageUi();
    d->updateProjectsPageUi();
    d->updateToolbarUi();
}

} // namespace Ui
