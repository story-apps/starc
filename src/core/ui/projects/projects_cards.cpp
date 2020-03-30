#include "projects_cards.h"

#include <include/custom_events.h>

#include <management_layer/content/projects/project.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>

#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <QPointer>
#include <QResizeEvent>
#include <QTimer>
#include <QtMath>
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
    ~ProjectCard() override;

    /**
     * @brief Задать проект для отображения на карточке
     */
    void setProject(const ManagementLayer::Project& _project);

    /**
     * @brief Получить проект, который отображается на карточке
     */
    ManagementLayer::Project project() const;

    /**
     * @brief Отрисовка карточки
     */
    void paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option, QWidget* _widget) override;

    /**
     * @brief Анимируем hover
     */
    void hoverEnterEvent(QGraphicsSceneHoverEvent* _event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* _event) override;

    /**
     * @brief Реализуем клик на объекте
     */
    void mousePressEvent(QGraphicsSceneMouseEvent* _event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* _event) override;

private:
    /**
     * @brief Получить указатель на сцену
     * @note Используется для испускания сигналов через сцену
     */
    ProjectsScene* projectsScene() const;

    /**
     * @brief Получить следующее z-значение для позиционирования карточки
     */
    qreal nextZValue() const;

    /**
     * @brief Получить области действий карточки
     * @note В списке они идут от последнего к первому, так проще формировать
     */
    QVector<QRectF> actionsRects() const;

    /**
     * @brief Проект для отображения на карточке
     */
    ManagementLayer::Project m_project;

    /**
     * @brief  Декорации тени при наведении
     */
    QVariantAnimation m_shadowHeightAnimation;

    /**
     * @brief  Декорации иконок действий при наведении
     */
    QVariantAnimation m_actionsOpacityAnimation;

    /**
     * @brief Декорации при клике
     */
    QPointF m_decorationCenterPosition;
    QVariantAnimation m_decorationRadiusAnimation;
    QVariantAnimation m_decorationOpacityAnimation;
};

ProjectCard::ProjectCard(QGraphicsItem* _parent)
    : QGraphicsRectItem(_parent)
{
    setRect(QRectF({0, 0}, Ui::DesignSystem::projectCard().size()));
    setAcceptHoverEvents(true);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_shadowHeightAnimation.setStartValue(Ui::DesignSystem::card().minimumShadowBlurRadius());
    m_shadowHeightAnimation.setEndValue(Ui::DesignSystem::card().maximumShadowBlurRadius());
    m_shadowHeightAnimation.setEasingCurve(QEasingCurve::OutQuad);
    m_shadowHeightAnimation.setDuration(160);
    QObject::connect(&m_shadowHeightAnimation, &QVariantAnimation::valueChanged, [this] { update(); });

    m_actionsOpacityAnimation.setStartValue(0.0);
    m_actionsOpacityAnimation.setEndValue(1.0);
    m_actionsOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    m_actionsOpacityAnimation.setDuration(220);
    QObject::connect(&m_actionsOpacityAnimation, &QVariantAnimation::valueChanged, [this] { update(); });

    m_decorationRadiusAnimation.setDuration(240);
    QObject::connect(&m_decorationRadiusAnimation, &QVariantAnimation::valueChanged, [this] { update(); });

    m_decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    m_decorationOpacityAnimation.setDuration(240);
    QObject::connect(&m_decorationOpacityAnimation, &QVariantAnimation::valueChanged, [this] { update(); });
}

ProjectCard::~ProjectCard()
{
    m_shadowHeightAnimation.disconnect();
    m_shadowHeightAnimation.stop();
    m_actionsOpacityAnimation.disconnect();
    m_actionsOpacityAnimation.stop();
    m_decorationRadiusAnimation.disconnect();
    m_decorationRadiusAnimation.stop();
    m_decorationOpacityAnimation.disconnect();
    m_decorationOpacityAnimation.stop();
}

void ProjectCard::setProject(const ManagementLayer::Project& _project)
{
    if (m_project == _project) {
        return;
    }

    m_project = _project;
    update();
}

ManagementLayer::Project ProjectCard::project() const
{
    return m_project;
}

