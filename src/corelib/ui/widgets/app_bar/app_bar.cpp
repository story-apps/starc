#include "app_bar.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>

#include <QAction>
#include <QPaintEvent>
#include <QPainter>
#include <QToolTip>
#include <QVariantAnimation>


class AppBar::Implementation
{
public:
    explicit Implementation(AppBar* _q);

    /**
     * @brief Анимировать клик
     */
    void animateClick();

    /**
     * @brief Получить пункт меню по координате
     */
    QAction* pressedAction(const QPoint& _coordinate, const QList<QAction*>& _actions) const;


    /**
     * @brief Владелец данных
     */
    AppBar* q = nullptr;

    /**
     * @brief Иконка опций тулбара
     */
    QAction* optionsAction = nullptr;

    /**
     * @brief Иконка на которой кликнули последней
     */
    QAction* lastPressedAction = nullptr;

    /**
     * @brief  Декорации иконки при клике
     */
    QVariantAnimation decorationRadiusAnimation;
    QVariantAnimation decorationOpacityAnimation;
};

AppBar::Implementation::Implementation(AppBar* _q)
    : q(_q)
{
    decorationRadiusAnimation.setEasingCurve(QEasingCurve::OutQuad);
    decorationRadiusAnimation.setDuration(160);

    decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationOpacityAnimation.setStartValue(0.5);
    decorationOpacityAnimation.setEndValue(0.0);
    decorationOpacityAnimation.setDuration(160);
}

void AppBar::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}

QAction* AppBar::Implementation::pressedAction(const QPoint& _coordinate,
                                               const QList<QAction*>& _actions) const
{
    qreal actionLeft = q->isLeftToRight()
        ? Ui::DesignSystem::appBar().margins().left()
            - (Ui::DesignSystem::appBar().iconsSpacing() / 2.0)
        : q->width() - (Ui::DesignSystem::appBar().iconsSpacing() / 2.0)
            - Ui::DesignSystem::appBar().iconSize().width()
            - Ui::DesignSystem::appBar().margins().left();
    //
    // Берём увеличенный регион для удобства клика в области кнопки
    //
    const qreal actionWidth
        = Ui::DesignSystem::appBar().iconSize().width() + Ui::DesignSystem::appBar().iconsSpacing();
    for (QAction* action : _actions) {
        const qreal actionRight = actionLeft + actionWidth;

        if (actionLeft < _coordinate.x() && _coordinate.x() < actionRight) {
            return action;
        }

        actionLeft = q->isLeftToRight() ? actionRight : actionLeft - actionWidth;
    }

    if (optionsAction != nullptr) {
        actionLeft = q->isLeftToRight()
            ? (q->width() - Ui::DesignSystem::appBar().iconSize().width()
               - Ui::DesignSystem::appBar().margins().right())
            : Ui::DesignSystem::appBar().margins().left();
        const qreal actionRight = actionLeft + actionWidth;
        if (actionLeft < _coordinate.x() && _coordinate.x() < actionRight) {
            return optionsAction;
        }
    }

    return nullptr;
}


// ****


AppBar::AppBar(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });

    designSystemChangeEvent(nullptr);
}

void AppBar::setOptionsAction(QAction* _options)
{
    if (d->optionsAction == _options) {
        return;
    }

    d->optionsAction = _options;
    update();
}

QSize AppBar::minimumSizeHint() const
{
    const int actionsSize = actions().size() + (d->optionsAction != nullptr ? 1 : 0);
    const qreal width = Ui::DesignSystem::appBar().margins().left()
        + Ui::DesignSystem::appBar().iconSize().width() * actionsSize
        + Ui::DesignSystem::appBar().iconsSpacing() * (actionsSize - 1)
        + Ui::DesignSystem::appBar().margins().right();
    const qreal height = Ui::DesignSystem::appBar().margins().top()
        + Ui::DesignSystem::appBar().iconSize().height()
        + Ui::DesignSystem::appBar().margins().bottom();
    return QSizeF(width, height).toSize();
}

AppBar::~AppBar() = default;

bool AppBar::event(QEvent* _event)
{
    if (_event->type() == QEvent::ToolTip) {
        QHelpEvent* event = static_cast<QHelpEvent*>(_event);
        QAction* action = d->pressedAction(event->pos(), actions());
        if (action != nullptr && action->toolTip() != action->iconText()) {
            QToolTip::showText(event->globalPos(), action->toolTip());
        } else {
            QToolTip::hideText();
        }
        return true;
    }

    return Widget::event(_event);
}

void AppBar::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Рисуем иконки
    //
    painter.setFont(Ui::DesignSystem::font().iconsMid());
    qreal actionX = isLeftToRight() ? Ui::DesignSystem::appBar().margins().left()
                                    : width() - Ui::DesignSystem::appBar().iconSize().width()
            - Ui::DesignSystem::appBar().margins().right();
    const qreal actionY = Ui::DesignSystem::appBar().margins().top();
    const QSizeF actionSize = Ui::DesignSystem::appBar().iconSize();
    const QColor iconInactiveColor = ColorHelper::colorBetween(textColor(), backgroundColor());
    auto drawIcon = [this, &painter, &actionX, actionY, actionSize,
                     iconInactiveColor](const QAction* _action) {
        //
        // ... сама иконка
        //
        const QRectF actionRect(QPointF(actionX, actionY), actionSize);
        painter.setPen((!_action->isCheckable() || _action->isChecked()) ? textColor()
                                                                         : iconInactiveColor);
        painter.drawText(actionRect, Qt::AlignCenter, _action->text());

        //
        // ... декорация
        //
        if (_action == d->lastPressedAction
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
    };
    for (const QAction* action : actions()) {
        drawIcon(action);

        actionX += (isLeftToRight() ? 1 : -1)
            * (actionSize.width() + Ui::DesignSystem::appBar().iconsSpacing());
    }

    //
    // Иконка опций панели
    //
    if (d->optionsAction != nullptr) {
        actionX = isLeftToRight() ? (width() - Ui::DesignSystem::appBar().iconSize().width()
                                     - Ui::DesignSystem::appBar().margins().right())
                                  : Ui::DesignSystem::appBar().margins().left();
        drawIcon(d->optionsAction);
    }
}

void AppBar::mousePressEvent(QMouseEvent* _event)
{
    QAction* pressedAction = d->pressedAction(_event->pos(), actions());
    if (pressedAction == nullptr) {
        return;
    }

    d->lastPressedAction = pressedAction;
    if (d->lastPressedAction->text().isEmpty()) {
        return;
    }

    d->animateClick();
}

void AppBar::mouseReleaseEvent(QMouseEvent* _event)
{
    QAction* pressedAction = d->pressedAction(_event->pos(), actions());
    if (pressedAction == nullptr) {
        return;
    }

    if (!pressedAction->isCheckable()) {
        pressedAction->trigger();
        return;
    }

    if (pressedAction->isChecked()) {
        return;
    }

    for (auto action : actions()) {
        if (action->isCheckable() && action != pressedAction) {
            action->setChecked(false);
        }
    }
    pressedAction->setChecked(true);
    update();
}

void AppBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    d->decorationRadiusAnimation.setStartValue(Ui::DesignSystem::appBar().iconSize().height()
                                               / 2.0);
    d->decorationRadiusAnimation.setEndValue(Ui::DesignSystem::appBar().heightRegular() / 2.5);

    updateGeometry();
    update();
}
