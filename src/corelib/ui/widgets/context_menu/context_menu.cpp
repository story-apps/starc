#include "context_menu.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/icon_helper.h>
#include <utils/helpers/text_helper.h>

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QToolTip>
#include <QVariantAnimation>
#include <QWidgetAction>


class ContextMenu::Implementation
{
public:
    explicit Implementation(ContextMenu* _q);

    /**
     * @brief Определить полный/идеальный размер меню
     */
    QSize fullSize() const;
    QSize sizeHint() const;

    /**
     * @brief Перерасположить виджеты действий
     */
    void relayoutWidgetActions() const;

    /**
     * @brief Получить пункт меню по координате
     */
    QAction* actionForPosition(const QPoint& _coordinate) const;

    /**
     * @brief Определить область действия
     */
    QRectF actionRect(const QAction* _action) const;

    /**
     * @brief Обработать движение мышью
     */
    void processMouseMove(const QPoint& _pos);

    /**
     * @brief Анимировать клик
     */
    void animateClick();


    ContextMenu* q = nullptr;

    /**
     * @brief Полоса прокрутки
     */
    ScrollBar* scrollBar = nullptr;

    /**
     * @brief Действие над которым сейчас мышка
     */
    QAction* hoveredAction = nullptr;

    /**
     * @brief Вложенные менюшки
     */
    QHash<QAction*, ContextMenu*> actionToSubmenu;

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

ContextMenu::Implementation::Implementation(ContextMenu* _q)
    : q(_q)
    , scrollBar(new ScrollBar(_q))
{
    scrollBar->hide();

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

QSize ContextMenu::Implementation::fullSize() const
{
    auto width = 0.0;
    auto height = Ui::DesignSystem::card().shadowMargins().top()
        + Ui::DesignSystem::contextMenu().margins().top();
    for (auto action : q->actions()) {
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

            const auto iconWidth = action->iconText() == action->text()
                ? 0.0
                : Ui::DesignSystem::treeOneLineItem().iconSize().width()
                    + Ui::DesignSystem::treeOneLineItem().spacing();
            const auto actionWidth = Ui::DesignSystem::treeOneLineItem().margins().left()
                + iconWidth + Ui::DesignSystem::treeOneLineItem().spacing()
                + TextHelper::fineTextWidthF(action->text(), Ui::DesignSystem::font().subtitle2())
                + Ui::DesignSystem::layout().px12()
                + TextHelper::fineTextWidthF(action->whatsThis(),
                                             Ui::DesignSystem::font().subtitle2())
                + Ui::DesignSystem::treeOneLineItem().margins().right();
            width = std::max(width, actionWidth);
            height += Ui::DesignSystem::treeOneLineItem().height();
        }
    }
    width += Ui::DesignSystem::card().shadowMargins().left()
        + Ui::DesignSystem::card().shadowMargins().right();
    height += Ui::DesignSystem::card().shadowMargins().bottom()
        + Ui::DesignSystem::contextMenu().margins().top();

    return QSizeF(width, height).toSize();
}

QSize ContextMenu::Implementation::sizeHint() const
{
    auto size = fullSize();
    const auto screenHeight = q->screen()->size().height();
    if (size.height() > screenHeight) {
        size.setHeight(screenHeight);
    }
    return size;
}

void ContextMenu::Implementation::relayoutWidgetActions() const
{
    auto y = Ui::DesignSystem::card().shadowMargins().top()
        + Ui::DesignSystem::contextMenu().margins().top() - scrollBar->value();
    for (auto action : q->actions()) {
        if (auto widgetAction = qobject_cast<QWidgetAction*>(action)) {
            auto widget = widgetAction->defaultWidget();
            widget->setParent(q);
            widget->move(Ui::DesignSystem::card().shadowMargins().left(), y);
            widget->resize(widget->sizeHint());
            widget->setVisible(true);
            widget->installEventFilter(q);
            y += widget->height();
        } else {
            y += Ui::DesignSystem::treeOneLineItem().height();
        }
    }
}

QAction* ContextMenu::Implementation::actionForPosition(const QPoint& _coordinate) const
{
    auto actionTop = Ui::DesignSystem::card().shadowMargins().top()
        + Ui::DesignSystem::contextMenu().margins().top() - scrollBar->value();
    for (auto action : q->actions()) {
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

QRectF ContextMenu::Implementation::actionRect(const QAction* _action) const
{
    auto actionTop = Ui::DesignSystem::card().shadowMargins().top()
        + Ui::DesignSystem::contextMenu().margins().top() - scrollBar->value();
    for (auto action : q->actions()) {
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
                          q->width() - Ui::DesignSystem::card().shadowMargins().left()
                              - Ui::DesignSystem::card().shadowMargins().right(),
                          actionBottom - actionTop);
        }

        actionTop = actionBottom;
    }

    return {};
}

void ContextMenu::Implementation::processMouseMove(const QPoint& _pos)
{
    auto hideSubmenu = [this] {
        if (actionToSubmenu.contains(hoveredAction)) {
            actionToSubmenu[hoveredAction]->hideContextMenu();
        }
    };

    if (!q->rect().contains(_pos)) {
        hideSubmenu();
        hoveredAction = nullptr;
        q->update();
        return;
    }

    auto newHoveredAction = actionForPosition(_pos);
    if (newHoveredAction == hoveredAction) {
        return;
    }

    hideSubmenu();
    hoveredAction = newHoveredAction;
    if (hoveredAction != nullptr) {
        hoveredAction->hover();
    }
    q->update();
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
    , d(new Implementation(this))
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_Hover, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    hide();

    connect(d->scrollBar, &ScrollBar::valueChanged, this, qOverload<>(&ContextMenu::update));
    //
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

ContextMenu::~ContextMenu()
{
    while (!actions().isEmpty()) {
        auto action = actions().constFirst();
        if (d->actionToSubmenu.contains(action)) {
            d->actionToSubmenu[action]->deleteLater();
            d->actionToSubmenu.remove(action);
        }

        removeAction(action);
    }
}

void ContextMenu::setActions(const QVector<QAction*>& _actions)
{
    //
    // Удаляем старые
    //
    while (!actions().isEmpty()) {
        auto action = actions().constFirst();
        if (d->actionToSubmenu.contains(action)) {
            d->actionToSubmenu[action]->deleteLater();
            d->actionToSubmenu.remove(action);
        }

        removeAction(action);
    }

    //
    // Сохраняем новые
    //
    for (auto* action : _actions) {
        addAction(action);

        const auto subactions
            = action->findChildren<QAction*>(QString(), Qt::FindDirectChildrenOnly);
        if (!subactions.isEmpty()) {
            auto submenu = new ContextMenu(this);
            submenu->installEventFilter(this);
            submenu->setBackgroundColor(backgroundColor());
            submenu->setTextColor(textColor());
            submenu->setActions(subactions.toVector());
            d->actionToSubmenu.insert(action, submenu);

            connect(action, &QAction::hovered, submenu, [this] {
                Q_ASSERT(d->hoveredAction);
                const auto actionRect = d->actionRect(d->hoveredAction);
                const auto submenu = d->actionToSubmenu[d->hoveredAction];
                //
                // Размещаем подменю красиво
                //
                const auto submenuSizeHint = submenu->d->sizeHint();
                auto submenuPosition = mapToGlobal(actionRect.topRight().toPoint());
                submenuPosition += QPoint(0, -Ui::DesignSystem::contextMenu().margins().top());
                const auto screen = QApplication::screenAt(submenuPosition);
                Q_ASSERT(screen);
                const auto screenGeometry = screen->geometry();
                if (isRightToLeft()) {
                    //
                    // Обычный случай, когда всё влезает
                    //
                    if (submenuPosition.x() - submenuSizeHint.width() - actionRect.width() > 0) {
                        submenuPosition.setX(submenuPosition.x() - actionRect.width());
                    }
                    //
                    // ... не влезаем справа
                    //
                    else {
                        submenuPosition.setX(submenuPosition.x() + submenuSizeHint.width()
                                             - Ui::DesignSystem::card().shadowMargins().left()
                                             - Ui::DesignSystem::card().shadowMargins().right());
                    }
                } else {
                    //
                    // ... обработать случай, если не влезаем по ширине, а также для RTL
                    //
                    if (submenuPosition.x() + submenuSizeHint.width() > screenGeometry.right()) {
                        submenuPosition.setX(submenuPosition.x() - actionRect.width()
                                             - submenuSizeHint.width()
                                             + Ui::DesignSystem::card().shadowMargins().left()
                                             + Ui::DesignSystem::card().shadowMargins().right());
                    }
                }
                //
                // ... обработать случай, если не влезаем по высоте
                //
                if (submenuPosition.y() + submenuSizeHint.height() > screenGeometry.bottom()) {
                    submenuPosition.setY(screenGeometry.bottom() - submenuSizeHint.height());
                }

                submenu->showContextMenu(submenuPosition);
            });
        }
    }
}

void ContextMenu::showContextMenu(const QPoint& _pos)
{
    if (isVisible()) {
        return;
    }

    if (actions().isEmpty()) {
        return;
    }

    //
    // Позиционируем все вложенные виджеты-действия
    //
    d->relayoutWidgetActions();

    //
    // Настраиваем анимацию отображения
    //
    d->sizeAnimation.stop();
    d->sizeAnimation.setDirection(QVariantAnimation::Forward);
    d->sizeAnimation.setDuration(240);
    const auto sizeHint = d->sizeHint();
    d->sizeAnimation.setEndValue(sizeHint);
    resize(1, 1);
    //
    // Настраиваем полосу прокрутки
    //
    if (sizeHint.height() < d->fullSize().height()) {
        d->scrollBar->resize(d->scrollBar->sizeHint().width(),
                             sizeHint.height() - Ui::DesignSystem::card().shadowMargins().top()
                                 - Ui::DesignSystem::card().shadowMargins().bottom());
        d->scrollBar->move(sizeHint.width() - d->scrollBar->width()
                               - Ui::DesignSystem::card().shadowMargins().right(),
                           Ui::DesignSystem::card().shadowMargins().top());
        d->scrollBar->setMaximum(d->fullSize().height() - sizeHint.height());
        d->scrollBar->setPageStep(sizeHint.height());
        d->scrollBar->setSingleStep(sizeHint.height() / 20);
        d->scrollBar->show();
    } else {
        d->scrollBar->hide();
    }
    //
    d->positionAnimation.stop();
    d->positionAnimation.setDirection(QVariantAnimation::Forward);
    d->positionAnimation.setDuration(240);
    auto position = _pos
        - QPointF(Ui::DesignSystem::card().shadowMargins().left(),
                  Ui::DesignSystem::card().shadowMargins().top());
    auto endPosition = position;
    const auto screen = this->screen();
    const auto screenGeometry = screen->geometry();
    //
    // Если контекстное меню не помещается на экране справа от указателя,
    // а также по-умолчанию для RTL компоновки
    //
    if ((endPosition.x() + sizeHint.width() > screenGeometry.right())
        || (isRightToLeft() && (endPosition.x() - sizeHint.width() > 0))) {
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
        endPosition.setY(std::max(endPosition.y() - sizeHint.height()
                                      + Ui::DesignSystem::card().shadowMargins().top()
                                      + Ui::DesignSystem::card().shadowMargins().bottom(),
                                  static_cast<qreal>(screenGeometry.top())));
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
    if ((d->positionAnimation.state() == QVariantAnimation::Running
         && d->positionAnimation.direction() == QVariantAnimation::Backward)
        || !isVisible()) {
        return;
    }

    //
    // Сперва скрываем все подменю
    //
    for (auto submenu : std::as_const(d->actionToSubmenu)) {
        submenu->hideContextMenu();
    }

    //
    // А потом себя
    //
    d->positionAnimation.setDirection(QVariantAnimation::Backward);
    d->positionAnimation.setDuration(60);
    d->positionAnimation.start();
    d->sizeAnimation.setDirection(QVariantAnimation::Backward);
    d->sizeAnimation.setDuration(60);
    d->sizeAnimation.start();
}

void ContextMenu::processBackgroundColorChange()
{
    d->scrollBar->setBackgroundColor(backgroundColor());
    for (auto submenu : std::as_const(d->actionToSubmenu)) {
        submenu->setBackgroundColor(backgroundColor());
    }
}

void ContextMenu::processTextColorChange()
{
    for (auto submenu : std::as_const(d->actionToSubmenu)) {
        submenu->setTextColor(textColor());
    }
}

bool ContextMenu::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
    case QEvent::ToolTip: {
        const auto event = static_cast<QHelpEvent*>(_event);
        const auto action = d->actionForPosition(event->pos());
        if (action != nullptr) {
            QToolTip::showText(event->globalPos(), action->toolTip());
        }
        return true;
    }

    default: {
        return Card::event(_event);
    }
    }
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
        + Ui::DesignSystem::contextMenu().margins().top() - d->scrollBar->value();

    auto configurePen = [&painter](QAction* _action, const QColor& _color) {
        painter.setPen(_color);
        if (_action->isChecked()) {
            painter.setOpacity(1.0);
        } else if (!_action->isEnabled()) {
            painter.setOpacity(Ui::DesignSystem::disabledTextOpacity());
        } else {
            painter.setOpacity(Ui::DesignSystem::inactiveTextOpacity());
        }
    };

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

            QColor separatorColor = textColor();
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
        if (action->isEnabled() && action == d->hoveredAction) {
            painter.fillRect(
                actionRect,
                ColorHelper::transparent(textColor(), Ui::DesignSystem::hoverBackgroundOpacity()));
        }

        //
        // ... иконка
        //
        QRectF iconRect;
        if (action->iconText() != action->text()) {
            iconRect = QRectF(
                QPointF(isLeftToRight()
                            ? (actionX + Ui::DesignSystem::treeOneLineItem().margins().left())
                            : (actionX + actionWidth
                               - Ui::DesignSystem::treeOneLineItem().iconSize().width()
                               - Ui::DesignSystem::treeOneLineItem().margins().right()),
                        actionRect.top()),
                QSizeF(Ui::DesignSystem::treeOneLineItem().iconSize().width(),
                       actionRect.height()));
            configurePen(action,
                         action->isChecked() ? Ui::DesignSystem::color().accent() : textColor());
            painter.setFont(Ui::DesignSystem::font().iconsMid());
            painter.drawText(iconRect, Qt::AlignCenter, action->iconText());
        }
        //
        // ... текст
        //
        configurePen(action, textColor());
        painter.setFont(Ui::DesignSystem::font().subtitle2());
        const auto textX = iconRect.isEmpty()
            ? actionX + Ui::DesignSystem::treeOneLineItem().margins().left()
            : (isLeftToRight() ? (iconRect.right() + Ui::DesignSystem::treeOneLineItem().spacing())
                               : (actionX + Ui::DesignSystem::treeOneLineItem().margins().left()));
        const auto textWidth = actionWidth - textX
            - (isRightToLeft() && !iconRect.isEmpty()
                   ? iconRect.width() + Ui::DesignSystem::treeOneLineItem().spacing()
                   : 0.0);
        const QRectF textRect(textX, actionRect.top(), textWidth, actionRect.height());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, action->text());
        //
        // ... горячие клавиши
        //
        if (!action->whatsThis().isEmpty()) {
            painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, action->whatsThis());
        }
        //
        // ... подменю
        //
        if (!action->children().isEmpty()) {
            painter.setFont(Ui::DesignSystem::font().iconsMid());
            painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, IconHelper::menuRight());
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
        painter.setBrush(Ui::DesignSystem::color().accent());
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

    QAction* pressedAction = d->actionForPosition(_event->pos());
    if (pressedAction == nullptr || !pressedAction->isEnabled()) {
        return;
    }

    d->decorationCenterPosition = _event->pos();
    d->decorationRect = d->actionRect(pressedAction);
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

    QAction* pressedAction = d->actionForPosition(_event->pos());
    if (pressedAction == nullptr || !pressedAction->isEnabled()) {
        return;
    }

    if (qobject_cast<QWidgetAction*>(pressedAction) != nullptr) {
        return;
    }

    if (pressedAction->isCheckable()) {
        pressedAction->toggle();
    } else {
        pressedAction->trigger();
        hideContextMenu();
    }
}