void ProjectCard::paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option, QWidget* _widget)
{
    Q_UNUSED(_option)
    Q_UNUSED(_widget)

    const QRectF backgroundRect = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins());
    if (!backgroundRect.isValid()) {
        return;
    }

    //
    // Готовим фон (кэшируем его, чтобы использовать во всех карточках)
    //
    static QPixmap backgroundPixmapCache;
    static QColor backgroundPixmapColor;
    if (backgroundPixmapCache.size() != backgroundRect.size()
        || backgroundPixmapColor != Ui::DesignSystem::color().background()) {
        backgroundPixmapCache = QPixmap(backgroundRect.size().toSize());
        backgroundPixmapCache.fill(Qt::transparent);
        QPainter backgroundImagePainter(&backgroundPixmapCache);
        backgroundImagePainter.setRenderHint(QPainter::Antialiasing);
        backgroundImagePainter.setPen(Qt::NoPen);
        backgroundPixmapColor = Ui::DesignSystem::color().background();
        backgroundImagePainter.setBrush(backgroundPixmapColor);
        const qreal borderRadius = Ui::DesignSystem::card().borderRadius();
        backgroundImagePainter.drawRoundedRect(QRect({0,0}, backgroundPixmapCache.size()), borderRadius, borderRadius);
    }
    //
    // ... рисуем тень
    //
    const qreal shadowHeight = std::max(Ui::DesignSystem::floatingToolBar().minimumShadowBlurRadius(),
                                        m_shadowHeightAnimation.currentValue().toReal());
    const QPixmap shadow
            = ImageHelper::dropShadow(backgroundPixmapCache,
                                      Ui::DesignSystem::floatingToolBar().shadowMargins(),
                                      shadowHeight,
                                      Ui::DesignSystem::color().shadow());
    _painter->drawPixmap(0, 0, shadow);
    //
    // ... рисуем сам фон
    //
    _painter->drawPixmap(backgroundRect, backgroundPixmapCache, backgroundPixmapCache.rect());

    //
    // Постер
    //
    const QPixmap& poster = m_project.poster();
    const QRectF posterRect(backgroundRect.topLeft(),
                            poster.size().scaled(backgroundRect.size().toSize(), Qt::KeepAspectRatio));
    _painter->drawPixmap(posterRect, poster, poster.rect());

    //
    // Заголовок
    //
    _painter->setPen(Ui::DesignSystem::color().onBackground());
    _painter->setFont(Ui::DesignSystem::font().h6());
    const QFontMetricsF textFontMetrics(Ui::DesignSystem::font().h6());
    const QRectF textRect(posterRect.right() + Ui::DesignSystem::layout().px16(),
                          backgroundRect.top() + Ui::DesignSystem::layout().px8(),
                          backgroundRect.width() - posterRect.width() - Ui::DesignSystem::layout().px12() * 2,
                          textFontMetrics.lineSpacing());
    _painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, textFontMetrics.elidedText(m_project.name(), Qt::ElideRight, textRect.width()));

    //
    // Путь
    //
    const QColor betweenBackgroundColor = ColorHelper::colorBetween(Ui::DesignSystem::color().onBackground(),
                                                                    Ui::DesignSystem::color().background());
    _painter->setPen(betweenBackgroundColor);
    _painter->setFont(Ui::DesignSystem::font().body2());
    const QFontMetricsF fontMetrics(_painter->font());
    const QRectF pathRect(textRect.left(), textRect.bottom() + Ui::DesignSystem::layout().px4(),
                          textRect.width(), fontMetrics.lineSpacing());
    _painter->drawText(pathRect, Qt::AlignLeft | Qt::AlignVCenter, fontMetrics.elidedText(m_project.path(), Qt::ElideLeft, pathRect.width()));

    //
    // Логлайн
    //
    _painter->setPen(Ui::DesignSystem::color().onBackground());
    const QRectF loglineRect(pathRect.left(), pathRect.bottom() + Ui::DesignSystem::layout().px4(),
                             pathRect.width(), fontMetrics.lineSpacing() * 5);
    _painter->drawText(loglineRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, m_project.logline());

    //
    // Нижняя строка формируется в зависимости от того, наведена ли мышь на проект
    // и разруливается это всё за счёт прозрачности
    //

    //
    // Дата последнего изменения
    //
    _painter->setOpacity(1.0 - m_actionsOpacityAnimation.currentValue().toReal());
    _painter->setPen(betweenBackgroundColor);
    const qreal lastDateHeight = fontMetrics.lineSpacing() + Ui::DesignSystem::layout().px8() * 2;
    const QRectF lastDateRect(loglineRect.left(), backgroundRect.bottom() - lastDateHeight,
                             pathRect.width(), lastDateHeight);
    _painter->drawText(lastDateRect, Qt::AlignLeft | Qt::AlignTop, m_project.displayLastEditTime());

    //
    // Иконка расположения проекта
    //
    _painter->setOpacity(1.0);
    _painter->setPen(Ui::DesignSystem::color().onBackground());
    _painter->setFont(Ui::DesignSystem::font().iconsMid());
    const QRectF iconRect(backgroundRect.right() - Ui::DesignSystem::layout().px24() * 2,
                          backgroundRect.bottom() - Ui::DesignSystem::layout().px24() * 2,
                          Ui::DesignSystem::layout().px24() * 2,
                          Ui::DesignSystem::layout().px24() * 2);
    _painter->drawText(iconRect, Qt::AlignCenter,
                       m_project.type() == ManagementLayer::ProjectType::Local ? u8"\uf379" : u8"\uf163");

    //
    // Иконки действий
    //
    _painter->setOpacity(m_actionsOpacityAnimation.currentValue().toReal());
    const auto iconsRects = actionsRects();
    if (m_project.type() == ManagementLayer::ProjectType::Local) {
        _painter->drawText(iconsRects[0], Qt::AlignCenter, u8"\uf167"); // move to cloud
        _painter->drawText(iconsRects[1], Qt::AlignCenter, u8"\uf6d0"); // hide
    } else {
        _painter->drawText(iconsRects[0], Qt::AlignCenter, u8"\uf3eb"); // pencil
        _painter->drawText(iconsRects[1], Qt::AlignCenter, u8"\uf1c0"); // delete
    }

    //
    // Декорация
    //
    if (m_decorationRadiusAnimation.state() == QVariantAnimation::Running
        || m_decorationOpacityAnimation.state() == QVariantAnimation::Running
        || QApplication::mouseButtons().testFlag(Qt::LeftButton)) {
        _painter->setClipRect(backgroundRect);
        _painter->setPen(Qt::NoPen);
        _painter->setBrush(Ui::DesignSystem::color().secondary());
        _painter->setOpacity(m_decorationOpacityAnimation.currentValue().toReal());
        _painter->drawEllipse(m_decorationCenterPosition, m_decorationRadiusAnimation.currentValue().toReal(),
                            m_decorationRadiusAnimation.currentValue().toReal());
    }
}

