#include "floating_tool_bar.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/icon_helper.h>
#include <utils/helpers/image_helper.h>

#include <QAction>
#include <QPaintEvent>
#include <QPainter>
#include <QToolTip>
#include <QVariantAnimation>


namespace {
const char* kActionWidthKey = "action-width";
}

class FloatingToolBar::Implementation
{
public:
    explicit Implementation(FloatingToolBar* _q);

    /**
     * @brief Анимировать клик
     */
    void animateClick();

    /**
     * @brief Анимировать тень
     */
    void animateHoverIn();
    void animateHoverOut();

    /**
     * @brief Получить пункт меню по координате
     */
    QAction* actionAt(const QPoint& _coordinate) const;

    /**
     * @brief Ширина действия
     */
    qreal actionWidth(QAction* _action) const;


    FloatingToolBar* q = nullptr;

    /**
     * @brief Ориентация панели
     */
    Qt::Orientation orientation = Qt::Horizontal;

    /**
     * @brief Отбражать ли панель в стиле шторки
     */
    bool isCurtain = false;

    /**
     * @brief Иконка на которой кликнули последней
     */
    QAction* lastPressedAction = nullptr;

    /**
     * @brief Цвета иконок
     */
    QMap<const QAction*, QColor> actionToColor;

    /**
     * @brief  Декорации иконки при клике
     */
    QVariantAnimation decorationRadiusAnimation;
    QVariantAnimation decorationOpacityAnimation;

    /**
     * @brief  Декорации тени при наведении
     */
    QVariantAnimation opacityAnimation;
    QVariantAnimation shadowBlurRadiusAnimation;
};

FloatingToolBar::Implementation::Implementation(FloatingToolBar* _q)
    : q(_q)
{
    decorationRadiusAnimation.setEasingCurve(QEasingCurve::OutQuad);
    decorationRadiusAnimation.setDuration(160);

    decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationOpacityAnimation.setStartValue(0.5);
    decorationOpacityAnimation.setEndValue(0.0);
    decorationOpacityAnimation.setDuration(160);

    opacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    opacityAnimation.setDuration(160);
    opacityAnimation.setStartValue(0.6);
    opacityAnimation.setEndValue(1.0);
    opacityAnimation.setCurrentTime(0);

    shadowBlurRadiusAnimation.setEasingCurve(QEasingCurve::OutQuad);
    shadowBlurRadiusAnimation.setDuration(160);
}

void FloatingToolBar::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}

void FloatingToolBar::Implementation::animateHoverIn()
{
    if (shadowBlurRadiusAnimation.direction() == QVariantAnimation::Forward
        && shadowBlurRadiusAnimation.currentValue() == shadowBlurRadiusAnimation.endValue()) {
        return;
    }

    opacityAnimation.setDirection(QVariantAnimation::Forward);
    opacityAnimation.start();
    shadowBlurRadiusAnimation.setDirection(QVariantAnimation::Forward);
    shadowBlurRadiusAnimation.start();
}

void FloatingToolBar::Implementation::animateHoverOut()
{
    opacityAnimation.setDirection(QVariantAnimation::Backward);
    opacityAnimation.start();
    shadowBlurRadiusAnimation.setDirection(QVariantAnimation::Backward);
    shadowBlurRadiusAnimation.start();
}

