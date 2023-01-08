#include "projects_cards.h"

#include <include/custom_events.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <management_layer/content/projects/project.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QPointer>
#include <QResizeEvent>
#include <QStyleOption>
#include <QVariantAnimation>

#include <cmath>


namespace Ui {

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
    void paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option,
               QWidget* _widget) override;

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
     * @brief Проект для отображения на карточке
     */
    ManagementLayer::Project m_project;

    /**
     * @brief  Декорации тени при наведении
     */
    QVariantAnimation m_shadowHeightAnimation;

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
    setRect(QRectF({ 0, 0 }, Ui::DesignSystem::projectCard().size()));
    setAcceptHoverEvents(true);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_shadowHeightAnimation.setStartValue(Ui::DesignSystem::card().minimumShadowBlurRadius());
    m_shadowHeightAnimation.setEndValue(Ui::DesignSystem::card().maximumShadowBlurRadius());
    m_shadowHeightAnimation.setEasingCurve(QEasingCurve::OutQuad);
    m_shadowHeightAnimation.setDuration(160);
    QObject::connect(&m_shadowHeightAnimation, &QVariantAnimation::valueChanged,
                     &m_shadowHeightAnimation, [this] { update(); });

    m_decorationRadiusAnimation.setDuration(240);
    QObject::connect(&m_decorationRadiusAnimation, &QVariantAnimation::valueChanged,
                     &m_decorationRadiusAnimation, [this] { update(); });

    m_decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    m_decorationOpacityAnimation.setDuration(240);
    QObject::connect(&m_decorationOpacityAnimation, &QVariantAnimation::valueChanged,
                     &m_decorationOpacityAnimation, [this] { update(); });
}

ProjectCard::~ProjectCard()
{
    m_shadowHeightAnimation.disconnect();
    m_shadowHeightAnimation.stop();
    m_decorationRadiusAnimation.disconnect();
    m_decorationRadiusAnimation.stop();
    m_decorationOpacityAnimation.disconnect();
    m_decorationOpacityAnimation.stop();
}

void ProjectCard::setProject(const ManagementLayer::Project& _project)
{
    //
    // Всегда обновляем проект, т.к. мог измениться постер, а мы не сравниваем картинки
    //
    m_project = _project;

    //
    // Если название не влезает, то установим его тултипом
    //
    const QRectF backgroundRect = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins());
    const QSizeF posterSize
        = m_project.poster().size().scaled(backgroundRect.size().toSize(), Qt::KeepAspectRatio);
    const auto textWidth
        = backgroundRect.width() - posterSize.width() - Ui::DesignSystem::layout().px12() * 2;
    if (TextHelper::fineTextWidthF(m_project.name(), Ui::DesignSystem::font().h6()) > textWidth) {
        setToolTip(m_project.name());
    } else {
        setToolTip({});
    }

    update();
}

ManagementLayer::Project ProjectCard::project() const
{
    return m_project;
}