void ProjectCard::hoverEnterEvent(QGraphicsSceneHoverEvent* _event)
{
    QGraphicsRectItem::hoverEnterEvent(_event);
    m_shadowHeightAnimation.setDirection(QVariantAnimation::Forward);
    m_shadowHeightAnimation.start();
    m_actionsOpacityAnimation.setDirection(QVariantAnimation::Forward);
    m_actionsOpacityAnimation.start();
}

void ProjectCard::hoverLeaveEvent(QGraphicsSceneHoverEvent* _event)
{
    QGraphicsRectItem::hoverLeaveEvent(_event);
    m_shadowHeightAnimation.setDirection(QVariantAnimation::Backward);
    m_shadowHeightAnimation.start();
    m_actionsOpacityAnimation.setDirection(QVariantAnimation::Backward);
    m_actionsOpacityAnimation.start();
}

void ProjectCard::mousePressEvent(QGraphicsSceneMouseEvent* _event)
{
    QGraphicsRectItem::mousePressEvent(_event);

    setZValue(nextZValue());

    m_decorationCenterPosition = _event->pos();
    m_decorationRadiusAnimation.setEasingCurve(QEasingCurve::InQuad);
    m_decorationRadiusAnimation.setStartValue(1.0);
    m_decorationRadiusAnimation.setEndValue(rect().width());
    for (const auto& rect : actionsRects()) {
        if (rect.contains(_event->pos())) {
            m_decorationCenterPosition = rect.center();
            m_decorationRadiusAnimation.setEasingCurve(QEasingCurve::OutQuad);
            m_decorationRadiusAnimation.setStartValue(rect.width() / 2);
            m_decorationRadiusAnimation.setEndValue(rect.height() * 2 / 3);
            break;
        }
    }
    m_decorationOpacityAnimation.setCurrentTime(0);
    m_decorationRadiusAnimation.start();
    m_decorationOpacityAnimation.setStartValue(0.5);
    m_decorationOpacityAnimation.setEndValue(0.15);
    m_decorationOpacityAnimation.start();
}

