#include "context_menu.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QVariantAnimation>
#include <QWidgetAction>


class ContextMenu::Implementation
{
public:
    Implementation();

    /**
     * @brief Определить идеальный размер меню
     */
    QSize sizeHint(const QList<QAction*>& _actions) const;

    /**
     * @brief Перерасположить виджеты действий
     */
    void relayoutWidgetActions(ContextMenu* _menu) const;

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


    /**
     * @brief Анимирование геометрии
     */
    QVariantAnimation positionAnimation;
    QVariantAnimation sizeAnimation;

    /**
     * @brief  Декорации кнопки при клике
     */
    QPointF decorationCenterPosition;
    QRectF decorationRect;
    QVariantAnimation decorationRadiusAnimation;
    QVariantAnimation decorationOpacityAnimation;
};

ContextMenu::Implementation::Implementation()
{
    positionAnimation.setEasingCurve(QEasingCurve::OutQuint);
    //
    sizeAnimation.setEasingCurve(QEasingCurve::OutQuint);
    sizeAnimation.setStartValue(QSize(1, 1));

    decorationRadiusAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationRadiusAnimation.setStartValue(1.0);
    decorationRadiusAnimation.setDuration(240);
    //
    decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationOpacityAnimation.setStartValue(0.5);
    decorationOpacityAnimation.setEndValue(0.0);
    decorationOpacityAnimation.setDuration(420);
}

QSize ContextMenu::Implementation::sizeHint(const QList<QAction*>& _actions) const
{
    auto width = 0.0;
    auto height = Ui::DesignSystem::card().shadowMargins().top();
    for (auto action : _actions) {
        if (!action->isVisible()) {
            continue;
        }

        if (auto widgetAction = qobject_cast<QWidgetAction*>(action)) {
            const auto widgetSizeHint = widgetAction->defaultWidget()->sizeHint();
            width = std::max(width, static_cast<qreal>(widgetSizeHint.width()));
            height += widgetSizeHint.height();
        } else {
            if (action->isSeparator()) {
                height += Ui::DesignSystem::drawer().separatorSpacing() * 2;
            }

            //
            // TODO: учитывать ширину шортката
            //
            const auto iconWidth = action->iconText() == action->text()
                ? 0.0
                : Ui::DesignSystem::treeOneLineItem().iconSize().width();
            const auto actionWidth = Ui::DesignSystem::treeOneLineItem().margins().left()
                + iconWidth + Ui::DesignSystem::treeOneLineItem().spacing()
                + TextHelper::fineTextWidthF(action->text(), Ui::DesignSystem::font().subtitle2())
                + Ui::DesignSystem::treeOneLineItem().margins().right();
            width = std::max(width, actionWidth);
            height += Ui::DesignSystem::treeOneLineItem().height();
        }
    }
    width += Ui::DesignSystem::card().shadowMargins().left() + Ui::DesignSystem::layout().px62()
        + Ui::DesignSystem::card().shadowMargins().right();
    height += Ui::DesignSystem::card().shadowMargins().bottom()
        + Ui::DesignSystem::contextMenu().margins().top();

    return QSizeF(width, height).toSize();
}

void ContextMenu::Implementation::relayoutWidgetActions(ContextMenu* _menu) const
{
    auto y = Ui::DesignSystem::card().shadowMargins().top();
    for (auto action : _menu->actions()) {
        if (auto widgetAction = qobject_cast<QWidgetAction*>(action)) {
            auto widget = widgetAction->defaultWidget();
            widget->setParent(_menu);
            widget->move(0, y);
            const auto widgetSizeHint = widgetAction->defaultWidget()->sizeHint();
            widget->resize(_menu->width(), widgetSizeHint.height());
            y += widgetSizeHint.height();
        } else {
            y += Ui::DesignSystem::treeOneLineItem().height();
        }
    }
}

QAction* ContextMenu::Implementation::pressedAction(const QPoint& _coordinate,
                                                    const QList<QAction*>& _actions) const
{
    auto actionTop = Ui::DesignSystem::card().shadowMargins().top()
        + Ui::DesignSystem::contextMenu().margins().top();
    for (auto action : _actions) {
        if (!action->isVisible()) {
            continue;
        }

        if (action->isSeparator()) {
            actionTop += Ui::DesignSystem::drawer().separatorSpacing() * 2;
        }

        auto actionBottom = actionTop;
        if (auto widgetAction = qobject_cast<QWidgetAction*>(action)) {
            actionBottom += widgetAction->defaultWidget()->sizeHint().height();
        } else {
            actionBottom += Ui::DesignSystem::treeOneLineItem().height();
        }
        if (actionTop < _coordinate.y() && _coordinate.y() < actionBottom) {
            return action;
        }

        actionTop = actionBottom;
    }

    return nullptr;
}

