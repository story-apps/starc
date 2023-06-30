#include "projects_cards_graphics_view.h"

#include "project_card.h"
#include "project_team_card.h"

#include <management_layer/content/projects/projects_model.h>
#include <management_layer/content/projects/projects_model_project_item.h>
#include <ui/modules/cards/cards_graphics_scene.h>


namespace Ui {

ProjectsCardsGraphicsView::ProjectsCardsGraphicsView(QWidget* _parent)
    : CardsGraphicsView(new CardsGraphicsScene(_parent), _parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAdditionalScrollingAvailable(false);

    auto cardsScene = qobject_cast<CardsGraphicsScene*>(scene());
    connect(cardsScene, &CardsGraphicsScene::itemClicked, this, [this](const QModelIndex& _index) {
        const auto projectsModel = qobject_cast<BusinessLayer::ProjectsModel*>(model());
        const auto projectItem = projectsModel->itemForIndex(_index);
        if (projectItem->type() != BusinessLayer::ProjectsModelItemType::Project) {
            return;
        }

        const auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(projectItem);
        emit openProjectRequested(project);
    });
    connect(cardsScene, &CardsGraphicsScene::itemContextMenuRequested, this,
            [this](const QModelIndex& _index) {
                const auto projectsModel = qobject_cast<BusinessLayer::ProjectsModel*>(model());
                const auto projectItem = projectsModel->itemForIndex(_index);
                if (projectItem->type() != BusinessLayer::ProjectsModelItemType::Project) {
                    return;
                }

                const auto project
                    = static_cast<BusinessLayer::ProjectsModelProjectItem*>(projectItem);
                emit projectContextMenuRequested(project);
            });
}

bool ProjectsCardsGraphicsView::excludeFromFlatIndex(const QModelIndex& _index) const
{
    Q_UNUSED(_index)
    return false;
}

AbstractCardItem* ProjectsCardsGraphicsView::createCardFor(const QModelIndex& _index) const
{
    auto projectsModel = qobject_cast<BusinessLayer::ProjectsModel*>(model());
    Q_ASSERT(projectsModel);
    auto item = projectsModel->itemForIndex(_index);
    Q_ASSERT(item);
    if (item->type() == BusinessLayer::ProjectsModelItemType::Team) {
        return new ProjectTeamCard;
    } else {
        return new ProjectCard;
    }
}

} // namespace Ui
