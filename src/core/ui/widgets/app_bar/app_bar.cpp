#include "app_bar.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/color_helper.h>

#include <QAction>
#include <QPainter>
#include <QPaintEvent>
#include <QVariantAnimation>


class AppBar::Implementation
{
public:
    Implementation();

    /**
     * @brief Анимировать клик
     */
    void animateClick();

    /**
     * @brief Получить пункт меню по координате
     */
    QAction* pressedAction(const QPoint& _coordinate, const QList<QAction*>& _actions) const;


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

AppBar::Implementation::Implementation()
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

QAction* AppBar::Implementation::pressedAction(const QPoint& _coordinate, const QList<QAction*>& _actions) const
{
    qreal actionLeft = Ui::DesignSystem::appBar().margins().left()
                       - (Ui::DesignSystem::appBar().iconsSpacing() / 2.0);
    //
    // Берём увеличенный регион для удобства клика в области кнопки
    //
    const qreal actionWidth = Ui::DesignSystem::appBar().iconSize().width()
                              + Ui::DesignSystem::appBar().iconsSpacing();
    for (QAction* action : _actions) {
        const qreal actionRight = actionLeft + actionWidth;

        if (actionLeft < _coordinate.x() && _coordinate.x() < actionRight) {
            return action;
        }

        actionLeft = actionRight;
    }

    return nullptr;
}


// ****


AppBar::AppBar(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });

    designSystemChangeEvent(nullptr);
}

AppBar::~AppBar() = default;

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
    qreal actionX = Ui::DesignSystem::appBar().margins().left();
    const qreal actionY = Ui::DesignSystem::appBar().margins().top();
    const QSizeF actionSize = Ui::DesignSystem::appBar().iconSize();
    const QColor iconInactiveColor = ColorHelper::colorBetween(textColor(), backgroundColor());
    for (const QAction* action : actions()) {
        //
        // ... сама иконка
        //
        const QRectF actionRect(QPointF(actionX, actionY), actionSize);
        painter.setPen((!action->isCheckable() || action->isChecked()) ? textColor() : iconInactiveColor);
        painter.drawText(actionRect, Qt::AlignCenter, action->text());

        //
        // ... декорация
        //
        if (action == d->lastPressedAction
            && (d->decorationRadiusAnimation.state() == QVariantAnimation::Running
                || d->decorationOpacityAnimation.state() == QVariantAnimation::Running)) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(Ui::DesignSystem::color().secondary());
            painter.setOpacity(d->decorationOpacityAnimation.currentValue().toReal());
            painter.drawEllipse(actionRect.center(), d->decorationRadiusAnimation.currentValue().toReal(),
                                d->decorationRadiusAnimation.currentValue().toReal());
            painter.setOpacity(1.0);
        }

        actionX += actionRect.width() + Ui::DesignSystem::appBar().iconsSpacing();
    }
}

void AppBar::mousePressEvent(QMouseEvent* _event)
{
    QAction* pressedAction = d->pressedAction(_event->pos(), actions());
    if (pressedAction == nullptr) {
        return;
    }

    d->lastPressedAction = pressedAction;
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

    d->decorationRadiusAnimation.setStartValue(Ui::DesignSystem::appBar().iconSize().height() / 2.0);
    d->decorationRadiusAnimation.setEndValue(Ui::DesignSystem::appBar().heightRegular() / 2.5);

    updateGeometry();
    update();
}