void ProjectCard::paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option,
                        QWidget* _widget)
{
    Q_UNUSED(_option)

    _painter->setOpacity(1.0);

    const bool isLeftToRight = _widget->isLeftToRight();
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
        backgroundImagePainter.drawRoundedRect(QRect({ 0, 0 }, backgroundPixmapCache.size()),
                                               borderRadius, borderRadius);
    }
    //
    // ... рисуем тень
    //
    const qreal shadowHeight
        = std::max(Ui::DesignSystem::floatingToolBar().minimumShadowBlurRadius(),
                   m_shadowHeightAnimation.currentValue().toReal());
    const QPixmap shadow = ImageHelper::dropShadow(
        backgroundPixmapCache, Ui::DesignSystem::floatingToolBar().shadowMargins(), shadowHeight,
        Ui::DesignSystem::color().shadow());
    _painter->drawPixmap(0, 0, shadow);
    //
    // ... рисуем сам фон
    //
    _painter->drawPixmap(backgroundRect, backgroundPixmapCache, backgroundPixmapCache.rect());

    //
    // Постер
    //
    const QSizeF posterSize
        = m_project.poster().size().scaled(backgroundRect.size().toSize(), Qt::KeepAspectRatio);
    const QRectF posterRect(isLeftToRight
                                ? backgroundRect.topLeft()
                                : backgroundRect.topRight() - QPointF(posterSize.width(), 0),
                            posterSize);
    const auto scaledPoster = m_project.poster().scaled(posterSize.toSize(), Qt::KeepAspectRatio,
                                                        Qt::SmoothTransformation);
    _painter->drawPixmap(posterRect, scaledPoster, scaledPoster.rect());

    //
    // Заголовок
    //
    _painter->setPen(Ui::DesignSystem::color().onBackground());
    _painter->setFont(Ui::DesignSystem::font().h6());
    const QRectF textRect(isLeftToRight ? posterRect.right() + Ui::DesignSystem::layout().px16()
                                        : backgroundRect.left() + Ui::DesignSystem::layout().px12(),
                          backgroundRect.top() + Ui::DesignSystem::layout().px8(),
                          backgroundRect.width() - posterRect.width()
                              - Ui::DesignSystem::layout().px12() * 2,
                          TextHelper::fineLineSpacing(_painter->font()));
    _painter->drawText(
        textRect, Qt::AlignLeft | Qt::AlignVCenter,
        TextHelper::elidedText(m_project.name(), _painter->font(), textRect.width()));

    //
    // Логлайн
    //
    _painter->setPen(Ui::DesignSystem::color().onBackground());
    _painter->setFont(Ui::DesignSystem::font().body2());
    const auto fullLoglinHeight
        = TextHelper::heightForWidth(m_project.logline(), _painter->font(), textRect.width());
    const auto fontLineSpacing = TextHelper::fineLineSpacing(_painter->font());
    const int loglineMaxLines = m_project.isLocal() ? 5 : 4;
    const QRectF loglineRect(textRect.left(), textRect.bottom() + Ui::DesignSystem::layout().px8(),
                             textRect.width(),
                             std::min(fullLoglinHeight, fontLineSpacing * loglineMaxLines));
    const QString loglineCorrected
        = TextHelper::elidedText(m_project.logline(), _painter->font(), loglineRect);
    _painter->drawText(loglineRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                       loglineCorrected);

    //
    // Дата последнего изменения
    //
    const QColor betweenBackgroundColor = ColorHelper::colorBetween(
        Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::color().background());
    _painter->setPen(betweenBackgroundColor);
    const auto lastDateTop = loglineCorrected.isEmpty()
        ? loglineRect.top()
        : loglineRect.bottom() + Ui::DesignSystem::layout().px4();
    const QRectF lastDateRect(loglineRect.left(), lastDateTop, loglineRect.width(),
                              fontLineSpacing);
    _painter->drawText(lastDateRect, Qt::AlignLeft | Qt::AlignTop, m_project.displayLastEditTime());

    //
    // Роль в проекте
    //
    if (m_project.isRemote()) {
        if (m_project.isOwner()) {
            _painter->setFont(Ui::DesignSystem::font().subtitle2());
            const QRectF roleRect(lastDateRect.bottomLeft(),
                                  QPointF(lastDateRect.right(),
                                          lastDateRect.bottom() + fontLineSpacing
                                              + Ui::DesignSystem::layout().px(6)));
            _painter->drawText(roleRect, Qt::AlignLeft | Qt::AlignBottom,
                               projectsScene()->tr("Owner"));
        } else {
            QString roleIcon;
            QString roleText;
            switch (m_project.editingMode()) {
            case ManagementLayer::DocumentEditingMode::Edit: {
                roleIcon = u8"\U000F03EB";
                roleText = projectsScene()->tr("Editor");
                break;
            }

            case ManagementLayer::DocumentEditingMode::Comment: {
                roleIcon = u8"\U000F0208";
                roleText = projectsScene()->tr("Commentator");
                break;
            }

            case ManagementLayer::DocumentEditingMode::Read: {
                roleIcon = u8"\U000F0208";
                roleText = projectsScene()->tr("Reader");
                break;
            }
            }

            _painter->setFont(Ui::DesignSystem::font().iconsSmall());
            const QRectF iconRect(lastDateRect.bottomLeft(),
                                  QSizeF(Ui::DesignSystem::layout().px24(),
                                         fontLineSpacing + Ui::DesignSystem::layout().px4()));
            _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignBottom, roleIcon);


            _painter->setFont(Ui::DesignSystem::font().subtitle2());
            const QRectF roleRect(iconRect.topRight(),
                                  QPointF(lastDateRect.right(),
                                          lastDateRect.bottom() + fontLineSpacing
                                              + Ui::DesignSystem::layout().px(6)));
            _painter->drawText(roleRect, Qt::AlignLeft | Qt::AlignBottom, roleText);
        }
    }

    //
    // Иконка расположения проекта
    //
    _painter->setPen(Ui::DesignSystem::color().onBackground());
    _painter->setFont(Ui::DesignSystem::font().iconsMid());
    const QRectF iconRect(
        isLeftToRight ? backgroundRect.right() - Ui::DesignSystem::layout().px24() * 2
                      : backgroundRect.left(),
        backgroundRect.bottom() - Ui::DesignSystem::layout().px24() * 2,
        Ui::DesignSystem::layout().px24() * 2, Ui::DesignSystem::layout().px24() * 2);
    _painter->drawText(iconRect, Qt::AlignCenter,
                       m_project.type() == ManagementLayer::ProjectType::Cloud ? u8"\U000F0163"
                                                                               : u8"\U000F0322");

    //
    // Декорация
    //
    if (m_decorationRadiusAnimation.state() == QVariantAnimation::Running
        || m_decorationOpacityAnimation.state() == QVariantAnimation::Running
        || QApplication::mouseButtons().testFlag(Qt::LeftButton)) {
        _painter->setClipRect(backgroundRect);
        _painter->setPen(Qt::NoPen);
        _painter->setBrush(Ui::DesignSystem::color().accent());
        _painter->setOpacity(m_decorationOpacityAnimation.currentValue().toReal());
        _painter->drawEllipse(m_decorationCenterPosition,
                              m_decorationRadiusAnimation.currentValue().toReal(),
                              m_decorationRadiusAnimation.currentValue().toReal());
    }
}