QAction* FloatingToolBar::Implementation::actionAt(const QPoint& _coordinate) const
{
    qreal actionLeft = q->isLeftToRight() || orientation == Qt::Vertical
        ? Ui::DesignSystem::floatingToolBar().shadowMargins().left()
            + Ui::DesignSystem::floatingToolBar().margins().left()
            - (Ui::DesignSystem::floatingToolBar().spacing() / 2.0)
        : 0.0;
    for (QAction* action : q->actions()) {
        if (!action->isVisible() || action->isSeparator()) {
            continue;
        }

        //
        // Для RTL определяем левую позицию действия на проходе с этим действием
        //
        if (q->isRightToLeft() && orientation != Qt::Vertical) {
            if (qFuzzyCompare(actionLeft, 0.0)) {
                actionLeft = q->width() - actionWidth(action)
                    - Ui::DesignSystem::floatingToolBar().margins().right()
                    - Ui::DesignSystem::floatingToolBar().shadowMargins().right()
                    - (Ui::DesignSystem::floatingToolBar().spacing() / 2.0);
            } else {
                actionLeft -= actionWidth(action) + Ui::DesignSystem::floatingToolBar().spacing();
            }
        }

        const qreal actionRight
            = actionLeft + actionWidth(action) + Ui::DesignSystem::floatingToolBar().spacing();

        if (orientation == Qt::Horizontal) {
            if (actionLeft < _coordinate.x() && _coordinate.x() < actionRight) {
                return action;
            }
        } else {
            if (actionLeft < _coordinate.y() && _coordinate.y() < actionRight) {
                return action;
            }
        }

        //
        // Для LTR определяем левую позицию следующего элемента
        //
        if (q->isLeftToRight() || orientation == Qt::Vertical) {
            actionLeft = actionRight;
        }
    }

    return nullptr;
}

qreal FloatingToolBar::Implementation::actionWidth(QAction* _action) const
{
    const auto actionWidth = _action->property(kActionWidthKey);
    if (actionWidth.isNull()) {
        return Ui::DesignSystem::floatingToolBar().iconSize().width();
    }

    return actionWidth.toReal();
}


// ****


FloatingToolBar::FloatingToolBar(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setOpacity(d->opacityAnimation.startValue().toDouble());

    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });
    connect(&d->opacityAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { setOpacity(_value.toDouble()); });
    connect(&d->shadowBlurRadiusAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });
}

FloatingToolBar::~FloatingToolBar() = default;

void FloatingToolBar::setOrientation(Qt::Orientation _orientation)
{
    if (d->orientation == _orientation) {
        return;
    }

    d->orientation = _orientation;
    updateGeometry();
    update();
}

void FloatingToolBar::setCurtain(bool _curtain)
{
    if (d->isCurtain == _curtain) {
        return;
    }

    d->isCurtain = _curtain;
    update();
}

void FloatingToolBar::setStartOpacity(qreal _opacity)
{
    d->opacityAnimation.setStartValue(_opacity);
}

void FloatingToolBar::setActionCustomWidth(QAction* _action, int _width)
{
    if (!actions().contains(_action)) {
        return;
    }

    _action->setProperty(kActionWidthKey, _width);
    updateGeometry();
}

void FloatingToolBar::clearActionCustomWidth(QAction* _action)
{
    if (!actions().contains(_action)) {
        return;
    }

    _action->setProperty(kActionWidthKey, {});
    updateGeometry();
}

int FloatingToolBar::actionCustomWidth(QAction* _action) const
{
    if (!actions().contains(_action) || !_action->isVisible()) {
        return 0;
    }

    return _action->property(kActionWidthKey).toInt();
}