QRectF ContextMenu::Implementation::actionRect(const QAction* _action,
                                               const QList<QAction*>& _actions, int _width) const
{
    auto actionTop = Ui::DesignSystem::card().shadowMargins().top()
        + Ui::DesignSystem::contextMenu().margins().top();
    for (auto action : _actions) {
        if (!action->isVisible()) {
            continue;
        }

        if (action->isSeparator()) {
            actionTop += Ui::DesignSystem::drawer().separatorSpacing() * 2;
        }

        auto actionBottom = actionTop;
        if (auto widgetAction = qobject_cast<QWidgetAction*>(action)) {
            actionBottom += widgetAction->defaultWidget()->sizeHint().height();
        } else {
            actionBottom += Ui::DesignSystem::treeOneLineItem().height();
        }
        if (action == _action) {
            return QRectF(Ui::DesignSystem::card().shadowMargins().left(), actionTop,
                          _width - Ui::DesignSystem::card().shadowMargins().left()
                              - Ui::DesignSystem::card().shadowMargins().right(),
                          actionBottom - actionTop);
        }

        actionTop = actionBottom;
    }

    return {};
}

void ContextMenu::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}


// ****

ContextMenu::ContextMenu(QWidget* _parent)
    : Card(_parent)
    , d(new Implementation)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_Hover, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    hide();

    connect(&d->positionAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { move(_value.toPoint()); });
    connect(&d->sizeAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { resize(_value.toSize()); });
    connect(&d->sizeAnimation, &QVariantAnimation::finished, this, [this] {
        //
        // После завершении анимации скрытия скрываем сам виджет контекстного меню
        //
        if (d->sizeAnimation.direction() == QAbstractAnimation::Backward) {
            hide();
        }
    });
    //
    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });
}

ContextMenu::~ContextMenu() = default;

void ContextMenu::setActions(const QVector<QAction*>& _actions)
{
    while (!actions().isEmpty()) {
        removeAction(actions().constFirst());
    }

    addActions(_actions.toList());
}

void ContextMenu::showContextMenu(const QPoint& _pos)
{
    if (actions().isEmpty()) {
        return;
    }

    //
    // Позиционируем все вложенные виджеты-действия
    //
    d->relayoutWidgetActions(this);

    //
    // Настраиваем анимацию отображения
    //
    d->sizeAnimation.stop();
    d->sizeAnimation.setDirection(QVariantAnimation::Forward);
    d->sizeAnimation.setDuration(240);
    const auto sizeHint = d->sizeHint(actions());
    d->sizeAnimation.setEndValue(sizeHint);
    resize(1, 1);
    //
    d->positionAnimation.stop();
    d->positionAnimation.setDirection(QVariantAnimation::Forward);
    d->positionAnimation.setDuration(240);
    auto position = _pos
        - QPointF(Ui::DesignSystem::card().shadowMargins().left(),
                  Ui::DesignSystem::card().shadowMargins().top());
    auto endPosition = position;
    const auto screen = QApplication::screenAt(position.toPoint());
    if (screen == nullptr) {
        return;
    }
    const auto screenGeometry = screen->geometry();
    //
    // Если контекстное меню не помещается на экране справа от указателя
    //
    if (endPosition.x() + sizeHint.width() > screenGeometry.right()) {
        position.setX(position.x() - this->width());
        endPosition.setX(endPosition.x() - sizeHint.width()
                         + Ui::DesignSystem::card().shadowMargins().left()
                         + Ui::DesignSystem::card().shadowMargins().right());
    }
    //
    // Если контекстное меню не помещается на экране снизу от указателя
    //
    if (endPosition.y() + sizeHint.height() > screenGeometry.bottom()) {
        position.setY(position.y() - this->height());
        endPosition.setY(endPosition.y() - sizeHint.height()
                         + Ui::DesignSystem::card().shadowMargins().top()
                         + Ui::DesignSystem::card().shadowMargins().bottom());
    }
    d->positionAnimation.setStartValue(position);
    d->positionAnimation.setEndValue(endPosition);
    move(position.toPoint());

    //
    // NOTE: Если сделать размер 1х1, то на Windows будет моргать окошечко при появлении
    //

    //
    // Отображаем контекстное меню
    //
    show();

    d->positionAnimation.start();
    d->sizeAnimation.start();
}

