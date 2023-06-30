#include "project_card.h"

#include "project_team_card.h"

#include <domain/starcloud_api.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <management_layer/content/projects/projects_model.h>
#include <management_layer/content/projects/projects_model_project_item.h>
#include <management_layer/content/projects/projects_model_team_item.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QVariantAnimation>


namespace Ui {

class ProjectCard::Implementation
{
public:
    explicit Implementation(ProjectCard* _q);


    /**
     * @brief Владелец
     */
    ProjectCard* q = nullptr;

    /**
     * @brief Элемент с данными о проекте
     */
    BusinessLayer::ProjectsModelProjectItem* project = nullptr;

    /**
     * @brief  Декорации тени при наведении
     */
    QVariantAnimation shadowHeightAnimation;

    /**
     * @brief  Декорации при клике
     */
    ClickAnimation decorationAnimation;
};

ProjectCard::Implementation::Implementation(ProjectCard* _q)
    : q(_q)
{
    decorationAnimation.setFast(false);
}


// ****


ProjectCard::ProjectCard(QGraphicsItem* _parent)
    : AbstractCardItem(_parent)
    , d(new Implementation(this))
{
    setRect(QRectF({ 0, 0 }, DesignSystem::projectCard().size()));
    setAcceptHoverEvents(true);

    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    d->shadowHeightAnimation.setStartValue(DesignSystem::card().minimumShadowBlurRadius());
    d->shadowHeightAnimation.setEndValue(DesignSystem::card().maximumShadowBlurRadius());
    d->shadowHeightAnimation.setEasingCurve(QEasingCurve::OutQuad);
    d->shadowHeightAnimation.setDuration(160);
    QObject::connect(&d->shadowHeightAnimation, &QVariantAnimation::valueChanged,
                     &d->shadowHeightAnimation, [this] { update(); });
    QObject::connect(&d->decorationAnimation, &ClickAnimation::valueChanged,
                     &d->decorationAnimation, [this] { update(); });
}

ProjectCard::~ProjectCard()
{
    d->shadowHeightAnimation.disconnect();
    d->shadowHeightAnimation.stop();
}

int ProjectCard::type() const
{
    return Type;
}

BusinessLayer::ProjectsModelProjectItem* ProjectCard::project() const
{
    return d->project;
}

bool ProjectCard::canBePlacedAfter(AbstractCardItem* _previousCard)
{
    //
    // Если проект в коменде, то его нельзя вытащить на самый верх
    //
    if (_previousCard == nullptr && d->project->teamId() != Domain::kInvalidId) {
        return false;
    }

    if (const auto projectCard = qgraphicsitem_cast<ProjectCard*>(_previousCard);
        projectCard != nullptr) {
        //
        // Проект нельзя разместить после проекта из другой команды (внутри другой команды)
        //
        if (projectCard->project()->teamId() != d->project->teamId()) {
            return false;
        }
    } else {
        //
        // Проект нельзя разместить после команды (в руте), если он уже внутри какой-либо команды
        //
        if (d->project->teamId() != Domain::kInvalidId) {
            return false;
        }
    }

    return true;
}

bool ProjectCard::canBeEmbedded(AbstractCardItem* _container) const
{
    //
    // Проект можно перемещать только в рамках его команды
    //
    const auto teamCard = qgraphicsitem_cast<ProjectTeamCard*>(_container);
    Q_ASSERT(teamCard);
    if (teamCard->teamItem()->id() == d->project->teamId()) {
        return true;
    }

    return false;
}

void ProjectCard::paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option,
                        QWidget* _widget)
{
    Q_UNUSED(_widget)

    const bool isLeftToRight = _option->direction == Qt::LeftToRight;
    const QRectF backgroundRect = rect().marginsRemoved(DesignSystem::card().shadowMargins());
    if (!backgroundRect.isValid()) {
        return;
    }

    //
    // Готовим фон (кэшируем его, чтобы использовать во всех карточках)
    //
    static QPixmap backgroundPixmapCache;
    static QColor backgroundPixmapColor;
    if (backgroundPixmapCache.size() != backgroundRect.size()
        || backgroundPixmapColor != DesignSystem::color().background()) {
        backgroundPixmapCache = QPixmap(backgroundRect.size().toSize());
        backgroundPixmapCache.fill(Qt::transparent);
        QPainter backgroundImagePainter(&backgroundPixmapCache);
        backgroundImagePainter.setRenderHint(QPainter::Antialiasing);
        backgroundImagePainter.setPen(Qt::NoPen);
        backgroundPixmapColor = DesignSystem::color().background();
        backgroundImagePainter.setBrush(backgroundPixmapColor);
        const qreal borderRadius = DesignSystem::card().borderRadius();
        backgroundImagePainter.drawRoundedRect(QRect({ 0, 0 }, backgroundPixmapCache.size()),
                                               borderRadius, borderRadius);
    }
    //
    // ... рисуем тень
    //
    const qreal shadowHeight = std::max(DesignSystem::floatingToolBar().minimumShadowBlurRadius(),
                                        d->shadowHeightAnimation.currentValue().toReal());
    const QPixmap shadow = ImageHelper::dropShadow(backgroundPixmapCache,
                                                   DesignSystem::floatingToolBar().shadowMargins(),
                                                   shadowHeight, DesignSystem::color().shadow());
    _painter->drawPixmap(0, 0, shadow);
    //
    // ... рисуем сам фон
    //
    _painter->drawPixmap(backgroundRect, backgroundPixmapCache, backgroundPixmapCache.rect());

    //
    // Постер
    //
    const QSizeF posterSize
        = d->project->poster().size().scaled(backgroundRect.size().toSize(), Qt::KeepAspectRatio);
    const QRectF posterRect(isLeftToRight
                                ? backgroundRect.topLeft()
                                : backgroundRect.topRight() - QPointF(posterSize.width(), 0),
                            posterSize);
    const auto scaledPoster = d->project->poster().scaled(posterSize.toSize(), Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation);
    _painter->drawPixmap(posterRect, scaledPoster, scaledPoster.rect());

    //
    // Заголовок
    //
    _painter->setPen(DesignSystem::color().onBackground());
    _painter->setFont(DesignSystem::font().h6());
    const QRectF textRect(isLeftToRight ? posterRect.right() + DesignSystem::layout().px16()
                                        : backgroundRect.left() + DesignSystem::layout().px12(),
                          backgroundRect.top() + DesignSystem::layout().px8(),
                          backgroundRect.width() - posterRect.width()
                              - DesignSystem::layout().px12() * 2,
                          TextHelper::fineLineSpacing(_painter->font()));
    _painter->drawText(
        textRect, Qt::AlignLeft | Qt::AlignVCenter,
        TextHelper::elidedText(d->project->name(), _painter->font(), textRect.width()));

    //
    // Логлайн
    //
    _painter->setPen(DesignSystem::color().onBackground());
    _painter->setFont(DesignSystem::font().body2());
    const auto fullLoglinHeight
        = TextHelper::heightForWidth(d->project->logline(), _painter->font(), textRect.width());
    const auto fontLineSpacing = TextHelper::fineLineSpacing(_painter->font());
    const int loglineMaxLines = d->project->isLocal() ? 5 : 4;
    const QRectF loglineRect(textRect.left(), textRect.bottom() + DesignSystem::layout().px8(),
                             textRect.width(),
                             std::min(fullLoglinHeight, fontLineSpacing * loglineMaxLines));
    const QString loglineCorrected
        = TextHelper::elidedText(d->project->logline(), _painter->font(), loglineRect);
    _painter->drawText(loglineRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                       loglineCorrected);

    //
    // Дата последнего изменения
    //
    const QColor betweenBackgroundColor = ColorHelper::colorBetween(
        DesignSystem::color().onBackground(), DesignSystem::color().background());
    _painter->setPen(betweenBackgroundColor);
    const auto lastDateTop = loglineCorrected.isEmpty()
        ? loglineRect.top()
        : loglineRect.bottom() + DesignSystem::layout().px4();
    const QRectF lastDateRect(loglineRect.left(), lastDateTop, loglineRect.width(),
                              fontLineSpacing);
    _painter->drawText(lastDateRect, Qt::AlignLeft | Qt::AlignTop,
                       d->project->displayLastEditTime());

    //
    // Роль в проекте
    //
    if (d->project->isRemote()) {
        if (d->project->isOwner()) {
            _painter->setFont(DesignSystem::font().subtitle2());
            const QRectF roleRect(
                lastDateRect.bottomLeft(),
                QPointF(lastDateRect.right(),
                        lastDateRect.bottom() + fontLineSpacing + DesignSystem::layout().px(6)));
            _painter->drawText(roleRect, Qt::AlignLeft | Qt::AlignBottom,
                               QCoreApplication::translate("ProjectCard", "Owner"));
        } else {
            QString roleIcon;
            QString roleText;
            switch (d->project->editingMode()) {
            case ManagementLayer::DocumentEditingMode::Edit: {
                roleIcon = u8"\U000F03EB";
                roleText = QCoreApplication::translate("ProjectCard", "Editor");
                break;
            }

            case ManagementLayer::DocumentEditingMode::Comment: {
                roleIcon = u8"\U000F0184";
                roleText = QCoreApplication::translate("ProjectCard", "Commentator");
                break;
            }

            case ManagementLayer::DocumentEditingMode::Read: {
                roleIcon = u8"\U000F0208";
                roleText = QCoreApplication::translate("ProjectCard", "Reader");
                break;
            }

            case ManagementLayer::DocumentEditingMode::Mixed: {
                roleIcon = u8"\U000F11BF";
                roleText = QCoreApplication::translate("ProjectCard", "Partial access");
                break;
            }
            }

            _painter->setFont(DesignSystem::font().iconsSmall());
            const QRectF iconRect(lastDateRect.bottomLeft(),
                                  QSizeF(DesignSystem::layout().px24(),
                                         fontLineSpacing + DesignSystem::layout().px4()));
            _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignBottom, roleIcon);


            _painter->setFont(DesignSystem::font().subtitle2());
            const QRectF roleRect(
                iconRect.topRight(),
                QPointF(lastDateRect.right(),
                        lastDateRect.bottom() + fontLineSpacing + DesignSystem::layout().px(6)));
            _painter->drawText(roleRect, Qt::AlignLeft | Qt::AlignBottom, roleText);
        }
    }

    //
    // Иконка расположения проекта
    //
    _painter->setPen(DesignSystem::color().onBackground());
    _painter->setFont(DesignSystem::font().iconsMid());
    const QRectF iconRect(isLeftToRight ? backgroundRect.right() - DesignSystem::layout().px24() * 2
                                        : backgroundRect.left(),
                          backgroundRect.bottom() - DesignSystem::layout().px24() * 2,
                          DesignSystem::layout().px24() * 2, DesignSystem::layout().px24() * 2);
    _painter->drawText(iconRect, Qt::AlignCenter,
                       d->project->projectType() == BusinessLayer::ProjectType::Cloud
                           ? u8"\U000F0163"
                           : u8"\U000F0322");

    //
    // Декорация
    //
    if (d->decorationAnimation.state() == ClickAnimation::Running) {
        _painter->setPen(Qt::NoPen);
        _painter->setBrush(Ui::DesignSystem::color().accent());
        _painter->setClipRect(backgroundRect);
        _painter->setOpacity(d->decorationAnimation.opacity());
        const auto radius = d->decorationAnimation.radius();
        _painter->drawEllipse(d->decorationAnimation.clickPosition(), radius, radius);
        _painter->setClipRect(QRectF(), Qt::NoClip);
        _painter->setOpacity(1.0);
    }
}