void ProjectCard::hoverEnterEvent(QGraphicsSceneHoverEvent* _event)
{
    QGraphicsRectItem::hoverEnterEvent(_event);
    m_shadowHeightAnimation.setDirection(QVariantAnimation::Forward);
    m_shadowHeightAnimation.start();
}

void ProjectCard::hoverLeaveEvent(QGraphicsSceneHoverEvent* _event)
{
    QGraphicsRectItem::hoverLeaveEvent(_event);
    m_shadowHeightAnimation.setDirection(QVariantAnimation::Backward);
    m_shadowHeightAnimation.start();
}

void ProjectCard::mousePressEvent(QGraphicsSceneMouseEvent* _event)
{
    if (!boundingRect().contains(_event->pos())) {
        if (m_shadowHeightAnimation.direction() == QVariantAnimation::Forward) {
            m_shadowHeightAnimation.setDirection(QVariantAnimation::Backward);
            m_shadowHeightAnimation.start();

            m_decorationOpacityAnimation.setEndValue(0.0);
        }
        return;
    }

    QGraphicsRectItem::mousePressEvent(_event);

    setZValue(nextZValue());

    m_decorationCenterPosition = _event->pos();
    m_decorationRadiusAnimation.setEasingCurve(QEasingCurve::InQuad);
    m_decorationRadiusAnimation.setStartValue(1.0);
    m_decorationRadiusAnimation.setEndValue(rect().width());
    m_decorationOpacityAnimation.setCurrentTime(0);
    m_decorationRadiusAnimation.start();
    m_decorationOpacityAnimation.setStartValue(0.5);
    m_decorationOpacityAnimation.setEndValue(0.15);
    m_decorationOpacityAnimation.start();

    if (_event->button() == Qt::RightButton) {
        emit projectsScene()->projectContextMenuRequested(m_project);
    }
}