void ProjectCard::mouseReleaseEvent(QGraphicsSceneMouseEvent* _event)
{
    QGraphicsRectItem::mouseReleaseEvent(_event);

    bool needReorderCards = true;
    if (m_decorationOpacityAnimation.state() == QVariantAnimation::Running) {
        m_decorationOpacityAnimation.pause();
        m_decorationOpacityAnimation.setEndValue(0.0);
        m_decorationOpacityAnimation.resume();

        const auto iconsRects = actionsRects();
        if (m_project.type() == ManagementLayer::ProjectType::Local) {
            if (iconsRects[0].contains(_event->pos())) {
                emit projectsScene()->moveProjectToCloudRequested(m_project);
            } else if (iconsRects[1].contains(_event->pos())) {
                emit projectsScene()->hideProjectRequested(m_project);
                needReorderCards = false;
            } else {
                emit projectsScene()->projectPressed(m_project);
            }
        } else {
            if (iconsRects[0].contains(_event->pos())) {
                emit projectsScene()->changeProjectNameRequested(m_project);
            } else if (iconsRects[1].contains(_event->pos())) {
                emit projectsScene()->removeProjectRequested(m_project);
                needReorderCards = false;
            } else {
                emit projectsScene()->projectPressed(m_project);
            }
        }
    } else {
        m_decorationOpacityAnimation.setStartValue(0.15);
        m_decorationOpacityAnimation.setEndValue(0.0);
        m_decorationOpacityAnimation.start();
    }

    //
    // Если карточка не была удалена, возвращаем её на место
    //
    if (needReorderCards) {
        //
        // Делаем отложенный вызов, чтобы mouseGrabber сцены освободился
        //
        QTimer::singleShot(0, projectsScene(), [this] {
            emit projectsScene()->reorderProjectCardRequested(this);
        });
    }
}

ProjectsScene* ProjectCard::projectsScene() const
{
    ProjectsScene* projectsScene = qobject_cast<ProjectsScene*>(scene());
    Q_ASSERT(projectsScene);
    return projectsScene;
}

qreal ProjectCard::nextZValue() const
{
    static qreal z = 0.0;
    z += 0.001;
    return z;
}

QVector<QRectF> ProjectCard::actionsRects() const
{
    const QRectF backgroundRect = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins());
    const QPixmap& poster = m_project.poster();
    const QRectF posterRect(backgroundRect.topLeft(),
                            poster.size().scaled(backgroundRect.size().toSize(), Qt::KeepAspectRatio));
    QRectF iconRect(posterRect.right() + Ui::DesignSystem::layout().px12(),
                    backgroundRect.bottom() - Ui::DesignSystem::layout().px24() * 2,
                    Ui::DesignSystem::layout().px24(),
                    Ui::DesignSystem::layout().px24() * 2);

    QVector<QRectF> rects;
    const int kRepeats = m_project.type() == ManagementLayer::ProjectType::Local ? 2 : 2;
    for (int repeat = 0; repeat < kRepeats; ++repeat) {
        rects.append(iconRect);
        iconRect.moveLeft(iconRect.left() + Ui::DesignSystem::layout().px8() * 5);
    }

    return rects;
}

} // namespace


ProjectsScene::ProjectsScene(QObject* _parent)
    : QGraphicsScene(_parent)
{
}

void ProjectsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* _event)
{
    QGraphicsScene::mouseMoveEvent(_event);

    if (mouseGrabberItem() != nullptr) {
        emit reorderProjectCardRequested(mouseGrabberItem());
    }
}


// ****


class ProjectsCards::Implementation
{
public:
    explicit Implementation(QGraphicsView* _parent);

    /**
     * @brief Обновить размер карточек в соответствии с дизайн системой
     */
    void resizeCards();

    /**
     * @brief Упорядочить карточки проектов
     */
    void reorderCards();

    /**
     * @brief Определить новую позицию карточки, т.к. она была перенесена
     */
    void reorderCard(QGraphicsItem* _cardItem);


    ProjectsScene* scene = nullptr;
    ManagementLayer::ProjectsModel* projects = nullptr;
    QVector<ProjectCard*> projectsCards;
    bool cardsAnimationsAvailable = true;
    QHash<ProjectCard*, QPointer<QVariantAnimation>> projectsCardsAnimations;
};

ProjectsCards::Implementation::Implementation(QGraphicsView* _parent)
    : scene(new ProjectsScene(_parent))
{
}

