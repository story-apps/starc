#include "projects_cards.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/image_helper.h>

#include <QGraphicsRectItem>
#include <QResizeEvent>
#include <QVariantAnimation>


namespace Ui
{

namespace {

/**
 * @brief Карточка проекта
 */
class ProjectCard : public QGraphicsRectItem
{
public:
    explicit ProjectCard(QGraphicsItem* _parent = nullptr);

    /**
     * @brief Переопределяем метод, чтобы работал qgraphicsitem_cast
     */
    int type() const override;

    /**
     * @brief Отрисовка карточки
     */
    void paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option, QWidget* _widget) override;

    /**
     * @brief Анимируем hover
     */
    void hoverEnterEvent(QGraphicsSceneHoverEvent* _event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* _event) override;

private:
    /**
     * @brief  Декорации тени при наведении
     */
    QVariantAnimation shadowHeightAnimation;
};

ProjectCard::ProjectCard(QGraphicsItem* _parent)
    : QGraphicsRectItem(_parent)
{
    setRect(0, 0, 200, 120);
    setAcceptHoverEvents(true);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    shadowHeightAnimation.setStartValue(Ui::DesignSystem::card().minimumShadowBlurRadius());
    shadowHeightAnimation.setEndValue(Ui::DesignSystem::card().maximumShadowBlurRadius());
    shadowHeightAnimation.setEasingCurve(QEasingCurve::OutQuad);
    shadowHeightAnimation.setDuration(160);
    QObject::connect(&shadowHeightAnimation, &QVariantAnimation::valueChanged, [this] { update(); });
}

int ProjectCard::type() const
{
    return QGraphicsItem::UserType;
}

void ProjectCard::paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option, QWidget* _widget)
{
    Q_UNUSED(_option);
    Q_UNUSED(_widget);

    const QRectF backgroundRect = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins());
    if (!backgroundRect.isValid()) {
        return;
    }

    //
    // Заливаем фон
    //
    QPixmap backgroundImage(backgroundRect.size().toSize());
    backgroundImage.fill(Qt::transparent);
    QPainter backgroundImagePainter(&backgroundImage);
    backgroundImagePainter.setPen(Qt::NoPen);
    backgroundImagePainter.setBrush(Ui::DesignSystem::color().background());
    const qreal borderRadius = Ui::DesignSystem::card().borderRadius();
    backgroundImagePainter.drawRoundedRect(QRect({0,0}, backgroundImage.size()), borderRadius, borderRadius);
    //
    // ... рисуем тень
    //
    const qreal shadowHeight = std::max(Ui::DesignSystem::floatingToolBar().minimumShadowBlurRadius(),
                                        shadowHeightAnimation.currentValue().toReal());
    const QPixmap shadow
            = ImageHelper::dropShadow(backgroundImage,
                                      Ui::DesignSystem::floatingToolBar().shadowMargins(),
                                      shadowHeight,
                                      Ui::DesignSystem::color().shadow());
    _painter->drawPixmap(0, 0, shadow);
    //
    // ... рисуем сам фон
    //
    _painter->setPen(Qt::NoPen);
    _painter->setBrush(Ui::DesignSystem::color().background());
    _painter->drawRoundedRect(backgroundRect, borderRadius, borderRadius);
    _painter->drawPixmap(backgroundRect.topLeft(), backgroundImage);
}

void ProjectCard::hoverEnterEvent(QGraphicsSceneHoverEvent* _event)
{
    QGraphicsRectItem::hoverEnterEvent(_event);
    shadowHeightAnimation.setDirection(QVariantAnimation::Forward);
    shadowHeightAnimation.start();
}

void ProjectCard::hoverLeaveEvent(QGraphicsSceneHoverEvent* _event)
{
    QGraphicsRectItem::hoverLeaveEvent(_event);
    shadowHeightAnimation.setDirection(QVariantAnimation::Backward);
    shadowHeightAnimation.start();
}

} // namespace

class ProjectsCards::Implementation
{
public:
    explicit Implementation(QGraphicsView* _parent);

    QGraphicsScene* scene = nullptr;
};

ProjectsCards::Implementation::Implementation(QGraphicsView* _parent)
    : scene(new QGraphicsScene(_parent))
{
}


// ****


ProjectsCards::ProjectsCards(QWidget* _parent)
    : QGraphicsView(_parent),
      d(new Implementation(this))
{
    setFrameShape(QFrame::NoFrame);
    setScene(d->scene);

    d->scene->addItem(new ProjectCard);
}

ProjectsCards::~ProjectsCards() = default;

void ProjectsCards::resizeEvent(QResizeEvent* _event)
{
    QGraphicsView::resizeEvent(_event);
}

}
