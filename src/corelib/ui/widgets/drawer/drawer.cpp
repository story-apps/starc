#include "drawer.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/shugar.h>

#include <QAction>
#include <QMouseEvent>
#include <QPainter>
#include <QVariantAnimation>


class Drawer::Implementation
{
public:
    Implementation();

    /**
     * @brief Высота панели аккаунта
     */
    qreal accountPanelHeight() const;

    /**
     * @brief Был ли осуществлён клик на панели аккаунта
     */
    bool isAccountPanelClicked(const QPoint& _coordinate) const;

    /**
     * @brief Получить информацию о дополнительном действии аккаунта в заданной координате
     */
    bool isAccountActionClicked(const QPoint& _coordinate, int _width) const;
    QPair<QAction*, QRectF> pressedAccountActionInfo(const QPoint& _coordinate, int _width) const;
    QAction* pressedAccountAction(const QPoint& _coordinate, int _width) const;
    QRectF pressedAccountActionRect(const QPoint& _coordinate, int _width) const;

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
     * @brief Параметры панели информации о пользователе
     */
    bool isAccountVisible = false;
    QPixmap avatar;
    QString accountName;
    QString accountEmail;

    /**
     * @brief Дополнительные действия панели информации о пользователе
     */
    QVector<QAction*> accountActions;

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

qreal Drawer::Implementation::accountPanelHeight() const
{
    if (!isAccountVisible) {
        return Ui::DesignSystem::drawer().margins().top();
    }

    return Ui::DesignSystem::drawer().margins().top() + Ui::DesignSystem::layout().px48() // avatar
        + Ui::DesignSystem::layout().px24() // avatar <-> user name spacing
        + Ui::DesignSystem::layout().px48() // user name + email
        + Ui::DesignSystem::layout().px8(); // bottom spacing
}

bool Drawer::Implementation::isAccountPanelClicked(const QPoint& _coordinate) const
{
    if (!isAccountVisible) {
        return false;
    }

    return _coordinate.y() < accountPanelHeight();
}

bool Drawer::Implementation::isAccountActionClicked(const QPoint& _coordinate, int _width) const
{
    return pressedAccountActionInfo(_coordinate, _width).first != nullptr;
}

QPair<QAction*, QRectF> Drawer::Implementation::pressedAccountActionInfo(const QPoint& _coordinate,
                                                                         int _width) const
{
    if (!isAccountVisible || accountActions.isEmpty()) {
        return {};
    }

    const auto top = Ui::DesignSystem::drawer().margins().top() + Ui::DesignSystem::layout().px2();
    QRectF actionRect(QPointF(_width - Ui::DesignSystem::layout().px48(), top),
                      QPointF(_width - Ui::DesignSystem::layout().px4(),
                              top + Ui::DesignSystem::layout().px24()));

    for (auto action : reversed(accountActions)) {
        if (!action->isVisible()) {
            continue;
        }

        if (actionRect.contains(_coordinate)) {
            return { action, actionRect };
        }

        actionRect.moveRight(actionRect.right() - Ui::DesignSystem::layout().px(52));
    }

    return {};
}

QAction* Drawer::Implementation::pressedAccountAction(const QPoint& _coordinate, int _width) const
{
    return pressedAccountActionInfo(_coordinate, _width).first;
}

QRectF Drawer::Implementation::pressedAccountActionRect(const QPoint& _coordinate, int _width) const
{
    return pressedAccountActionInfo(_coordinate, _width).second;
}

QAction* Drawer::Implementation::pressedAction(const QPoint& _coordinate,
                                               const QList<QAction*>& _actions) const
{
    qreal actionTop = accountPanelHeight();
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

QRectF Drawer::Implementation::actionRect(const QAction* _action, const QList<QAction*>& _actions,
                                          int _width) const
{
    qreal actionTop = accountPanelHeight();
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
    : Widget(_parent)
    , d(new Implementation)
{
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });

    designSystemChangeEvent(nullptr);
}

Drawer::~Drawer() = default;

void Drawer::setAccountVisible(bool _use)
{
    if (d->isAccountVisible == _use) {
        return;
    }

    d->isAccountVisible = _use;
    updateGeometry();
    update();
}

void Drawer::setAvatar(const QPixmap& _avatar)
{
    if (d->avatar.cacheKey() == _avatar.cacheKey()) {
        return;
    }

    d->avatar = _avatar;
    if (d->isAccountVisible) {
        update();
    }
}

void Drawer::setAccountName(const QString& _name)
{
    if (d->accountName == _name) {
        return;
    }

    d->accountName = _name;
    if (d->isAccountVisible) {
        update();
    }
}

void Drawer::setAccountEmail(const QString& _email)
{
    if (d->accountEmail == _email) {
        return;
    }

    d->accountEmail = _email;
    if (d->isAccountVisible) {
        update();
    }
}

void Drawer::setAccountActions(const QVector<QAction*>& _actions)
{
    if (d->accountActions == _actions) {
        return;
    }

    qDeleteAll(d->accountActions);
    d->accountActions = _actions;

    update();
}

QSize Drawer::sizeHint() const
{
    qreal width = 0.0;
    qreal height = d->accountPanelHeight();
    for (int actionIndex = 0; actionIndex < actions().size(); ++actionIndex) {
        QAction* action = actions().at(actionIndex);
        if (!action->isVisible()) {
            continue;
        }

        if (action->isSeparator()) {
            height += Ui::DesignSystem::drawer().separatorSpacing() * 2;
        }

        const qreal actionWidth = Ui::DesignSystem::drawer().actionMargins().left()
            + Ui::DesignSystem::drawer().iconSize().width() + Ui::DesignSystem::drawer().spacing()
            + TextHelper::fineTextWidthF(action->text(), Ui::DesignSystem::font().subtitle2())
            + (action->whatsThis().isEmpty()
                   ? 0.0
                   : (Ui::DesignSystem::drawer().spacing()
                      + TextHelper::fineTextWidthF(action->whatsThis(),
                                                   Ui::DesignSystem::font().subtitle2())))
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

    qreal y = Ui::DesignSystem::drawer().margins().top();

    //
    // Рисуем панель аккаунта
    //
    if (d->isAccountVisible) {
        const auto left
            = Ui::DesignSystem::drawer().margins().left() + Ui::DesignSystem::layout().px2();

        //
        // Аватар
        //
        const QRectF avatarRect(left, y + Ui::DesignSystem::layout().px2(),
                                Ui::DesignSystem::layout().px48(),
                                Ui::DesignSystem::layout().px48());
        if (!d->avatar.isNull()) {
            painter.drawPixmap(avatarRect.topLeft(),
                               ImageHelper::makeAvatar(d->avatar, avatarRect.size().toSize()));
        } else {
            painter.drawPixmap(avatarRect.topLeft(),
                               ImageHelper::makeAvatar(d->accountName,
                                                       Ui::DesignSystem::font().h6(),
                                                       avatarRect.size().toSize(), Qt::white));
        }

        //
        // Имя пользователя
        //
        painter.setPen(Ui::DesignSystem::color().onPrimary());
        painter.setFont(Ui::DesignSystem::font().h6());
        const QRectF userNameRect(left, avatarRect.bottom() + Ui::DesignSystem::layout().px24(),
                                  width(), Ui::DesignSystem::layout().px24());
        painter.drawText(userNameRect, Qt::AlignBottom | Qt::AlignLeft, d->accountName);

        //
        // Имейл пользователя
        //
        painter.setPen(ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                                Ui::DesignSystem::inactiveTextOpacity()));
        painter.setFont(Ui::DesignSystem::font().subtitle1());
        const QRectF userEmailRect(left, userNameRect.bottom(), width(),
                                   Ui::DesignSystem::layout().px24());
        painter.drawText(userEmailRect, Qt::AlignTop | Qt::AlignLeft, d->accountEmail);

        //
        // Иконки
        //
        painter.setFont(Ui::DesignSystem::font().iconsMid());
        //
        // ... дополнительные действия
        //
        {
            QRectF actionRect(
                QPointF(width() - Ui::DesignSystem::layout().px48(), avatarRect.top()),
                QPointF(width() - Ui::DesignSystem::layout().px4(),
                        avatarRect.top() + Ui::DesignSystem::layout().px24()));

            for (auto action : reversed(d->accountActions)) {
                if (!action->isVisible()) {
                    continue;
                }

                painter.drawText(actionRect, Qt::AlignCenter, action->iconText());
                actionRect.moveRight(actionRect.right() - Ui::DesignSystem::layout().px(52));
            }
        }
        //
        // ... иконка перехода в личный кабинет
        //
        painter.setPen(Ui::DesignSystem::color().onPrimary());
        painter.setFont(Ui::DesignSystem::font().iconsMid());
        const QRectF iconRect(
            QPointF(width() - Ui::DesignSystem::layout().px48(), userNameRect.top()),
            QPointF(width(), userEmailRect.bottom()));
        painter.drawText(iconRect, Qt::AlignCenter, u8"\U000F0142");

        y = userEmailRect.bottom() + Ui::DesignSystem::layout().px8();
    }

    //
    // Рисуем пункты меню
    //
    for (int actionIndex = 0; actionIndex < actions().size(); ++actionIndex) {
        QAction* action = actions().at(actionIndex);
        if (!action->isVisible()) {
            continue;
        }

        //
        // ... разделительная полоса сверху
        //
        if (action->isSeparator()) {
            y += Ui::DesignSystem::drawer().separatorSpacing();

            QColor separatorColor = Ui::DesignSystem::color().onPrimary();
            separatorColor.setAlphaF(Ui::DesignSystem::disabledTextOpacity());
            painter.setPen(QPen(separatorColor, Ui::DesignSystem::drawer().separatorHeight()));
            painter.drawLine(QPointF(0.0, y), QPointF(width(), y));

            y += Ui::DesignSystem::drawer().separatorSpacing();
        }

        //
        // ... обводка
        //
        const QRectF actionRect(0.0, y, width(), Ui::DesignSystem::drawer().actionHeight());
        if (action->isChecked()) {
            painter.fillRect(
                actionRect.marginsRemoved(Ui::DesignSystem::drawer().selectionMargins()),
                Ui::DesignSystem::drawer().selectionColor());
        } else if (underMouse() && action->isEnabled()
                   && actionRect.contains(mapFromGlobal(QCursor::pos()))) {
            painter.fillRect(
                actionRect.marginsRemoved(Ui::DesignSystem::drawer().selectionMargins()),
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
        const QRectF iconRect(
            QPointF(Ui::DesignSystem::drawer().actionMargins().left(),
                    actionRect.top() + Ui::DesignSystem::drawer().actionMargins().top()),
            Ui::DesignSystem::drawer().iconSize());
        if (action->iconText() != action->text()) {
            painter.setFont(Ui::DesignSystem::font().iconsMid());
            painter.drawText(iconRect, Qt::AlignCenter, action->iconText());
        }
        //
        // ... текст
        //
        painter.setFont(Ui::DesignSystem::font().subtitle2());
        const QRectF textRect(iconRect.right() + Ui::DesignSystem::drawer().spacing(),
                              iconRect.top(),
                              width() - iconRect.right() - Ui::DesignSystem::drawer().spacing()
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

        y += Ui::DesignSystem::drawer().actionHeight();
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

void Drawer::mousePressEvent(QMouseEvent* _event)
{
    if (d->isAccountActionClicked(_event->pos(), width())) {
        d->decorationRect = d->pressedAccountActionRect(_event->pos(), width())
                                .adjusted(0, -Ui::DesignSystem::layout().px12(), 0,
                                          Ui::DesignSystem::layout().px12());
        d->decorationCenterPosition = d->decorationRect.center();
        d->decorationRadiusAnimation.setEndValue(d->decorationRect.width() / 2.0);
        d->animateClick();
        return;
    }

    if (d->isAccountPanelClicked(_event->pos())) {
        d->decorationCenterPosition = _event->pos();
        d->decorationRect
            = QRectF(0, 0, width(), d->accountPanelHeight() + Ui::DesignSystem::layout().px12());
        d->decorationRadiusAnimation.setEndValue(d->decorationRect.width());
        d->animateClick();
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

void Drawer::mouseReleaseEvent(QMouseEvent* _event)
{
    if (!rect().contains(_event->pos())) {
        return;
    }

    if (d->isAccountActionClicked(_event->pos(), width())) {
        auto pressedAction = d->pressedAccountAction(_event->pos(), width());
        pressedAction->trigger();
        update();
        return;
    }

    if (d->isAccountPanelClicked(_event->pos())) {
        emit accountPressed();
        return;
    }

    auto pressedAction = d->pressedAction(_event->pos(), actions());
    if (pressedAction == nullptr || pressedAction->isChecked() || !pressedAction->isEnabled()) {
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