void ContextMenu::mouseMoveEvent(QMouseEvent* _event)
{
    d->processMouseMove(_event->pos());
}

void ContextMenu::leaveEvent(QEvent* _event)
{
    Card::leaveEvent(_event);

    d->processMouseMove(mapFromGlobal(QCursor::pos()));
}

void ContextMenu::wheelEvent(QWheelEvent* _event)
{
    Card::wheelEvent(_event);

    if (d->scrollBar->isVisible()) {
        QCoreApplication::sendEvent(d->scrollBar, _event);
    }
}

bool ContextMenu::eventFilter(QObject* _watched, QEvent* _event)
{
    //
    // Подменю
    //
    if (auto contextMenu = qobject_cast<ContextMenu*>(_watched);
        d->actionToSubmenu.key(contextMenu, nullptr) != nullptr) {
        switch (_event->type()) {
        case QEvent::MouseMove: {
            const auto event = static_cast<QMouseEvent*>(_event);
            const auto mousePos = mapFromGlobal(event->globalPos());
            if (rect().contains(mousePos)) {
                d->processMouseMove(mousePos);
            }
            break;
        }

        //
        // Нажатие вне области подменю
        //
        case QEvent::MouseButtonPress: {
            const auto event = static_cast<QMouseEvent*>(_event);
            if (!contextMenu->rect().contains(event->pos())) {
                QMetaObject::invokeMethod(this, &ContextMenu::hideContextMenu,
                                          Qt::QueuedConnection);
            }
            break;
        }

        //
        // Нажатие на элементе подменю
        //
        case QEvent::MouseButtonRelease: {
            const auto event = static_cast<QMouseEvent*>(_event);
            const auto triggeredAction = contextMenu->d->actionForPosition(event->pos());
            if (triggeredAction != nullptr && !triggeredAction->isCheckable()
                && qobject_cast<const QWidgetAction*>(triggeredAction) == nullptr) {
                QMetaObject::invokeMethod(this, &ContextMenu::hideContextMenu,
                                          Qt::QueuedConnection);
            } else {
                update();
            }
            break;
        }

        default:
            break;
        }
    }
    //
    // Вложенный в действие виджет
    //
    else if (qobject_cast<Widget*>(_watched) != nullptr) {
        if (_event->type() == QEvent::Enter) {
            d->hoveredAction = nullptr;
            update();
        }
    }

    return Card::eventFilter(_watched, _event);
}
