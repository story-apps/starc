#include "drawer.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAction>
#include <QPainter>
#include <QMouseEvent>
#include <QVariantAnimation>


class Drawer::Implementation
{
public:
    Implementation();

    /**
     * @brief Получить пункт меню по координате
     */
    QAction* pressedAction(const QPoint& _coordinate, const QList<QAction*>& _actions) const;

    /**
     * @brief Определить область действия
     */
    QRectF actionRect(const QAction* _action, const QList<QAction*>& _actions, int _width) const;

    /**
     * @brief Анимировать клик
     */
    void animateClick();


    QString title;
    QString subtitle;

    /**
     * @brief  Декорации кнопки при клике
     */
    QPointF decorationCenterPosition;
    QRectF decorationRect;
    QVariantAnimation decorationRadiusAnimation;
    QVariantAnimation decorationOpacityAnimation;
};

Drawer::Implementation::Implementation()
{
    decorationRadiusAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationRadiusAnimation.setStartValue(1.0);
    decorationRadiusAnimation.setDuration(240);

    decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationOpacityAnimation.setStartValue(0.5);
    decorationOpacityAnimation.setEndValue(0.0);
    decorationOpacityAnimation.setDuration(420);
}

QAction* Drawer::Implementation::pressedAction(const QPoint& _coordinate, const QList<QAction*>& _actions) const
{
    qreal actionTop = Ui::DesignSystem::drawer().margins().top()
                      + Ui::DesignSystem::drawer().titleHeight()
                      + Ui::DesignSystem::drawer().subtitleHeight()
                      + Ui::DesignSystem::drawer().subtitleBottomMargin();
    for (QAction* action : _actions) {
        if (!action->isVisible()) {
            continue;
        }

        if (action->isSeparator()) {
            actionTop += Ui::DesignSystem::drawer().separatorSpacing() * 2;
        }

        const qreal actionBottom = actionTop + Ui::DesignSystem::drawer().actionHeight();
        if (actionTop < _coordinate.y() && _coordinate.y() < actionBottom) {
            return action;
        }

        actionTop = actionBottom;
    }

    return nullptr;
}

QRectF Drawer::Implementation::actionRect(const QAction* _action, const QList<QAction*>& _actions, int _width) const
{
    qreal actionTop = Ui::DesignSystem::drawer().margins().top()
                      + Ui::DesignSystem::drawer().titleHeight()
                      + Ui::DesignSystem::drawer().subtitleHeight()
                      + Ui::DesignSystem::drawer().subtitleBottomMargin();
    for (QAction* action : _actions) {
        if (!action->isVisible()) {
            continue;
        }

        if (action->isSeparator()) {
            actionTop += Ui::DesignSystem::drawer().separatorSpacing() * 2;
        }

        const qreal actionBottom = actionTop + Ui::DesignSystem::drawer().actionHeight();
        if (action == _action) {
            return QRectF(0.0, actionTop, _width, Ui::DesignSystem::drawer().actionHeight());
        }

        actionTop = actionBottom;
    }

    return {};
}

void Drawer::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}


// ****


Drawer::Drawer(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setFixedWidth(static_cast<int>(Ui::DesignSystem::drawer().width()));

    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
}

Drawer::~Drawer() = default;

void Drawer::setTitle(const QString& _title)
{
    if (d->title == _title) {
        return;
    }

    d->title = _title;
    update();
}

void Drawer::setSubtitle(const QString& _subtitle)
{
    if (d->subtitle == _subtitle) {
        return;
    }

    d->subtitle = _subtitle;
    update();
}

QSize Drawer::sizeHint() const
{
    qreal width = 0.0;
    qreal height = Ui::DesignSystem::drawer().margins().top()
                   + Ui::DesignSystem::drawer().titleHeight()
                   + Ui::DesignSystem::drawer().subtitleHeight()
                   + Ui::DesignSystem::drawer().subtitleBottomMargin();
    for (int actionIndex = 0; actionIndex < actions().size(); ++actionIndex) {
        QAction* action = actions().at(actionIndex);
        if (!action->isVisible()) {
            continue;
        }

        if (action->isSeparator()) {
            height += Ui::DesignSystem::drawer().separatorSpacing() * 2;
        }

        const qreal actionWidth = Ui::DesignSystem::drawer().actionMargins().left()
                                  + Ui::DesignSystem::drawer().iconSize().width()
                                  + Ui::DesignSystem::drawer().spacing()
                                  + TextHelper::fineTextWidth(action->text(), Ui::DesignSystem::font().subtitle2())
                                  + (action->whatsThis().isEmpty()
                                     ? 0.0
                                     : Ui::DesignSystem::drawer().spacing()
                                       + TextHelper::fineTextWidth(action->whatsThis(), Ui::DesignSystem::font().subtitle2()))
                                  + Ui::DesignSystem::drawer().actionMargins().right();
        width = std::max(width, actionWidth);

        height += Ui::DesignSystem::drawer().actionHeight();
    }

    return QSizeF(width, height).toSize();
}

