#include "projects_cards.h"

#include <domain/project.h>

#include <ui/design_system/design_system.h>

#include <utils/helpers/image_helper.h>

#include <QAbstractItemModel>
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
     * @brief Задать проект для отображения на карточке
     */
    void setProject(const Domain::Project& _project);

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
     * @brief Проект для отображения на карточке
     */
    Domain::Project m_project;

    /**
     * @brief  Декорации тени при наведении
     */
    QVariantAnimation m_shadowHeightAnimation;
};

ProjectCard::ProjectCard(QGraphicsItem* _parent)
    : QGraphicsRectItem(_parent)
{
    setRect(0, 0, 400, 210);
    setAcceptHoverEvents(true);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_shadowHeightAnimation.setStartValue(Ui::DesignSystem::card().minimumShadowBlurRadius());
    m_shadowHeightAnimation.setEndValue(Ui::DesignSystem::card().maximumShadowBlurRadius());
    m_shadowHeightAnimation.setEasingCurve(QEasingCurve::OutQuad);
    m_shadowHeightAnimation.setDuration(160);
    QObject::connect(&m_shadowHeightAnimation, &QVariantAnimation::valueChanged, [this] { update(); });
}

void ProjectCard::setProject(const Domain::Project& _project)
{
    if (m_project == _project) {
        return;
    }

    m_project = _project;
    update();
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
                                        m_shadowHeightAnimation.currentValue().toReal());
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

    //
    // TODO: Постер
    //
    static QPixmap poster;
    if (poster.height() != backgroundRect.size().toSize().height()) {
        poster = QPixmap(":/images/movie-poster").scaled(backgroundRect.size().toSize(),
                                                         Qt::KeepAspectRatio,
                                                         Qt::SmoothTransformation);
    }
    _painter->drawPixmap(backgroundRect.topLeft(), poster);

    //
    // Заголовок
    //

    //
    // Путь
    //

    //
    // Логлайн
    //

    //
    // Дата последнего изменения
    //

    //
    // Иконки действий
    //
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

} // namespace

class ProjectsCards::Implementation
{
public:
    explicit Implementation(QGraphicsView* _parent);

    /**
     * @brief Упорядочить карточки проектов
     */
    void reorderCards();

    QGraphicsScene* scene = nullptr;
    Domain::ProjectsModel* projects = nullptr;
};

ProjectsCards::Implementation::Implementation(QGraphicsView* _parent)
    : scene(new QGraphicsScene(_parent))
{
}

void ProjectsCards::Implementation::reorderCards()
{

}


// ****


ProjectsCards::ProjectsCards(QWidget* _parent)
    : QGraphicsView(_parent),
      d(new Implementation(this))
{
    setFrameShape(QFrame::NoFrame);
    setScene(d->scene);
}

void ProjectsCards::setBackgroundColor(const QColor& _color)
{
    scene()->setBackgroundBrush(_color);
}

void ProjectsCards::setProjects(Domain::ProjectsModel* _projects)
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

    connect(d->projects, &QAbstractListModel::rowsInserted, this,
            [this] (const QModelIndex& _parent, int _first, int _last)
    {
        Q_UNUSED(_parent);

        //
        // Ожидаем вставку только наверху
        //
        Q_ASSERT(_first == 0);

        //
        // Вставляем карточки
        //
        for (int row = _first; row <= _last; ++row) {
            //
            // Вставляется новая карточка слева вверху
            //
            auto projectCard = new ProjectCard;
            projectCard->setProject(d->projects->projectAt(_first));
            d->scene->addItem(projectCard);

            //
            // Корректируем расположение карточек в соответствии с новыми реалиями
            //
            d->reorderCards();
        }
    });
}

ProjectsCards::~ProjectsCards() = default;

void ProjectsCards::resizeEvent(QResizeEvent* _event)
{
    QGraphicsView::resizeEvent(_event);
}

}
