/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2013-2020 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "color_2d_slider.h"

#include <ui/design_system/design_system.h>

#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

namespace color_widgets {

static const double selector_radius = 6;

class Color2DSlider::Private
{
public:
    qreal hue = 1, sat = 1, val = 1;
    Component comp_x = Saturation;
    Component comp_y = Value;
    QImage square;

    qreal PixHue(float x, float y)
    {
        if (comp_x == Hue)
            return x;
        if (comp_y == Hue)
            return y;
        return hue;
    }

    qreal PixSat(float x, float y)
    {
        if (comp_x == Saturation)
            return x;
        if (comp_y == Saturation)
            return y;
        return sat;
    }

    qreal PixVal(float x, float y)
    {
        if (comp_x == Value)
            return x;
        if (comp_y == Value)
            return y;
        return val;
    }

    void renderSquare(const QSize& size)
    {
        const QSize sizeCorrected
            = size - QSize(Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16());
        square = QImage(sizeCorrected, QImage::Format_RGB32);

        for (int y = 0; y < sizeCorrected.height(); ++y) {
            qreal yfloat = 1 - qreal(y) / sizeCorrected.height();
            for (int x = 0; x < sizeCorrected.width(); ++x) {
                qreal xfloat = qreal(x) / sizeCorrected.width();
                square.setPixel(x, y,
                                QColor::fromHsvF(PixHue(xfloat, yfloat), PixSat(xfloat, yfloat),
                                                 PixVal(xfloat, yfloat))
                                    .rgb());
            }
        }
    }

    QPointF selectorPos(const QSize& size)
    {
        const QSize sizeCorrected
            = size - QSize(Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16());
        QPointF pt;
        switch (comp_x) {
        case Hue:
            pt.setX(sizeCorrected.width() * hue);
            break;
        case Saturation:
            pt.setX(sizeCorrected.width() * sat);
            break;
        case Value:
            pt.setX(sizeCorrected.width() * val);
            break;
        }
        switch (comp_y) {
        case Hue:
            pt.setY(sizeCorrected.height() * (1 - hue));
            break;
        case Saturation:
            pt.setY(sizeCorrected.height() * (1 - sat));
            break;
        case Value:
            pt.setY(sizeCorrected.height() * (1 - val));
            break;
        }
        return pt + QPointF(Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px8());
    }

    void setColorFromPos(const QPoint& pt, const QSize& size)
    {
        const QPoint pointCorrected
            = pt - QPoint(Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px8());
        const QSize sizeCorrected
            = size - QSize(Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16());
        QPointF ptfloat(qBound(0.0, qreal(pointCorrected.x()) / sizeCorrected.width(), 1.0),
                        qBound(0.0, 1 - qreal(pointCorrected.y()) / sizeCorrected.height(), 1.0));
        switch (comp_x) {
        case Hue:
            hue = ptfloat.x();
            break;
        case Saturation:
            sat = ptfloat.x();
            break;
        case Value:
            val = ptfloat.x();
            break;
        }
        switch (comp_y) {
        case Hue:
            hue = ptfloat.y();
            break;
        case Saturation:
            sat = ptfloat.y();
            break;
        case Value:
            val = ptfloat.y();
            break;
        }
    }
};

Color2DSlider::Color2DSlider(QWidget* parent)
    : Widget(parent)
    , p(new Private)
{
}

Color2DSlider::~Color2DSlider()
{
    delete p;
}

QColor Color2DSlider::color() const
{
    return QColor::fromHsvF(p->hue, p->sat, p->val);
}

QSize Color2DSlider::sizeHint() const
{
    return { 128, 128 };
}

qreal Color2DSlider::hue() const
{
    return p->hue;
}

qreal Color2DSlider::saturation() const
{
    return p->sat;
}

qreal Color2DSlider::value() const
{
    return p->val;
}

Color2DSlider::Component Color2DSlider::componentX() const
{
    return p->comp_x;
}

Color2DSlider::Component Color2DSlider::componentY() const
{
    return p->comp_y;
}

void Color2DSlider::setColor(const QColor& c)
{
    p->hue = c.hsvHueF();
    p->sat = c.saturationF();
    p->val = c.valueF();
    p->renderSquare(size());
    update();
    Q_EMIT colorChanged(color());
}

void Color2DSlider::setHue(qreal h)
{
    p->hue = h;
    p->renderSquare(size());
    update();
    Q_EMIT colorChanged(color());
}

void Color2DSlider::setSaturation(qreal s)
{
    p->sat = s;
    p->renderSquare(size());
    update();
    Q_EMIT colorChanged(color());
}

void Color2DSlider::setValue(qreal v)
{
    p->val = v;
    p->renderSquare(size());
    update();
    Q_EMIT colorChanged(color());
}

void Color2DSlider::setComponentX(Color2DSlider::Component componentX)
{
    if (componentX != p->comp_x) {
        p->comp_x = componentX;
        p->renderSquare(size());
        update();
        Q_EMIT componentXChanged(p->comp_x);
    }
}

void Color2DSlider::setComponentY(Color2DSlider::Component componentY)
{
    if (componentY != p->comp_y) {
        p->comp_y = componentY;
        p->renderSquare(size());
        update();
        Q_EMIT componentXChanged(p->comp_y);
    }
}

void Color2DSlider::paintEvent(QPaintEvent* _event)
{
    Widget::paintEvent(_event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    QBrush brush(p->square);
    QTransform brushTransform;
    brushTransform.translate(Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px8());
    brush.setTransform(brushTransform);
    painter.setBrush(brush);
    painter.drawRoundedRect(Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px8(),
                            p->square.width(), p->square.height(), Ui::DesignSystem::layout().px4(),
                            Ui::DesignSystem::layout().px4());

    painter.setPen(QPen(textColor(), Ui::DesignSystem::layout().px2()));
    painter.setBrush(color());
    const auto radius = Ui::DesignSystem::layout().px4() + Ui::DesignSystem::layout().px2();
    painter.drawEllipse(p->selectorPos(size()), radius, radius);
}

void Color2DSlider::mousePressEvent(QMouseEvent* event)
{
    p->setColorFromPos(event->pos(), size());
    Q_EMIT colorChanged(color());
    update();
}

void Color2DSlider::mouseMoveEvent(QMouseEvent* event)
{
    p->setColorFromPos(event->pos(), size());
    Q_EMIT colorChanged(color());
    update();
}

void Color2DSlider::mouseReleaseEvent(QMouseEvent* event)
{
    p->setColorFromPos(event->pos(), size());
    Q_EMIT colorChanged(color());
    update();
}

void Color2DSlider::resizeEvent(QResizeEvent* event)
{
    p->renderSquare(event->size());
    update();
}

void Color2DSlider::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    p->renderSquare(size());
    update();
}


} // namespace color_widgets
