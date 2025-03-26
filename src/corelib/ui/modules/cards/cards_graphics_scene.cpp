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
    /**
     * @brief Запрещено ли редактирование содержимого
     */
    bool isReadOnly = false;

    /**
     * @brief Необходимо ли добавлять дополнительную прокрутку
     */
    bool isAdditionalScrollingAvailable = true;

    /**
     * @brief Список таскаемых карточек
     */
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

void CardsGraphicsScene::setAdditionalScrollingAvailable(bool _available)
{
    if (d->isAdditionalScrollingAvailable == _available) {
        return;
    }

    d->isAdditionalScrollingAvailable = _available;
    fitToContents();
}

void CardsGraphicsScene::fitToContents()
{
    QRectF newSceneRect;
    if (d->isAdditionalScrollingAvailable) {
        const auto items = this->items();
        newSceneRect = sceneRect();
        for (auto item : items) {
            const QRectF movedItemRect(item->scenePos(), item->boundingRect().size());

            const auto epsilon = 0;
            views().isEmpty() ? DesignSystem::layout().px62()
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
    } else {
        const QSizeF viewSize = [this] {
            if (views().isEmpty()) {
                return QSizeF();
            }

            const QGraphicsView* view = views().constFirst();
            return QSizeF(view->viewport()->size());
        }();
        const int cardsInRowCount = [width = viewSize.width()]() mutable {
            int count = 0;
            width -= DesignSystem::projectCard().margins().left();
            width -= DesignSystem::projectCard().margins().right();
            forever
            {
                width -= count > 0 ? DesignSystem::projectCard().spacing() : 0;
                width -= DesignSystem::projectCard().size().width();

                if (width > 0) {
                    ++count;
                } else {
                    break;
                }
            }
            return std::max(1, count);
        }();
        const qreal sceneRectWidth
            = std::max(DesignSystem::projectCard().margins().left()
                           + DesignSystem::projectCard().size().width() * cardsInRowCount
                           + DesignSystem::projectCard().spacing() * (cardsInRowCount - 1)
                           + DesignSystem::projectCard().margins().right(),
                       viewSize.width());
        const auto items = this->items();
        qreal maxY = 0.0;
        for (auto item : items) {
            maxY = std::max(item->pos().y() + item->boundingRect().height(), maxY);
        }
        newSceneRect.setRight(sceneRectWidth);
        newSceneRect.setBottom(
            std::max(viewSize.height(), maxY + DesignSystem::projectCard().margins().bottom()));
    }

    if (newSceneRect != sceneRect()) {
        setSceneRect(newSceneRect);
        for (auto view : views()) {
            view->setSceneRect(newSceneRect);
        }
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
    QGraphicsScene::mousePressEvent(_event);

    if (d->isReadOnly) {
        return;
    }

    if (auto movedCard = static_cast<AbstractCardItem*>(mouseGrabberItem())) {
        std::function<void(AbstractCardItem*)> addCardWithChildrenToMoveList;
        addCardWithChildrenToMoveList
            = [this, &addCardWithChildrenToMoveList](AbstractCardItem* _card) {
                  d->mouseGrabberItems.insert(_card);

                  const auto children = _card->embeddedCards();
                  for (auto child : children) {
                      addCardWithChildrenToMoveList(child);
                  }
              };
        addCardWithChildrenToMoveList(movedCard);

        if (_event->button() == Qt::RightButton) {
            emit itemContextMenuRequested(movedCard->modelItemIndex());
        }
    }
}

void CardsGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* _event)
{
    if (d->isReadOnly) {
        return;
    }

    QGraphicsScene::mouseMoveEvent(_event);
}

void CardsGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* _event)
{
    QGraphicsScene::mouseReleaseEvent(_event);

    if (d->isReadOnly) {
        return;
    }

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