QSize FloatingToolBar::sizeHint() const
{
    const auto allActions = actions();
    const auto visibleActionsSize
        = std::count_if(allActions.begin(), allActions.end(),
                        [](QAction* _action) { return _action->isVisible(); });
    const qreal width = Ui::DesignSystem::floatingToolBar().shadowMargins().left()
        + Ui::DesignSystem::floatingToolBar().margins().left()
        + Ui::DesignSystem::floatingToolBar().iconSize().width() * visibleActionsSize
        + Ui::DesignSystem::floatingToolBar().spacing() * (visibleActionsSize - 1)
        + Ui::DesignSystem::floatingToolBar().margins().right()
        + Ui::DesignSystem::floatingToolBar().shadowMargins().right();
    const qreal additionalWidth = [allActions] {
        qreal width = 0.0;
        for (const auto action : allActions) {
            if (!action->isVisible()) {
                continue;
            }

            if (action->isSeparator()) {
                width -= Ui::DesignSystem::floatingToolBar().iconSize().width()
                    + Ui::DesignSystem::floatingToolBar().spacing();
            } else if (!action->property(kActionWidthKey).isNull()) {
                width += action->property(kActionWidthKey).toReal();
                width -= Ui::DesignSystem::floatingToolBar().iconSize().width();
            }
        }
        return width;
    }();
    const qreal height = Ui::DesignSystem::floatingToolBar().shadowMargins().top()
        + Ui::DesignSystem::floatingToolBar().height()
        + Ui::DesignSystem::floatingToolBar().shadowMargins().bottom();
    if (d->orientation == Qt::Horizontal) {
        return QSize(static_cast<int>(width + additionalWidth), static_cast<int>(height));
    } else {
        return QSize(static_cast<int>(height), static_cast<int>(width));
    }
}

QColor FloatingToolBar::actionColor(QAction* _action) const
{
    if (!actions().contains(_action)) {
        return {};
    }

    return d->actionToColor.value(_action);
}

void FloatingToolBar::setActionColor(QAction* _action, const QColor& _color)
{
    if (!actions().contains(_action)) {
        return;
    }

    if (_color.isValid()) {
        d->actionToColor[_action] = _color;
    } else {
        d->actionToColor.remove(_action);
    }

    update();
}

bool FloatingToolBar::canAnimateHoverOut() const
{
    return true;
}

void FloatingToolBar::animateHoverOut()
{
    if (canAnimateHoverOut()) {
        d->animateHoverOut();
    }
}

bool FloatingToolBar::event(QEvent* _event)
{
    if (_event->type() == QEvent::ToolTip) {
        QHelpEvent* event = static_cast<QHelpEvent*>(_event);
        QAction* action = d->actionAt(event->pos());
        if (action != nullptr && action->toolTip() != action->iconText()) {
            QToolTip::showText(event->globalPos(), action->toolTip());
        } else {
            QToolTip::hideText();
        }
        return true;
    }

    return Widget::event(_event);
}

