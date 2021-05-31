#include "pie.h"

#include <ui/design_system/design_system.h>

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

class Pie::Implementation {
public:
    Implementation();

    QPainterPath pieSlice(const QRectF& _rect, qreal _startAngle,
        qreal _arcLength, qreal _hole = 0);

    QRectF scaleRectF(const QRectF& _rect, qreal _scale);

    void addItem(const double _value, const QColor& _color);

    void recalculateSlices(const QRectF& _region);

    struct Item {
        double value;

        QPainterPath slice;
        QColor color;
    };

    std::vector<Item> items;
    const Item* selecrtedItem = nullptr;

    double hole;
};

Pie::Implementation::Implementation() { }

QPainterPath Pie::Implementation::pieSlice(const QRectF& _rect,
    qreal _startAngle, qreal _arcLength,
    qreal _hole)
{
    QPainterPath slice;
    slice.moveTo(_rect.center());
    slice.arcTo(_rect, _startAngle, _arcLength);

    if (_hole != 0) {
        slice.arcTo(scaleRectF(_rect, _hole), _startAngle + _arcLength,
            -1 * _arcLength);
    }

    return slice;
}

QRectF Pie::Implementation::scaleRectF(const QRectF& _rect, qreal _scale)
{
    auto size = QSizeF(_rect.width() * _scale, _rect.height() * _scale);

    return QRectF(_rect.center() - QPointF(size.width() / 2, size.height() / 2),
        size);
}

void Pie::Implementation::addItem(const double _value, const QColor& _color)
{
    items.push_back({ .value = _value, .color = _color });
}

void Pie::Implementation::recalculateSlices(const QRectF& _region)
{
    auto total = std::accumulate(items.begin(), items.end(), 0.0, [](const double a, const Implementation::Item& b) {
        return a + b.value;
    });
    auto circle = 360.0;

    for (auto [it, start] = std::tuple { items.begin(), 0.0 }; it != items.end(); ++it) {
        auto length = (circle * it->value) / total;
        it->slice = pieSlice(_region, start, length, hole);

        start += length;
    }
}

Pie::Pie(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    this->setMouseTracking(true);
}

Pie::~Pie() { }

void Pie::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    for (const auto& item : d->items) {
        painter.fillPath(item.slice, item.color);
    }

    if (d->selecrtedItem != nullptr) {
        painter.setFont(Ui::DesignSystem::font().body1());
        painter.setPen(Ui::DesignSystem::color().error());
        painter.drawText(d->selecrtedItem->slice.boundingRect(), Qt::AlignCenter, QString::number(d->selecrtedItem->value));
        painter.drawRect(d->selecrtedItem->slice.boundingRect());
    }
}

void Pie::resizeEvent(QResizeEvent* _event)
{
    Q_UNUSED(_event);
    recalculateSlices();
}

void Pie::mouseMoveEvent(QMouseEvent* _event)
{
    for (const auto& item : d->items) {
        if (item.slice.contains(_event->pos())) {
            if (d->selecrtedItem != &item) {
                d->selecrtedItem = &item;
                qDebug() << d->selecrtedItem->color;
                //                emit itemSelected();
                update();
                return;
            } else {
                return;
            }
        }
    }

    if (d->selecrtedItem != nullptr) {
        d->selecrtedItem = nullptr;
        update();
    }
}

void Pie::recalculateSlices()
{
    auto min = std::min(size().width(), size().height());
    d->recalculateSlices(QRectF(QPoint(rect().center().x() - (min / 2), rect().center().y() - (min / 2)), QSize(min, min)));
}

void Pie::addItem(const double _value, const QColor& _color)
{
    d->addItem(_value, _color);

    recalculateSlices();
}

void Pie::setHole(const double _hole)
{
    d->hole = _hole;
}
