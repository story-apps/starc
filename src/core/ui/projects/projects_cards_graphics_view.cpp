#include "projects_cards_graphics_view.h"

#include "project_card.h"

#include <management_layer/content/projects/project.h>
#include <ui/modules/cards/cards_graphics_scene.h>


namespace Ui {

ProjectsCardsGraphicsView::ProjectsCardsGraphicsView(QWidget* _parent)
    : CardsGraphicsView(new CardsGraphicsScene(_parent), _parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAdditionalScrollingAvailable(false);

    auto cardsScene = qobject_cast<CardsGraphicsScene*>(scene());
    connect(cardsScene, &CardsGraphicsScene::itemClicked, this, [this](const QModelIndex& _index) {
        const auto projectsModel = qobject_cast<ManagementLayer::ProjectsModel*>(model());
        const auto project = projectsModel->projectForIndex(_index);
        emit openProjectRequested(*project);
    });
    connect(cardsScene, &CardsGraphicsScene::itemContextMenuRequested, this,
            [this](const QModelIndex& _index) {
                const auto projectsModel = qobject_cast<ManagementLayer::ProjectsModel*>(model());
                const auto project = projectsModel->projectForIndex(_index);
                emit projectContextMenuRequested(*project);
            });
}

bool ProjectsCardsGraphicsView::excludeFromFlatIndex(const QModelIndex& _index) const
{
    Q_UNUSED(_index)
    return false;
}

AbstractCardItem* ProjectsCardsGraphicsView::createCardFor(const QModelIndex& _index) const
{
    Q_UNUSED(_index)
    return new ProjectCard;
}

} // namespace Ui