void FloatingToolBar::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(opacity());

    const QRect backgroundRect
        = rect().marginsRemoved(Ui::DesignSystem::floatingToolBar().shadowMargins().toMargins());
    if (!backgroundRect.isValid()) {
        return;
    }

    //
    // Заливаем фон
    //
    QPixmap backgroundImage(backgroundRect.size());
    backgroundImage.fill(Qt::transparent);
    QPainter backgroundImagePainter(&backgroundImage);
    backgroundImagePainter.setPen(Qt::NoPen);
    backgroundImagePainter.setBrush(backgroundColor());
    qreal radius = 0.0;
    {
        const auto rect = QRect({ 0, 0 }, backgroundImage.size());
        if (d->isCurtain) {
            radius = Ui::DesignSystem::layout().px8();
            backgroundImagePainter.drawRoundedRect(rect, radius, radius);
            backgroundImagePainter.fillRect(rect.adjusted(0, 0, 0, radius), painter.brush());
        } else {
            radius = Ui::DesignSystem::floatingToolBar().height() / 2.0;
            backgroundImagePainter.drawRoundedRect(rect, radius, radius);
        }
    }
    //
    // ... рисуем тень
    //
    const qreal shadowBlurRadius
        = std::max(Ui::DesignSystem::floatingToolBar().minimumShadowBlurRadius(),
                   d->shadowBlurRadiusAnimation.currentValue().toReal());
    const QPixmap shadow = ImageHelper::dropShadow(
        backgroundImage, Ui::DesignSystem::floatingToolBar().shadowMargins(), shadowBlurRadius,
        Ui::DesignSystem::color().shadow());
    painter.drawPixmap(0, 0, shadow);
    //
    // ... рисуем сам фон
    //
    painter.setPen(Qt::NoPen);
    painter.setBrush(backgroundColor());
    if (d->isCurtain) {
        const auto topRect = backgroundRect.adjusted(0, 0, 0, -radius);
        painter.fillRect(topRect, painter.brush());
        const auto bottomRect = backgroundRect.adjusted(0, topRect.height(), 0, 0);
        painter.setClipRect(bottomRect);
        painter.drawRoundedRect(backgroundRect, radius, radius);
        painter.setClipRect(QRectF(), Qt::NoClip);
    } else {
        painter.drawRoundedRect(backgroundRect, radius, radius);
    }

    if (actions().isEmpty()) {
        return;
    }

    //
    // Рисуем иконки
    //
    qreal actionIconX = isLeftToRight() || d->orientation == Qt::Vertical
        ? Ui::DesignSystem::floatingToolBar().shadowMargins().left()
            + Ui::DesignSystem::floatingToolBar().margins().left()
        : width() - d->actionWidth(actions().constFirst())
            - Ui::DesignSystem::floatingToolBar().margins().right()
            - Ui::DesignSystem::floatingToolBar().shadowMargins().right();
    const qreal actionIconY = Ui::DesignSystem::floatingToolBar().shadowMargins().top()
        + Ui::DesignSystem::floatingToolBar().margins().top();
    const QSizeF actionIconSize = Ui::DesignSystem::floatingToolBar().iconSize();
    for (const QAction* action : actions()) {
        if (!action->isVisible()) {
            continue;
        }

        //
        // Рисуем разделитель
        //
        if (action->isSeparator()) {
            const auto separatorX = isLeftToRight() || d->orientation == Qt::Vertical
                ? actionIconX - Ui::DesignSystem::floatingToolBar().spacing() / 2.0
                : actionIconX + actionIconSize.width()
                    + Ui::DesignSystem::floatingToolBar().spacing() / 2.0;
            painter.setPen(
                QPen(ColorHelper::transparent(textColor(), Ui::DesignSystem::disabledTextOpacity()),
                     Ui::DesignSystem::scaleFactor()));
            painter.drawLine(separatorX, actionIconY, separatorX,
                             actionIconSize.height() + actionIconY);
            continue;
        }

        //
        // Настроим цвет отрисовки действия
        //
        const QColor penColor = [this, action] {
            if (d->actionToColor.contains(action)) {
                return d->actionToColor.value(action);
            }

            return ColorHelper::transparent(
                action->isChecked() ? Ui::DesignSystem::color().secondary() : textColor(),
                action->isEnabled() ? 1.0 : Ui::DesignSystem::disabledTextOpacity());
        }();
        painter.setPen(penColor);

        //
        // Рисуем действие с кастомной шириной
        //
        if (!action->property(kActionWidthKey).isNull()) {
            painter.setFont(Ui::DesignSystem::font().subtitle2());
            const QRectF actionRect = d->orientation == Qt::Horizontal
                ? QRectF(actionIconX
                             - (isLeftToRight()
                                    ? 0.0
                                    : (action->property(kActionWidthKey).toReal()
                                       - Ui::DesignSystem::floatingToolBar().spacing())),
                         actionIconY, action->property(kActionWidthKey).toReal(),
                         actionIconSize.height())
                : QRectF(actionIconY, actionIconX, actionIconSize.width(),
                         action->property(kActionWidthKey).toReal());
            if (!backgroundRect.contains(actionRect.toRect(), true)) {
                continue;
            }

            painter.drawText(actionRect, Qt::AlignLeft | Qt::AlignVCenter, action->text());
            //
            // Если есть и текст и иконка, рисуем ещё и иконку
            //
            if (action->iconText() != action->text()) {
                painter.setFont(Ui::DesignSystem::font().iconsMid());
                painter.drawText(actionRect, Qt::AlignRight | Qt::AlignVCenter, action->iconText());
            }

            actionIconX += (isLeftToRight() || d->orientation == Qt::Vertical ? 1 : -1)
                * (actionRect.width() + Ui::DesignSystem::floatingToolBar().spacing());
        }
        //
        // Рисуем действие с одной лишь иконкой
        //
        else {
            //
            // ... сама иконка
            //
            const QRectF actionRect = d->orientation == Qt::Horizontal
                ? QRectF(QPointF(actionIconX, actionIconY), actionIconSize)
                : QRectF(QPointF(actionIconY, actionIconX), actionIconSize);
            if (!backgroundRect.contains(actionRect.toRect(), true)) {
                continue;
            }

            painter.setFont(Ui::DesignSystem::font().iconsMid());
            painter.drawText(actionRect, Qt::AlignCenter,
                             IconHelper::directedIcon(action->iconText()));

            //
            // ... декорация
            //
            if (action == d->lastPressedAction
                && (d->decorationRadiusAnimation.state() == QVariantAnimation::Running
                    || d->decorationOpacityAnimation.state() == QVariantAnimation::Running)) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(Ui::DesignSystem::color().secondary());
                painter.setOpacity(d->decorationOpacityAnimation.currentValue().toReal());
                painter.drawEllipse(actionRect.center(),
                                    d->decorationRadiusAnimation.currentValue().toReal(),
                                    d->decorationRadiusAnimation.currentValue().toReal());
                painter.setOpacity(1.0);
            }

            actionIconX += (isLeftToRight() || d->orientation == Qt::Vertical ? 1 : -1)
                * (actionRect.width() + Ui::DesignSystem::floatingToolBar().spacing());
        }
    }
}