void ProjectsCards::Implementation::resizeCards()
{
    const QRectF cardRect({0, 0}, Ui::DesignSystem::projectCard().size());
    for (auto card : projectsCards) {
        card->setRect(cardRect);
    }
}

void ProjectsCards::Implementation::reorderCards()
{
    //
    // Определим размер окна
    //
    const QSizeF viewSize = [this] {
        if (scene->views().isEmpty()) {
            return QSizeF();
        }

        const QGraphicsView* view = scene->views().first();
        return QSizeF(view->size());
    }();

    //
    // Определим количество карточек в ряду
    //
    const int cardsInRowCount = [width = viewSize.width()] () mutable {
        int count = 0;
        width -= Ui::DesignSystem::projectCard().margins().left();
        width -= Ui::DesignSystem::projectCard().margins().right();
        forever {
            width -= count > 0 ? Ui::DesignSystem::projectCard().spacing() : 0;
            width -= Ui::DesignSystem::projectCard().size().width();

            if (width > 0) {
                ++count;
            } else {
                break;
            }
        }
        return std::max(1, count);
    }();

    //
    // Определим видимую область экрана
    //
    const QRectF viewportRect = [this] {
        if (scene->views().isEmpty()) {
            return QRectF();
        }

        const QGraphicsView* view = scene->views().first();
        return view->mapToScene(view->viewport()->geometry()).boundingRect();
    }();


    const bool isLtr = QLocale().textDirection() == Qt::LeftToRight;
    const qreal sceneRectWidth = Ui::DesignSystem::projectCard().margins().left()
                                 + Ui::DesignSystem::projectCard().size().width() * cardsInRowCount
                                 + Ui::DesignSystem::projectCard().spacing() * (cardsInRowCount - 1)
                                 + Ui::DesignSystem::projectCard().margins().right();
    const qreal firstCardInRowX = [isLtr, sceneRectWidth] {
        return isLtr
                ? Ui::DesignSystem::projectCard().margins().left()
                : sceneRectWidth
                  - Ui::DesignSystem::projectCard().margins().right()
                  - Ui::DesignSystem::projectCard().size().width();
    }();

    //
    // Проходим все элементы (они упорядочены так, как должны идти элементы в сценарии
    //
    qreal x = firstCardInRowX;
    qreal y = Ui::DesignSystem::projectCard().margins().top();
    qreal maxY = 0.0;
    qreal lastItemHeight = 0.0;
    int currentCardInRow = 0;
    for (auto card : projectsCards) {
        //
        // ... корректируем позицию в соответствии с позицией карточки в ряду,
        //     или если предыдущая была вложена, а текущая нет
        //
        if (currentCardInRow == cardsInRowCount) {
            currentCardInRow = 0;
            x = firstCardInRowX;
            y += lastItemHeight + Ui::DesignSystem::projectCard().spacing();
        }
        //
        // ... позиционируем карточку, если она не перемещается в данный момент
        //     и новая позиция отличается от текущей
        //
        const QPointF newItemPosition(x, y);
        if (card != scene->mouseGrabberItem()
            && card->pos() != newItemPosition) {
            const QRectF itemRect(card->pos(), card->boundingRect().size());
            const QRectF newItemRect(newItemPosition, card->boundingRect().size());
            if (cardsAnimationsAvailable
                && scene->isActive()
                && viewportRect.intersects(itemRect.united(newItemRect))) {
                //
                // ... анимируем смещение
                //
                QPointer<QVariantAnimation>& moveAnimation = projectsCardsAnimations[card];
                if (moveAnimation.isNull()) {
                    moveAnimation = new QVariantAnimation(scene);
                    moveAnimation->setDuration(220);
                    moveAnimation->setEasingCurve(QEasingCurve::OutQuad);
                    QObject::connect(moveAnimation.data(), &QVariantAnimation::valueChanged, scene,
                                     [card] (const QVariant& _value) { card->setPos(_value.toPointF()); });
                }
                //
                // ... при необходимости анимируем положение карточки
                //
                if (moveAnimation->endValue().toPointF() != newItemPosition) {
                    if (moveAnimation->state() == QVariantAnimation::Running) {
                        moveAnimation->stop();
                    }
                    moveAnimation->setStartValue(card->pos());
                    moveAnimation->setEndValue(newItemPosition);
                    moveAnimation->start(QAbstractAnimation::DeleteWhenStopped);
                }
            } else {
                card->setPos(newItemPosition);
            }
        }
        //
        // ... и корректируем координаты для позиционирования следующих элементов
        //
        x += (isLtr ? 1 : -1) * (Ui::DesignSystem::projectCard().size().width()
                                 + Ui::DesignSystem::projectCard().spacing());
        lastItemHeight = Ui::DesignSystem::projectCard().size().height();

        if (maxY < y) {
            maxY = y;
        }

        ++currentCardInRow;
    }

    //
    // Обновляем размер сцены
    //
    QRectF newSceneRect = scene->sceneRect();
    newSceneRect.setRight(std::max(viewSize.width(), sceneRectWidth));
    newSceneRect.setBottom(std::max(viewSize.height(),
                                    maxY
                                    + Ui::DesignSystem::projectCard().size().height()
                                    + Ui::DesignSystem::projectCard().margins().bottom()));
    scene->setSceneRect(newSceneRect);
}