void Drawer::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    QPainter painter(this);

    //
    // Заливаем фон
    //
    painter.fillRect(rect(), Ui::DesignSystem::color().primary());

    //
    // Рисуем заголовок
    //
    painter.setPen(Ui::DesignSystem::color().onPrimary());
    painter.setFont(Ui::DesignSystem::font().h6());
    const QRectF titleRect(Ui::DesignSystem::drawer().margins().left(),
                           Ui::DesignSystem::drawer().margins().top(),
                           width()
                           - Ui::DesignSystem::drawer().margins().left()
                           - Ui::DesignSystem::drawer().margins().right(),
                           Ui::DesignSystem::drawer().titleHeight());
    painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, d->title);
    //
    // ... и подзаголовок
    //
    painter.setFont(Ui::DesignSystem::font().body2());
    painter.setOpacity(Ui::DesignSystem::inactiveTextOpacity());
    const QRectF subtitleRect(titleRect.left(), titleRect.bottom(),
                              titleRect.width(), Ui::DesignSystem::drawer().subtitleHeight());
    painter.drawText(subtitleRect, Qt::AlignLeft | Qt::AlignVCenter, d->subtitle);
    painter.setOpacity(1.0);

    //
    // Рисуем пункты меню
    //
    qreal actionY = subtitleRect.bottom() + Ui::DesignSystem::drawer().subtitleBottomMargin();
    for (int actionIndex = 0; actionIndex < actions().size(); ++actionIndex) {
        QAction* action = actions().at(actionIndex);
        if (!action->isVisible()) {
            continue;
        }

        //
        // ... разделительная полоса сверху
        //
        if (action->isSeparator()) {
            actionY += Ui::DesignSystem::drawer().separatorSpacing();

            QColor separatorColor = Ui::DesignSystem::color().onPrimary();
            separatorColor.setAlphaF(Ui::DesignSystem::disabledTextOpacity());
            painter.setPen(QPen(separatorColor, Ui::DesignSystem::drawer().separatorHeight()));
            painter.drawLine(QPointF(0.0, actionY), QPointF(width(), actionY));

            actionY += Ui::DesignSystem::drawer().separatorSpacing();
        }

        //
        // ... обводка
        //
        const QRectF actionRect(0.0, actionY, width(), Ui::DesignSystem::drawer().actionHeight());
        if (action->isChecked()) {
            painter.fillRect(actionRect.marginsRemoved(Ui::DesignSystem::drawer().selectionMargins()),
                             Ui::DesignSystem::drawer().selectionColor());
        } else if (action->isEnabled()
                   && actionRect.contains(mapFromGlobal(QCursor::pos()))) {
            painter.fillRect(actionRect.marginsRemoved(Ui::DesignSystem::drawer().selectionMargins()),
                             ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                                      Ui::DesignSystem::hoverBackgroundOpacity()));
        }

        painter.setPen(action->isChecked() ? Ui::DesignSystem::color().secondary()
                                           : Ui::DesignSystem::color().onPrimary());
        if (action->isChecked()) {
            painter.setOpacity(1.0);
        } else if (!action->isEnabled()) {
            painter.setOpacity(Ui::DesignSystem::disabledTextOpacity());
        } else {
            painter.setOpacity(Ui::DesignSystem::inactiveTextOpacity());
        }
        //
        // ... иконка
        //
        const QRectF iconRect(QPointF(Ui::DesignSystem::drawer().actionMargins().left(),
                                      actionRect.top()
                                      + Ui::DesignSystem::drawer().actionMargins().top()),
                              Ui::DesignSystem::drawer().iconSize());
        if (action->iconText() != action->text()) {
            auto it = action->iconText(), t = action->text();
            painter.setFont(Ui::DesignSystem::font().iconsMid());
            painter.drawText(iconRect, Qt::AlignCenter, action->iconText());
        }
        //
        // ... текст
        //
        painter.setFont(Ui::DesignSystem::font().subtitle2());
        const QRectF textRect(iconRect.right() + Ui::DesignSystem::drawer().spacing(),
                              iconRect.top(),
                              width()
                              - iconRect.right()
                              - Ui::DesignSystem::drawer().spacing()
                              - Ui::DesignSystem::drawer().actionMargins().right(),
                              Ui::DesignSystem::drawer().actionHeight()
                              - Ui::DesignSystem::drawer().actionMargins().top()
                              - Ui::DesignSystem::drawer().actionMargins().bottom());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, action->text());

        //
        // ... горячие клавиши
        //
        if (!action->whatsThis().isEmpty()) {
            painter.drawText(textRect.adjusted(0, 0, -1 * Ui::DesignSystem::layout().px4(), 0),
                             Qt::AlignRight | Qt::AlignVCenter, action->whatsThis());
        }

        actionY += Ui::DesignSystem::drawer().actionHeight();
    }

    //
    // Если необходимо, рисуем декорацию
    //
    if (d->decorationRadiusAnimation.state() == QVariantAnimation::Running
        || d->decorationOpacityAnimation.state() == QVariantAnimation::Running) {
        painter.setClipRect(d->decorationRect);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Ui::DesignSystem::color().secondary());
        painter.setOpacity(d->decorationOpacityAnimation.currentValue().toReal());
        painter.drawEllipse(d->decorationCenterPosition, d->decorationRadiusAnimation.currentValue().toReal(),
                            d->decorationRadiusAnimation.currentValue().toReal());
        painter.setOpacity(1.0);
        painter.setClipRect(QRect(), Qt::NoClip);
    }
}

void Drawer::mousePressEvent(QMouseEvent* _event)
{
    QAction* pressedAction = d->pressedAction(_event->pos(), actions());
    if (pressedAction == nullptr
        || !pressedAction->isEnabled()) {
        return;
    }

    d->decorationCenterPosition = _event->pos();
    d->decorationRect = d->actionRect(pressedAction, actions(), width());
    d->decorationRadiusAnimation.setEndValue(d->decorationRect.width());
    d->animateClick();
}

void Drawer::mouseReleaseEvent(QMouseEvent* _event)
{
    if (!rect().contains(_event->pos())) {
        return;
    }

    QAction* pressedAction = d->pressedAction(_event->pos(), actions());
    if (pressedAction == nullptr
        || pressedAction->isChecked()
        || !pressedAction->isEnabled()) {
        return;
    }

    pressedAction->trigger();
    update();
}

void Drawer::mouseMoveEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event)
    update();
}
