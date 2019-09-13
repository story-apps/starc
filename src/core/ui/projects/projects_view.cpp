#include "projects_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>

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


    ProjectsView* q = nullptr;

    Widget* emptyPage = nullptr;
    H6Label* emptyPageTitleLabel = nullptr;
    Button* emptyPageCreateStoryButton = nullptr;

    Widget* projectsPage = nullptr;
};

ProjectsView::Implementation::Implementation(ProjectsView* _parent)
    : q(_parent),
      emptyPage(new Widget(_parent)),
      projectsPage(new Widget(_parent))
{
    initEmptyPage();
    projectsPage->hide();
}

void ProjectsView::Implementation::initEmptyPage()
{
    emptyPage->hide();

    emptyPageTitleLabel = new H6Label(emptyPage);
    emptyPageCreateStoryButton = new Button(emptyPage);


    QVBoxLayout* layout = new QVBoxLayout(emptyPage);
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(emptyPageTitleLabel, 0, Qt::AlignHCenter);
    layout->addWidget(emptyPageCreateStoryButton, 0, Qt::AlignHCenter);
    layout->addStretch();
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

    d->updateEmptyPageUi();
}

} // namespace Ui