void ProjectsCards::Implementation::reorderCard(QGraphicsItem* _cardItem)
{
    //
    // Определим карточку, которая перемещена
    //
    ProjectCard* movedCard = static_cast<ProjectCard*>(_cardItem);
    if (movedCard == nullptr) {
        return;
    }

    //
    // Определим место, куда перемещена карточка
    //
    const QPointF movedCardPosition = movedCard->pos();
    const QRectF movedCardRect = movedCard->boundingRect();
    const qreal movedCardLeft = movedCardPosition.x();
    const qreal movedCardTop = movedCardPosition.y();
    const qreal movedCardBottom = movedCardTop + movedCardRect.height();

    //
    // Ищем элемент, который будет последним перед карточкой в её новом месте
    //
    ProjectCard* previousCard = nullptr;
    for (auto card : projectsCards) {
        if (card == movedCard) {
            continue;
        }

        const auto cardAnimation = projectsCardsAnimations[card];
        const QPointF cardPosition = cardAnimation.isNull() ? card->pos() : cardAnimation->endValue().toPointF();
        const QRectF cardRect = card->boundingRect();
        const qreal cardLeft = cardPosition.x();
        const qreal cardTop = cardPosition.y();
        const qreal cardBottom = cardTop + cardRect.height();

        if (// на разных линиях
            (movedCardTop > cardTop
            && fabs(movedCardTop - cardTop) >= movedCardRect.height()/2.)
            // на одной линии, но левее для письменности слева-направо
            || (QLocale().textDirection() == Qt::LeftToRight
                && movedCardTop < cardBottom
                && movedCardBottom > cardTop
                && fabs(movedCardTop - cardTop) < movedCardRect.height()/2.
                && movedCardLeft > cardLeft)
             // на одной линии, но правее для письменности справа-налево
             || (QLocale().textDirection() == Qt::RightToLeft
                 && movedCardTop < cardBottom
                 && movedCardBottom > cardTop
                 && fabs(movedCardTop - cardTop) < movedCardRect.height()/2.
                 && movedCardLeft < cardLeft)) {
            previousCard = card;
        }
        //
        // Если не после данного элемента, то прерываем поиск
        //
        else {
            break;
        }
    }

    //
    // Перемещаем проект
    //
    const bool isProjectMoved
            = projects->moveProject(movedCard->project(),
                    previousCard != nullptr ? previousCard->project() : ManagementLayer::Project());
    //
    // В случае, сли проект не был перемещён, нужно вернуть его на место на доске
    //
    if (!isProjectMoved) {
        reorderCards();
    }
}


// ****


ProjectsCards::ProjectsCards(QWidget* _parent)
    : QGraphicsView(_parent),
      d(new Implementation(this))
{
    setFrameShape(QFrame::NoFrame);
    setScene(d->scene);
    setVerticalScrollBar(new ScrollBar(this));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(d->scene, &ProjectsScene::projectPressed, this, &ProjectsCards::openProjectRequested);
    connect(d->scene, &ProjectsScene::moveProjectToCloudRequested, this, &ProjectsCards::moveProjectToCloudRequested);
    connect(d->scene, &ProjectsScene::hideProjectRequested, this, &ProjectsCards::hideProjectRequested);
    connect(d->scene, &ProjectsScene::changeProjectNameRequested, this, &ProjectsCards::changeProjectNameRequested);
    connect(d->scene, &ProjectsScene::removeProjectRequested, this, &ProjectsCards::removeProjectRequested);
    connect(d->scene, &ProjectsScene::reorderProjectCardRequested, this,
            [this] (QGraphicsItem* _projectCard) { d->reorderCard(_projectCard); });
}

