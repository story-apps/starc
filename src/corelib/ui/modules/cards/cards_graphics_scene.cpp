#include "cards_graphics_scene.h"

#include "abstract_card_item.h"

#include <ui/design_system/design_system.h>

#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QModelIndex>
#include <QScreen>
#include <QSet>


namespace Ui {

namespace {
static qreal s_z = 0.0;
}

class CardsGraphicsScene::Implementation
{
public:
    bool isReadOnly = false;

    QSet<AbstractCardItem*> mouseGrabberItems;
};


// ****


CardsGraphicsScene::CardsGraphicsScene(QObject* _parent)
    : QGraphicsScene(_parent)
    , d(new Implementation)
{
}

CardsGraphicsScene::~CardsGraphicsScene() = default;

bool CardsGraphicsScene::isReadOnly() const
{
    return d->isReadOnly;
}

void CardsGraphicsScene::setReadOnly(bool _readOnly)
{
    d->isReadOnly = _readOnly;
}

void CardsGraphicsScene::fitToContents()
{
    const auto items = this->items();
    auto newSceneRect = sceneRect();
    for (auto item : items) {
        const QRectF movedItemRect(item->scenePos(), item->boundingRect().size());

        const auto epsilon = views().isEmpty() ? Ui::DesignSystem::layout().px62()
                                               : views().constFirst()->screen()->size().width();
        if (movedItemRect.left() < newSceneRect.left()
            || std::abs(newSceneRect.left() - movedItemRect.left()) < epsilon) {
            newSceneRect.setLeft(movedItemRect.left() - epsilon);
        }
        if (movedItemRect.top() < newSceneRect.top()
            || std::abs(newSceneRect.top() - movedItemRect.top()) < epsilon) {
            newSceneRect.setTop(movedItemRect.top() - epsilon);
        }
        if (movedItemRect.right() > newSceneRect.right()
            || (newSceneRect.right() - movedItemRect.right()) < epsilon) {
            newSceneRect.setRight(movedItemRect.right() + epsilon);
        }
        if (movedItemRect.bottom() > newSceneRect.bottom()
            || std::abs(newSceneRect.bottom() - movedItemRect.bottom()) < epsilon) {
            newSceneRect.setBottom(movedItemRect.bottom() + epsilon);
        }
    }

    if (newSceneRect != sceneRect()) {
        setSceneRect(newSceneRect);
    }
}

qreal CardsGraphicsScene::firstZValue() const
{
    s_z = 0.0;
    return s_z;
}

qreal CardsGraphicsScene::nextZValue() const
{
    s_z += 0.001;
    return s_z;
}

const QSet<AbstractCardItem*>& CardsGraphicsScene::mouseGrabberItems() const
{
    return d->mouseGrabberItems;
}

void CardsGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* _event)
{
    if (d->isReadOnly) {
        return;
    }

    QGraphicsScene::mousePressEvent(_event);

    if (auto movedCard = static_cast<AbstractCardItem*>(mouseGrabberItem())) {
        std::function<void(AbstractCardItem*)> addCardWithChildrenToMoveList;
        addCardWithChildrenToMoveList
            = [this, &addCardWithChildrenToMoveList](AbstractCardItem* _card) {
                  d->mouseGrabberItems.insert(_card);

                  const auto children = _card->children();
                  for (auto child : children) {
                      addCardWithChildrenToMoveList(child);
                  }
              };
        addCardWithChildrenToMoveList(movedCard);
    }
}

void CardsGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* _event)
{
    if (d->isReadOnly) {
        return;
    }

    QGraphicsScene::mouseReleaseEvent(_event);

    d->mouseGrabberItems.clear();
}

void CardsGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* _event)
{
    if (d->isReadOnly) {
        return;
    }

    QGraphicsScene::mouseDoubleClickEvent(_event);

    if (auto movedCard = static_cast<AbstractCardItem*>(mouseGrabberItem())) {
        emit itemDoubleClicked(movedCard->modelItemIndex());
    }
}

} // namespace Ui
