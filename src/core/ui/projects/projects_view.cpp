#include "projects_view.h"

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
     * @brief Обновить настройки UI панели инструментов
     */
    void updateToolBarsUi();

    /**
     * @brief Обновить настройки UI страницы без проктов
     */
    void updateEmptyPageUi();


    ProjectsView* q = nullptr;

    FloatingToolBar* toolBar = nullptr;
    FloatingToolBar* accountBar = nullptr;

    Widget* emptyPage = nullptr;
    H6Label* emptyPageTitleLabel = nullptr;
    Button* emptyPageCreateStoryButton = nullptr;

    Widget* projectsPage = nullptr;
};

ProjectsView::Implementation::Implementation(ProjectsView* _parent)
    : q(_parent),
      toolBar(new FloatingToolBar(_parent)),
      accountBar(new FloatingToolBar(_parent)),
      emptyPage(new Widget(_parent)),
      projectsPage(new Widget(_parent))
{
    initEmptyPage();
    projectsPage->hide();
}

void ProjectsView::Implementation::initEmptyPage()
{
    emptyPageTitleLabel = new H6Label(emptyPage);
    emptyPageCreateStoryButton = new Button(emptyPage);


    QVBoxLayout* layout = new QVBoxLayout(emptyPage);
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(emptyPageTitleLabel, 0, Qt::AlignHCenter);
    layout->addWidget(emptyPageCreateStoryButton, 0, Qt::AlignHCenter);
    layout->addStretch();

    emptyPage->hide();
}

void ProjectsView::Implementation::updateToolBarsUi()
{
    toolBar->resize(toolBar->sizeHint());
    toolBar->move(QPointF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px24()).toPoint());
    toolBar->setBackgroundColor(Ui::DesignSystem::color().primary());
    toolBar->setTextColor(Ui::DesignSystem::color().onPrimary());
    toolBar->raise();

    accountBar->resize(accountBar->sizeHint());
    accountBar->move(QPointF(q->size().width()
                             - accountBar->width()
                             - Ui::DesignSystem::layout().px24(),
                             Ui::DesignSystem::layout().px24()).toPoint());
    accountBar->setBackgroundColor(Ui::DesignSystem::color().primary());
    accountBar->setTextColor(Ui::DesignSystem::color().onPrimary());
    accountBar->raise();
}

void ProjectsView::Implementation::updateEmptyPageUi()
{
    emptyPage->setBackgroundColor(DesignSystem::color().surface());

    emptyPageTitleLabel->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    emptyPageTitleLabel->setBackgroundColor(DesignSystem::color().surface());
    emptyPageTitleLabel->setTextColor(DesignSystem::color().onSurface());
    emptyPageCreateStoryButton->setBackgroundColor(DesignSystem::color().secondary());
    emptyPageCreateStoryButton->setTextColor(DesignSystem::color().secondary());
}


// ****


ProjectsView::ProjectsView(QWidget* _parent)
    : StackWidget(_parent),
      d(new Implementation(this))
{
    QAction* createStoryAction = new QAction("\uf415");
    d->toolBar->addAction(createStoryAction);
    connect(createStoryAction, &QAction::triggered, this, &ProjectsView::createStoryPressed);
    QAction* openStoryAction = new QAction("\uf256");
    d->toolBar->addAction(openStoryAction);
    connect(openStoryAction, &QAction::triggered, this, &ProjectsView::openStoryPressed);

    QAction* accountAction = new QAction("\uf004");
    d->accountBar->addAction(accountAction);
    connect(accountAction, &QAction::triggered, this, &ProjectsView::accountPressed);

    showEmptyPage();

    designSystemChangeEvent(nullptr);
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
    d->accountBar->move(QPointF(_event->size().width()
                                - d->accountBar->width()
                                - Ui::DesignSystem::layout().px24(),
                                Ui::DesignSystem::layout().px24()).toPoint());
}

void ProjectsView::updateTranslations()
{
    d->emptyPageTitleLabel->setText(tr("Here will be a list of your stories."));
    d->emptyPageCreateStoryButton->setText(tr("It's time to create the first story!"));
}

ProjectsView::~ProjectsView() = default;

void ProjectsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    setBackgroundColor(DesignSystem::color().surface());

    d->updateToolBarsUi();
    d->updateEmptyPageUi();
}

} // namespace Ui