void ProjectCard::mouseReleaseEvent(QGraphicsSceneMouseEvent* _event)
{
    if (!boundingRect().contains(_event->pos())) {
        return;
    }

    QGraphicsRectItem::mouseReleaseEvent(_event);

    if (_event->button() == Qt::RightButton) {
        return;
    }

    auto scene = projectsScene();

    bool needToReorder = true;
    if (m_decorationOpacityAnimation.state() == QVariantAnimation::Running) {
        m_decorationOpacityAnimation.pause();
        m_decorationOpacityAnimation.setEndValue(0.0);
        m_decorationOpacityAnimation.resume();

        //
        // Отложенно уведомляем о клике на карточке.
        // NOTE: Важно, чтобы это событие выполнилось после завершения анимации, чтобы
        // корректно обработать кейс, когда проект был удалён/переименован на компе, в
        // результате чего, клик на карточке приведёт к её удалению
        //
        QMetaObject::invokeMethod(scene, [this, scene] { emit scene->projectPressed(m_project); });
    } else {
        m_decorationOpacityAnimation.setStartValue(0.15);
        m_decorationOpacityAnimation.setEndValue(0.0);
        m_decorationOpacityAnimation.start();
    }

    //
    // Возвращаем карточку на место
    //
    // Делаем отложенный вызов, чтобы mouseGrabber сцены освободился
    //
    if (needToReorder) {
        QMetaObject::invokeMethod(
            scene, [this, scene] { emit scene->reorderProjectCardRequested(this); },
            Qt::QueuedConnection);
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
    const QRectF cardRect({ 0, 0 }, Ui::DesignSystem::projectCard().size());
    for (auto card : std::as_const(projectsCards)) {
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

        const QGraphicsView* view = scene->views().constFirst();
        return QSizeF(view->size());
    }();

    //
    // Определим количество карточек в ряду
    //
    const int cardsInRowCount = [width = viewSize.width()]() mutable {
        int count = 0;
        width -= Ui::DesignSystem::projectCard().margins().left();
        width -= Ui::DesignSystem::projectCard().margins().right();
        forever
        {
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

        const QGraphicsView* view = scene->views().constFirst();
        return view->mapToScene(view->viewport()->geometry()).boundingRect();
    }();


    const qreal sceneRectWidth
        = std::max(Ui::DesignSystem::projectCard().margins().left()
                       + Ui::DesignSystem::projectCard().size().width() * cardsInRowCount
                       + Ui::DesignSystem::projectCard().spacing() * (cardsInRowCount - 1)
                       + Ui::DesignSystem::projectCard().margins().right(),
                   viewSize.width());
    const auto isLeftToRight = QApplication::isLeftToRight();
    const qreal firstCardInRowX = [isLeftToRight, sceneRectWidth]() {
        return isLeftToRight ? Ui::DesignSystem::projectCard().margins().left()
                             : sceneRectWidth - Ui::DesignSystem::projectCard().margins().right()
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
    for (auto card : std::as_const(projectsCards)) {
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
        if (card != scene->mouseGrabberItem() && card->pos() != newItemPosition) {
            const QRectF itemRect(card->pos(), card->boundingRect().size());
            const QRectF newItemRect(newItemPosition, card->boundingRect().size());
            if (cardsAnimationsAvailable && scene->isActive()
                && viewportRect.intersects(itemRect.united(newItemRect))) {
                //
                // ... анимируем смещение
                //
                QPointer<QVariantAnimation>& moveAnimation = projectsCardsAnimations[card];
                if (moveAnimation.isNull()) {
                    moveAnimation = new QVariantAnimation(scene);
                    moveAnimation->setDuration(220);
                    moveAnimation->setEasingCurve(QEasingCurve::OutQuad);
                    QObject::connect(
                        moveAnimation.data(), &QVariantAnimation::valueChanged, scene,
                        [card](const QVariant& _value) { card->setPos(_value.toPointF()); });
                }
                //
                // ... при необходимости анимируем положение карточки
                //
                if (moveAnimation->endValue().toPointF() != newItemPosition) {
                    if (moveAnimation->state() == QVariantAnimation::Running) {
                        moveAnimation->pause();
                    } else {
                        moveAnimation->setStartValue(card->pos());
                    }
                    moveAnimation->setEndValue(newItemPosition);
                    if (moveAnimation->state() == QVariantAnimation::Paused) {
                        moveAnimation->resume();
                    } else {
                        moveAnimation->start(QAbstractAnimation::DeleteWhenStopped);
                    }
                }
            } else {
                card->setPos(newItemPosition);
            }
        }
        //
        // ... и корректируем координаты для позиционирования следующих элементов
        //
        x += (isLeftToRight ? 1 : -1)
            * (Ui::DesignSystem::projectCard().size().width()
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
                                    maxY + Ui::DesignSystem::projectCard().size().height()
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
    for (auto card : std::as_const(projectsCards)) {
        if (card == movedCard) {
            continue;
        }

        const auto cardAnimation = projectsCardsAnimations[card];
        const QPointF cardPosition
            = cardAnimation.isNull() ? card->pos() : cardAnimation->endValue().toPointF();
        const QRectF cardRect = card->boundingRect();
        const qreal cardLeft = cardPosition.x();
        const qreal cardTop = cardPosition.y();
        const qreal cardBottom = cardTop + cardRect.height();

        if ( // на разных линиях
            (movedCardTop > cardTop && fabs(movedCardTop - cardTop) >= movedCardRect.height() / 2.)
            // на одной линии, но левее для письменности слева-направо
            || (QLocale().textDirection() == Qt::LeftToRight && movedCardTop < cardBottom
                && movedCardBottom > cardTop
                && fabs(movedCardTop - cardTop) < movedCardRect.height() / 2.
                && movedCardLeft > cardLeft)
            // на одной линии, но правее для письменности справа-налево
            || (QLocale().textDirection() == Qt::RightToLeft && movedCardTop < cardBottom
                && movedCardBottom > cardTop
                && fabs(movedCardTop - cardTop) < movedCardRect.height() / 2.
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
    const bool isProjectMoved = projects->moveProject(
        movedCard->project(),
        previousCard != nullptr ? previousCard->project() : ManagementLayer::Project());
    //
    // В случае, если проект не был перемещён, нужно вернуть его на место на доске
    //
    if (!isProjectMoved) {
        reorderCards();
    }
}


// ****


ProjectsCards::ProjectsCards(QWidget* _parent)
    : QGraphicsView(_parent)
    , d(new Implementation(this))
{
    setFrameShape(QFrame::NoFrame);
    setScene(d->scene);
    setVerticalScrollBar(new ScrollBar(this));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(d->scene, &ProjectsScene::projectPressed, this, &ProjectsCards::openProjectRequested);
    connect(d->scene, &ProjectsScene::projectContextMenuRequested, this,
            &ProjectsCards::projectContextMenuRequested);
    connect(d->scene, &ProjectsScene::reorderProjectCardRequested, this,
            [this](QGraphicsItem* _projectCard) { d->reorderCard(_projectCard); });
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
            [this](const QModelIndex& _parent, int _first, int _last) {
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
                    // Вставляется новая карточка слева вверху, чтобы при первом запуске не было
                    // скопления карточек в одном месте и чтобы они красиво вылетали, при добавлении
                    //
                    projectCard->setPos(-projectCard->rect().width(),
                                        -projectCard->rect().height());
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
            [this](const QModelIndex& _parent, int _first, int _last) {
                Q_UNUSED(_parent)

                //
                // Удаляем карточки
                //
                for (int row = _last; row >= _first; --row) {
                    auto projectCard = d->projectsCards.takeAt(row);
                    d->scene->removeItem(projectCard);
                    d->projectsCardsAnimations.remove(projectCard);
                    delete projectCard;
                    projectCard = nullptr;
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
            [this](const QModelIndex& _sourceParent, int _sourceStart, int _sourceEnd,
                   const QModelIndex& _destinationParent, int _destination) {
                Q_UNUSED(_sourceParent)
                Q_UNUSED(_destinationParent)

                //
                // Ожидаем перемещение только одной карточки
                //
                Q_ASSERT(_sourceStart == _sourceEnd);

                //
                // Перемещаем карточки
                //
                d->projectsCards.move(
                    _sourceStart, _sourceStart > _destination ? _destination : _destination - 1);
                //
                // и обновляем представление
                //
                d->reorderCards();
            });
    connect(d->projects, &ManagementLayer::ProjectsModel::dataChanged, this,
            [this](const QModelIndex& _from, const QModelIndex& _to) {
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

bool ProjectsCards::event(QEvent* _event)
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
