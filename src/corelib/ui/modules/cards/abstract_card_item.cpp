#include "abstract_card_item.h"

#include "cards_graphics_scene.h"

#include <QGraphicsScene>
#include <QModelIndex>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QVariantAnimation>


namespace Ui {

class AbstractCardItem::Implementation
{
public:
    explicit Implementation(AbstractCardItem* _q);

    /**
     * @brief Обновить положения вложенных карточек
     */
    void updateEmbeddedCardsPositions();

    /**
     * @brief Получить указатель на сцену
     * @note Используется для испускания сигналов через сцену
     */
    CardsGraphicsScene* cardsScene() const;


    AbstractCardItem* q = nullptr;

    /**
     * @brief Индекс элемента модели
     */
    QModelIndex modelItemIndex;

    /**
     * @brief Готова ли данная карточка к вложению другой
     */
    bool isReadyForEmbed = false;

    /**
     * @brief Контейнер, в который карточка должна быть вложена при отпускании
     */
    AbstractCardItem* container = nullptr;

    /**
     * @brief Позиции карточек относительно контейнера и собственно карточки
     */
    QHash<AbstractCardItem*, QPointF> embeddedCards;
};

AbstractCardItem::Implementation::Implementation(AbstractCardItem* _q)
    : q(_q)
{
}

void AbstractCardItem::Implementation::updateEmbeddedCardsPositions()
{
    for (auto iter = embeddedCards.begin(); iter != embeddedCards.end(); ++iter) {
        if (!iter.key()->isVisible()) {
            continue;
        }

        iter.key()->d->updateEmbeddedCardsPositions();
        iter.value() = iter.key()->pos() - q->pos();
    }
}

CardsGraphicsScene* AbstractCardItem::Implementation::cardsScene() const
{
    auto cardsScene = qobject_cast<CardsGraphicsScene*>(q->scene());
    Q_ASSERT(cardsScene);
    return cardsScene;
}


// ****


AbstractCardItem::AbstractCardItem(QGraphicsItem* _parent)
    : QGraphicsRectItem(_parent)
    , d(new Implementation(this))
{
}

AbstractCardItem::~AbstractCardItem() = default;

QModelIndex AbstractCardItem::modelItemIndex() const
{
    return d->modelItemIndex;
}

void AbstractCardItem::setModelItemIndex(const QModelIndex& _index)
{
    if (d->modelItemIndex == _index) {
        return;
    }

    d->modelItemIndex = _index;
    init();
    prepareGeometryChange();
    update();
}

qreal AbstractCardItem::headerHeight() const
{
    return 0;
}

bool AbstractCardItem::isContainer() const
{
    return false;
}

bool AbstractCardItem::isContainerOf(AbstractCardItem* _card) const
{
    if (!isContainer()) {
        return false;
    }

    if (_card->container() == nullptr) {
        return false;
    }

    if (d->embeddedCards.contains(_card)) {
        return true;
    }

    AbstractCardItem* topLevelContainer = _card->container();
    while (topLevelContainer != nullptr) {
        if (topLevelContainer == this) {
            return true;
        }

        topLevelContainer = topLevelContainer->container();
    }
    return false;
}

bool AbstractCardItem::isOpened() const
{
    return false;
}

bool AbstractCardItem::canBeEmbedded() const
{
    return true;
}

AbstractCardItem* AbstractCardItem::container() const
{
    return d->container;
}

void AbstractCardItem::setContainer(AbstractCardItem* _container)
{
    if (d->container == _container) {
        return;
    }

    //
    // Если карточка должна была быть вложена в другой контейнер, то сбрасываем ему флаг вложения
    //
    if (d->container != nullptr) {
        d->container->setReadyForEmbed(false);
        d->container->takeChild(this);
    }

    //
    // Обновим контейнер
    //
    d->container = _container;

    //
    // Если новый контейнер задан, установим ему флаг вложения
    //
    if (d->container != nullptr) {
        d->container->setReadyForEmbed(this == scene()->mouseGrabberItem());
        d->container->appendChild(this);
    }
}

void AbstractCardItem::appendChild(AbstractCardItem* _child)
{
    if (d->embeddedCards.contains(_child)) {
        return;
    }
    d->embeddedCards[_child] = _child->pos() - pos();
}

void AbstractCardItem::takeChild(AbstractCardItem* _child)
{
    d->embeddedCards.remove(_child);
}

QList<AbstractCardItem*> AbstractCardItem::children() const
{
    return d->embeddedCards.keys();
}

int AbstractCardItem::childCount() const
{
    return d->embeddedCards.size();
}

bool AbstractCardItem::isFilterAccepted(const QString& _text, bool _caseSensitive,
                                        int _filterType) const
{
    Q_UNUSED(_text)
    Q_UNUSED(_caseSensitive)
    Q_UNUSED(_filterType)

    return false;
}

void AbstractCardItem::init()
{
}

bool AbstractCardItem::isReadyForEmbed() const
{
    return d->isReadyForEmbed;
}

void AbstractCardItem::setReadyForEmbed(bool _isReady)
{
    if (!isContainer()) {
        return;
    }

    if (d->isReadyForEmbed == _isReady) {
        return;
    }

    d->isReadyForEmbed = _isReady;
    update();
}

void AbstractCardItem::mousePressEvent(QGraphicsSceneMouseEvent* _event)
{
    QGraphicsRectItem::mousePressEvent(_event);

    //
    // Обновляем положения, т.к. из-за анимаций или изенения состояния папок они могли измениться
    //
    d->updateEmbeddedCardsPositions();
}

void AbstractCardItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* _event)
{
    QGraphicsRectItem::mouseReleaseEvent(_event);

    //
    // Сбрасываем флаг вставки в контейнер, если он задан
    //
    if (d->container != nullptr) {
        d->container->setReadyForEmbed(false);
    }

    //
    // Если задан контейнер и он закрыт, то проигрываем анимацию вложения карточки внутрь контейнера
    //
    if (d->container != nullptr && !d->container->isOpened()) {
        constexpr int duration = 200;
        auto scaleAnimation = new QVariantAnimation;
        scaleAnimation->setDuration(duration);
        scaleAnimation->setEasingCurve(QEasingCurve::OutQuart);
        scaleAnimation->setStartValue(scale());
        scaleAnimation->setEndValue((d->container->boundingRect().width() * 0.8)
                                    / boundingRect().width());
        QObject::connect(scaleAnimation, &QVariantAnimation::valueChanged, scaleAnimation,
                         [this](const QVariant& _value) { setScale(_value.toReal()); });
        auto moveToTopAnimation = new QVariantAnimation;
        moveToTopAnimation->setDuration(duration);
        moveToTopAnimation->setEasingCurve(QEasingCurve::OutQuart);
        moveToTopAnimation->setStartValue(pos());
        moveToTopAnimation->setEndValue(
            d->container->pos()
            - QPointF(-0.1 * d->container->boundingRect().width(),
                      boundingRect().height()
                          - ((1 - scaleAnimation->endValue().toReal()) / 2.0)
                              * boundingRect().height()));
        QObject::connect(moveToTopAnimation, &QVariantAnimation::valueChanged, moveToTopAnimation,
                         [this](const QVariant& _value) { setPos(_value.toPointF()); });
        auto zAnimation = new QVariantAnimation;
        zAnimation->setDuration(1);
        zAnimation->setStartValue(d->container->zValue());
        zAnimation->setEndValue(d->container->zValue() - 0.001);
        QObject::connect(zAnimation, &QVariantAnimation::valueChanged, zAnimation,
                         [this](const QVariant& _value) { setZValue(_value.toReal()); });
        auto moveToBottomAnimation = new QVariantAnimation;
        moveToBottomAnimation->setDuration(duration);
        moveToBottomAnimation->setEasingCurve(QEasingCurve::InQuad);
        moveToBottomAnimation->setStartValue(moveToTopAnimation->endValue().toPointF());
        moveToBottomAnimation->setEndValue(d->container->pos()
                                           + QPointF(0.1 * d->container->boundingRect().width(),
                                                     0.1 * d->container->boundingRect().height()));
        QObject::connect(moveToBottomAnimation, &QVariantAnimation::valueChanged,
                         moveToBottomAnimation,
                         [this](const QVariant& _value) { setPos(_value.toPointF()); });

        auto moveToTopGroup = new QParallelAnimationGroup;
        moveToTopGroup->addAnimation(scaleAnimation);
        moveToTopGroup->addAnimation(moveToTopAnimation);
        auto group = new QSequentialAnimationGroup;
        group->addPause(100);
        group->addAnimation(moveToTopGroup);
        group->addAnimation(zAnimation);
        group->addAnimation(moveToBottomAnimation);
        QObject::connect(group, &QSequentialAnimationGroup::finished, group, [this] {
            setVisible(false);
            setScale(1.0);
        });
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    //
    // Возвращаем карточку на место
    //
    // Делаем отложенный вызов, чтобы mouseGrabber сцены освободился
    //
    QMetaObject::invokeMethod(
        scene(), [this] { emit d->cardsScene()->itemChanged(d->modelItemIndex); },
        Qt::QueuedConnection);
}

QVariant AbstractCardItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case QGraphicsItem::ItemScenePositionHasChanged: {
        //        for (auto iter = d->embeddedCards.begin(); iter != d->embeddedCards.end(); ++iter)
        //        {
        //            auto item = iter.key();
        //            if (item == scene()->mouseGrabberItem()) {
        //                continue;
        //            }

        //            auto itemPosDelta = iter.value();
        //            item->setPos(pos() + itemPosDelta);

        //            auto container = d->container;
        //            while (container != nullptr) {
        //                container->update();
        //                container = container->container();
        //            }
        //        }

        emit d->cardsScene()->itemChanged(d->modelItemIndex);

        break;
    }

    default: {
        break;
    }
    }

    return QGraphicsRectItem::itemChange(change, value);
}

} // namespace Ui