void FloatingToolBar::showEvent(QShowEvent* _event)
{
    Widget::showEvent(_event);

    if (geometry().contains(mapFromGlobal(QCursor::pos()))) {
        d->shadowBlurRadiusAnimation.setDirection(QVariantAnimation::Forward);
        d->shadowBlurRadiusAnimation.start();
        d->shadowBlurRadiusAnimation.setCurrentTime(d->shadowBlurRadiusAnimation.duration());
    }
}

#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
void FloatingToolBar::enterEvent(QEnterEvent* _event)
#else
void FloatingToolBar::enterEvent(QEvent* _event)
#endif
{
    Q_UNUSED(_event)
    d->animateHoverIn();
}

void FloatingToolBar::leaveEvent(QEvent* _event)
{
    Q_UNUSED(_event)
    animateHoverOut();
}

void FloatingToolBar::mousePressEvent(QMouseEvent* _event)
{
    QAction* pressedAction = d->actionAt(_event->pos());
    if (pressedAction == nullptr) {
        return;
    }

    if (!pressedAction->isEnabled()) {
        return;
    }

    d->lastPressedAction = pressedAction;
    d->animateClick();
}

void FloatingToolBar::mouseReleaseEvent(QMouseEvent* _event)
{
    QAction* pressedAction = d->actionAt(_event->pos());
    if (pressedAction == nullptr || pressedAction != d->lastPressedAction) {
        return;
    }

    if (!pressedAction->isEnabled()) {
        return;
    }

    if (!pressedAction->isCheckable()) {
        pressedAction->trigger();
        return;
    }

    pressedAction->toggle();
    update();
}

void FloatingToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    d->decorationRadiusAnimation.setStartValue(
        Ui::DesignSystem::floatingToolBar().iconSize().height() / 2.0);
    d->decorationRadiusAnimation.setEndValue(Ui::DesignSystem::floatingToolBar().height() / 2.5);
    d->shadowBlurRadiusAnimation.setStartValue(
        Ui::DesignSystem::floatingToolBar().minimumShadowBlurRadius());
    d->shadowBlurRadiusAnimation.setEndValue(
        Ui::DesignSystem::floatingToolBar().maximumShadowBlurRadius());

    updateGeometry();
    update();
}
