#include "drawer.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/image_helper.h>

#include <QAction>
#include <QPainter>
#include <QMouseEvent>


class Drawer::Implementation
{
public:
    /**
     * @brief Получить пункт меню по координате
     */
    QAction* pressedAction(const QPoint& _coordinate, const QList<QAction*>& _actions) const;

    QString title;
    QString subtitle;
};

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


// ****


Drawer::Drawer(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    setFixedWidth(static_cast<int>(Ui::DesignSystem::drawer().width()));
}

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

Drawer::~Drawer() = default;

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

        const QRectF actionRect(0.0, actionY, width(), Ui::DesignSystem::drawer().actionHeight());
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
        if (!action->iconText().isEmpty()) {
            painter.setFont(Ui::DesignSystem::font().iconsMid());
            painter.drawText(iconRect, Qt::AlignCenter, action->iconText());
        }
        //
        // ... текст
        //
        painter.setFont(Ui::DesignSystem::font().subtitle2());
        const QRectF textRect(iconRect.right() + Ui::DesignSystem::drawer().iconRightMargin(),
                              iconRect.top(),
                              width()
                              - iconRect.right()
                              - Ui::DesignSystem::drawer().iconRightMargin()
                              - Ui::DesignSystem::drawer().actionMargins().right(),
                              Ui::DesignSystem::drawer().actionHeight()
                              - Ui::DesignSystem::drawer().actionMargins().top()
                              - Ui::DesignSystem::drawer().actionMargins().bottom());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, action->text());

        //
        // ... обводка
        //
        if (action->isChecked()) {
            painter.fillRect(actionRect.marginsRemoved(Ui::DesignSystem::drawer().selectionMargins()),
                             Ui::DesignSystem::drawer().selectionColor());
        }

        actionY += Ui::DesignSystem::drawer().actionHeight();
    }
}

void Drawer::mouseReleaseEvent(QMouseEvent* _event)
{
    QAction* pressedAction = d->pressedAction(_event->pos(), actions());
    if (pressedAction == nullptr) {
        return;
    }

    if (pressedAction->isChecked()) {
        return;
    }

    pressedAction->trigger();
    update();
}