void ProjectsCards::setBackgroundColor(const QColor& _color)
{
    scene()->setBackgroundBrush(_color);
}

void ProjectsCards::setProjects(ManagementLayer::ProjectsModel* _projects)
{
    if (d->projects == _projects) {
        return;
    }

    if (d->projects != nullptr) {
        d->projects->disconnect(this);
    }

    d->projects = _projects;

    if (d->projects == nullptr) {
        return;
    }

    connect(d->projects, &ManagementLayer::ProjectsModel::rowsInserted, this,
            [this] (const QModelIndex& _parent, int _first, int _last)
    {
        Q_UNUSED(_parent)

        //
        // Вставляем карточки
        //
        for (int row = _first; row <= _last; ++row) {
            auto projectCard = new ProjectCard;
            projectCard->setProject(d->projects->projectAt(row));
            d->projectsCards.insert(row, projectCard);
            d->scene->addItem(projectCard);

            //
            // Вставляется новая карточка слева вверху, чтобы при первом запуске не было скопления
            // карточек в одном месте и чтобы они красиво вылетали, при добавлении
            //
            projectCard->setPos(-projectCard->rect().width(), -projectCard->rect().height());
        }

        //
        // Корректируем положение карточек
        //
        d->reorderCards();

        //
        // При необходимости посылаем запрос на отображение
        //
        notifyVisibleChange();
    });
    connect(d->projects, &ManagementLayer::ProjectsModel::rowsRemoved, this,
            [this] (const QModelIndex& _parent, int _first, int _last)
    {

        Q_UNUSED(_parent)

        //
        // Удаляем карточки
        //
        for (int row = _last; row >= _first; --row) {
            auto projectCard = d->projectsCards.takeAt(row);
            d->scene->removeItem(projectCard);
            d->projectsCardsAnimations.remove(projectCard);
            delete projectCard;
        }

        //
        // Корректируем положение карточек
        //
        d->reorderCards();

        //
        // При необходимости посылаем запрос на скрытие
        //
        notifyVisibleChange();
    });
    connect(d->projects, &ManagementLayer::ProjectsModel::rowsMoved, this,
            [this] (const QModelIndex& _sourceParent, int _sourceStart, int _sourceEnd,
                    const QModelIndex& _destinationParent, int _destination)
    {

        Q_UNUSED(_sourceParent)
        Q_UNUSED(_destinationParent)

        //
        // Ожидаем перемещение только одной карточки
        //
        Q_ASSERT(_sourceStart == _sourceEnd);

        //
        // Перемещаем карточки
        //
        d->projectsCards.move(_sourceStart, _sourceStart > _destination ? _destination : _destination - 1);
        //
        // и обновляем представление
        //
        d->reorderCards();
    });
    connect(d->projects, &ManagementLayer::ProjectsModel::dataChanged, this,
            [this] (const QModelIndex& _from, const QModelIndex& _to)
    {
        //
        // Ожидаем изменение только одной карточки
        //
        Q_ASSERT(_from == _to);

        //
        // Обновляем проект карточки
        //
        auto card = d->projectsCards.at(_from.row());
        const auto cardProject = d->projects->projectAt(_from.row());
        card->setProject(cardProject);
    });

    notifyVisibleChange();
}

bool ProjectsCards::event(QEvent *_event)
{
    switch (static_cast<int>(_event->type())) {
        case static_cast<QEvent::Type>(EventType::DesignSystemChangeEvent): {
            d->resizeCards();
            d->reorderCards();
            return true;
        }

        default: {
            return QGraphicsView::event(_event);
        }
    }
}

ProjectsCards::~ProjectsCards() = default;

void ProjectsCards::resizeEvent(QResizeEvent* _event)
{
    QGraphicsView::resizeEvent(_event);

    d->cardsAnimationsAvailable = false;
    d->reorderCards();
    d->cardsAnimationsAvailable = true;
}

void ProjectsCards::notifyVisibleChange()
{
    if (d->projects->isEmpty() && isVisible()) {
        emit hideRequested();
    } else if (!d->projects->isEmpty() && isHidden()) {
        emit showRequested();
    }
}

} // namespace Ui