void ContextMenu::hideContextMenu()
{
    d->positionAnimation.setDirection(QVariantAnimation::Backward);
    d->positionAnimation.setDuration(60);
    d->positionAnimation.start();
    d->sizeAnimation.setDirection(QVariantAnimation::Backward);
    d->sizeAnimation.setDuration(60);
    d->sizeAnimation.start();
}

void ContextMenu::paintEvent(QPaintEvent* _event)
{
    //
    // Рисуем карточку
    //
    Card::paintEvent(_event);

    //
    // Рисуем пункты меню
    //

    QPainter painter(this);

    //
    // Рисуем пункты меню
    //
    const auto actionX = Ui::DesignSystem::card().shadowMargins().left();
    const auto actionWidth = width() - Ui::DesignSystem::card().shadowMargins().left()
        - Ui::DesignSystem::card().shadowMargins().right();
    auto actionY = Ui::DesignSystem::card().shadowMargins().top()
        + Ui::DesignSystem::contextMenu().margins().top();
    ;
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
            painter.drawLine(QPointF(actionX, actionY), QPointF(actionX + actionWidth, actionY));

            actionY += Ui::DesignSystem::drawer().separatorSpacing();
        }

        //
        // ... фон
        //
        const QRectF actionRect(actionX, actionY, actionWidth,
                                Ui::DesignSystem::treeOneLineItem().height());
        if (action->isEnabled() && actionRect.contains(mapFromGlobal(QCursor::pos()))) {
            painter.fillRect(
                actionRect,
                ColorHelper::transparent(textColor(), Ui::DesignSystem::hoverBackgroundOpacity()));
        }

        painter.setPen(action->isChecked() ? Ui::DesignSystem::color().secondary() : textColor());
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
        QRectF iconRect;
        if (action->iconText() != action->text()) {
            iconRect
                = QRectF(QPointF(actionX + Ui::DesignSystem::treeOneLineItem().margins().left(),
                                 actionRect.top()),
                         QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                                actionRect.height()));
            auto it = action->iconText(), t = action->text();
            painter.setFont(Ui::DesignSystem::font().iconsMid());
            painter.drawText(iconRect, Qt::AlignCenter, action->iconText());
        }
        //
        // ... текст
        //
        painter.setFont(Ui::DesignSystem::font().subtitle2());
        const auto textX = iconRect.isEmpty()
            ? actionX + Ui::DesignSystem::treeOneLineItem().margins().left()
            : iconRect.right() + Ui::DesignSystem::treeOneLineItem().spacing();
        const QRectF textRect(textX, actionRect.top(), actionWidth - textX, actionRect.height());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, action->text());

        //
        // ... горячие клавиши
        //
        if (!action->whatsThis().isEmpty()) {
            painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, action->whatsThis());
        }

        actionY += Ui::DesignSystem::treeOneLineItem().height();
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
        painter.drawEllipse(d->decorationCenterPosition,
                            d->decorationRadiusAnimation.currentValue().toReal(),
                            d->decorationRadiusAnimation.currentValue().toReal());
        painter.setOpacity(1.0);
        painter.setClipRect(QRect(), Qt::NoClip);
    }
}

void ContextMenu::mousePressEvent(QMouseEvent* _event)
{
    if (!rect().contains(_event->pos())) {
        hideContextMenu();
        return;
    }

    QAction* pressedAction = d->pressedAction(_event->pos(), actions());
    if (pressedAction == nullptr || !pressedAction->isEnabled()) {
        return;
    }

    d->decorationCenterPosition = _event->pos();
    d->decorationRect = d->actionRect(pressedAction, actions(), width());
    d->decorationRadiusAnimation.setEndValue(d->decorationRect.width());
    d->animateClick();
}

void ContextMenu::mouseReleaseEvent(QMouseEvent* _event)
{
    if (d->sizeAnimation.state() == QVariantAnimation::Running) {
        return;
    }

    if (!rect().contains(_event->pos())) {
        return;
    }

    QAction* pressedAction = d->pressedAction(_event->pos(), actions());
    if (pressedAction == nullptr || !pressedAction->isEnabled() || pressedAction->isChecked()) {
        return;
    }

    pressedAction->trigger();
    hideContextMenu();
}

void ContextMenu::mouseMoveEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event)
    update();
}
