#include "shadow.h"

#include <ui/design_system/design_system.h>

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
    : Widget(_parent),
      m_edge(_edge)
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

            default: break;
        }
    }

    return Widget::eventFilter(_watched, _event);
}

void Shadow::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

    if (auto scrollArea = qobject_cast<QScrollArea*>(parentWidget())) {
        if ((m_edge == Qt::TopEdge && scrollArea->verticalScrollBar()->value() == 0)
            || (m_edge == Qt::LeftEdge && scrollArea->horizontalScrollBar()->value() == 0)) {
            return;
        }
    }

    QPainter painter(this);
    const auto lineWidth = Ui::DesignSystem::scaleFactor();
    painter.setPen(QPen(Ui::DesignSystem::color().shadow(), lineWidth));

    switch (m_edge) {
        default:
        case Qt::LeftEdge: {
            auto x = lineWidth / 2;
            painter.setOpacity(0.4);
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
            x += lineWidth;
            painter.setOpacity(0.2);
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
            x += lineWidth;
            painter.setOpacity(0.1);
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
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

        case Qt::RightEdge: {
            auto x = width() - lineWidth / 2;
            painter.setOpacity(0.4);
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
            x -= lineWidth;
            painter.setOpacity(0.2);
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
            x -= lineWidth;
            painter.setOpacity(0.1);
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
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
        case Qt::LeftEdge: {
            height = parentWidget()->height();
            width = shadowWidth;
            break;
        }

        case Qt::TopEdge: {
            height = shadowWidth;
            width = parentWidget()->width();
            break;
        }

        case Qt::RightEdge: {
            x = parentWidget()->width() - shadowWidth;
            height = parentWidget()->height();
            width = shadowWidth;
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