void ProjectCard::init()
{
    auto model = qobject_cast<const BusinessLayer::ProjectsModel*>(modelItemIndex().model());
    Q_ASSERT(model);
    auto projectItem = model->itemForIndex(modelItemIndex());
    Q_ASSERT(projectItem);
    Q_ASSERT(projectItem->type() == BusinessLayer::ProjectsModelItemType::Project);
    d->project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(projectItem);
    Q_ASSERT(d->project);

    //
    // Если название не влезает, то установим его тултипом
    //
    const QRectF backgroundRect = rect().marginsRemoved(DesignSystem::card().shadowMargins());
    const QSizeF posterSize
        = d->project->poster().size().scaled(backgroundRect.size().toSize(), Qt::KeepAspectRatio);
    const auto textWidth
        = backgroundRect.width() - posterSize.width() - DesignSystem::layout().px12() * 2;
    if (TextHelper::fineTextWidthF(d->project->name(), DesignSystem::font().h6()) > textWidth) {
        setToolTip(d->project->name());
    } else {
        setToolTip({});
    }
}

void ProjectCard::hoverEnterEvent(QGraphicsSceneHoverEvent* _event)
{
    AbstractCardItem::hoverEnterEvent(_event);
    d->shadowHeightAnimation.setDirection(QVariantAnimation::Forward);
    d->shadowHeightAnimation.start();
}

void ProjectCard::hoverLeaveEvent(QGraphicsSceneHoverEvent* _event)
{
    AbstractCardItem::hoverLeaveEvent(_event);
    d->shadowHeightAnimation.setDirection(QVariantAnimation::Backward);
    d->shadowHeightAnimation.start();
}

void ProjectCard::mousePressEvent(QGraphicsSceneMouseEvent* _event)
{
    if (!boundingRect().contains(_event->pos())) {
        if (d->shadowHeightAnimation.direction() == QVariantAnimation::Forward) {
            d->shadowHeightAnimation.setDirection(QVariantAnimation::Backward);
            d->shadowHeightAnimation.start();
        }
        return;
    }

    AbstractCardItem::mousePressEvent(_event);

    const auto backgroundRect = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins());
    if (!backgroundRect.contains(_event->pos())) {
        return;
    }

    d->decorationAnimation.setClickPosition(_event->pos());
    d->decorationAnimation.setRadiusInterval(
        0.0, std::max(backgroundRect.width(), backgroundRect.height()));
    d->decorationAnimation.start();
}

} // namespace Ui
