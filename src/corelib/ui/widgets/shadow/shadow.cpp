#include "shadow.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/tree/tree.h>
#include <utils/helpers/image_helper.h>

#include <QPainter>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QtMath>


Shadow::Shadow(QWidget* _parent)
    : Widget(_parent)
{
    _parent->installEventFilter(this);
    refreshGeometry();
}

Shadow::Shadow(Qt::Edge _edge, QWidget* _parent)
    : Widget(_parent)
    , m_edge(_edge)
{
    _parent->installEventFilter(this);
    refreshGeometry();
}

void Shadow::setEdge(Qt::Edge _edge)
{
    if (m_edge == _edge) {
        return;
    }

    m_edge = _edge;
    refreshGeometry();
    update();
}

bool Shadow::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == parentWidget()) {
        switch (_event->type()) {
        case QEvent::Resize: {
            refreshGeometry();
            break;
        }

        case QEvent::ChildAdded: {
            raise();
            break;
        }

        default:
            break;
        }
    }

    return Widget::eventFilter(_watched, _event);
}

void Shadow::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

    //
    // Если мы находимся внутри скролируемого виджета
    //
    QScrollBar* parentWidgetVerticalScrollBar = nullptr;
    QScrollBar* parentWidgetHorizontalScrollBar = nullptr;
    if (auto scrollArea = qobject_cast<QScrollArea*>(parentWidget())) {
        parentWidgetVerticalScrollBar = scrollArea->verticalScrollBar();
        parentWidgetHorizontalScrollBar = scrollArea->horizontalScrollBar();
    } else if (auto tree = qobject_cast<Tree*>(parentWidget())) {
        parentWidgetVerticalScrollBar = tree->verticalScrollBar();
    }
    //
    // То возможно нам и не нужно рисовать тень
    //
    if ((m_edge == Qt::TopEdge && parentWidgetVerticalScrollBar != nullptr
         && parentWidgetVerticalScrollBar->value() == 0)
        || (m_edge == Qt::BottomEdge && parentWidgetVerticalScrollBar != nullptr
            && parentWidgetVerticalScrollBar->value() == parentWidgetVerticalScrollBar->maximum())
        || (m_edge == Qt::LeftEdge && parentWidgetHorizontalScrollBar != nullptr
            && parentWidgetHorizontalScrollBar->value() == 0)
        || (m_edge == Qt::RightEdge && parentWidgetHorizontalScrollBar != nullptr
            && parentWidgetHorizontalScrollBar->value()
                == parentWidgetHorizontalScrollBar->maximum())) {
        return;
    }

    QPainter painter(this);
    const auto lineWidth = Ui::DesignSystem::scaleFactor();
    painter.setPen(QPen(Ui::DesignSystem::color().shadow(), lineWidth));

    switch (m_edge) {
    default:
    case Qt::LeftEdge:
    case Qt::RightEdge: {
        if ((m_edge == Qt::LeftEdge && isLeftToRight())
            || (m_edge == Qt::RightEdge && isRightToLeft())) {
            auto x = lineWidth / 2;
            painter.setOpacity(0.4);
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
            x += lineWidth;
            painter.setOpacity(0.2);
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
            x += lineWidth;
            painter.setOpacity(0.1);
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
        } else {
            auto x = width() - lineWidth / 2;
            painter.setOpacity(0.4);
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
            x -= lineWidth;
            painter.setOpacity(0.2);
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
            x -= lineWidth;
            painter.setOpacity(0.1);
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
        }

        break;
    }

    case Qt::TopEdge: {
        auto y = lineWidth / 2;
        painter.setOpacity(0.4);
        painter.drawLine(QPointF(0.0, y), QPointF(width(), y));
        y += lineWidth;
        painter.setOpacity(0.2);
        painter.drawLine(QPointF(0.0, y), QPointF(width(), y));
        y += lineWidth;
        painter.setOpacity(0.1);
        painter.drawLine(QPointF(0.0, y), QPointF(width(), y));
        break;
    }

    case Qt::BottomEdge: {
        auto y = height() - lineWidth / 2;
        painter.setOpacity(0.4);
        painter.drawLine(QPointF(0.0, y), QPointF(width(), y));
        y -= lineWidth;
        painter.setOpacity(0.2);
        painter.drawLine(QPointF(0.0, y), QPointF(width(), y));
        y -= lineWidth;
        painter.setOpacity(0.1);
        painter.drawLine(QPointF(0.0, y), QPointF(width(), y));
        break;
    }
    }
}

void Shadow::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    refreshGeometry();
}

void Shadow::refreshGeometry()
{
    if (parentWidget() == nullptr) {
        return;
    }

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    const int shadowWidth = qCeil(Ui::DesignSystem::scaleFactor() * 3);
    switch (m_edge) {
    default:
    case Qt::LeftEdge:
    case Qt::RightEdge: {
        if ((m_edge == Qt::LeftEdge && isLeftToRight())
            || (m_edge == Qt::RightEdge && isRightToLeft())) {
            height = parentWidget()->height();
            width = shadowWidth;
        } else {
            x = parentWidget()->width() - shadowWidth;
            height = parentWidget()->height();
            width = shadowWidth;
        }
        break;
    }

    case Qt::TopEdge: {
        height = shadowWidth;
        width = parentWidget()->width();
        break;
    }

    case Qt::BottomEdge: {
        y = parentWidget()->height() - shadowWidth;
        height = shadowWidth;
        width = parentWidget()->width();
        break;
    }
    }
    setGeometry(x, y, width, height);
}
