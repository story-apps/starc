#include "color_hue_slider.h"

#include <ui/design_system/design_system.h>

#include <QPainter>
#include <QResizeEvent>


class ColorHueSlider::Implementation
{
public:
    /**
     * @brief Перестроить градиент в зависимости от заданной ширины
     */
    void buildGradient(int _width);

    /**
     * @brief Определить точку расположения указателя цвета
     */
    QPointF selectorPos(const QSize& _size);

    /**
     * @brief Обновить значение цвета в зависимости от положения мышки
     */
    void updateHue(const QPoint& _pos, const QSize& _size);

    QLinearGradient gradient;
    qreal hue = 0.8;
};

void ColorHueSlider::Implementation::buildGradient(int _width)
{
    gradient = QLinearGradient(Ui::DesignSystem::layout().px8(), 0,
                               _width - Ui::DesignSystem::layout().px8(), 0);
    const qreal steps = 10;
    for (int step = 0; step <= steps; ++step) {
        gradient.setColorAt(step / steps, QColor::fromHsvF(step / steps, 1.0, 1.0));
    }
}

QPointF ColorHueSlider::Implementation::selectorPos(const QSize& _size)
{
    const QSizeF sizeCorrected
        = _size - QSizeF(Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16());
    return QPointF(Ui::DesignSystem::layout().px8() + sizeCorrected.width() * hue,
                   Ui::DesignSystem::layout().px8() + sizeCorrected.height() / 2);
}

void ColorHueSlider::Implementation::updateHue(const QPoint& _pos, const QSize& _size)
{
    const QPoint pointCorrected
        = _pos - QPoint(Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px8());
    const QSizeF sizeCorrected
        = _size - QSizeF(Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16());
    const QRectF hueRect(
        QPointF(Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px8()), sizeCorrected);
    hue = qBound(0.0, qreal(pointCorrected.x()) / sizeCorrected.width(), 1.0);
}


// ****


ColorHueSlider::ColorHueSlider(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    designSystemChangeEvent(nullptr);
}

ColorHueSlider::~ColorHueSlider() = default;

void ColorHueSlider::setHue(qreal _hue)
{
    d->hue = qBound(0.0, _hue, 1.0);
    update();
}

void ColorHueSlider::paintEvent(QPaintEvent* _event)
{
    Widget::paintEvent(_event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    painter.setBrush(d->gradient);
    painter.drawRoundedRect(
        rect().adjusted(Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px8(),
                        -Ui::DesignSystem::layout().px8(), -Ui::DesignSystem::layout().px8()),
        Ui::DesignSystem::layout().px4(), Ui::DesignSystem::layout().px4());

    painter.setPen(QPen(textColor(), Ui::DesignSystem::layout().px2()));
    painter.setBrush(QColor::fromHsvF(d->hue, 1.0, 1.0));
    const auto radius = Ui::DesignSystem::layout().px4() + Ui::DesignSystem::layout().px2();
    painter.drawEllipse(d->selectorPos(size()), radius, radius);
}

void ColorHueSlider::mousePressEvent(QMouseEvent* _event)
{
    d->updateHue(_event->pos(), size());
    emit hueChanged(d->hue);

    update();
}

void ColorHueSlider::mouseMoveEvent(QMouseEvent* _event)
{
    d->updateHue(_event->pos(), size());
    emit hueChanged(d->hue);

    update();
}

void ColorHueSlider::mouseReleaseEvent(QMouseEvent* _event)
{
    d->updateHue(_event->pos(), size());
    emit hueChanged(d->hue);

    update();
}

void ColorHueSlider::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);

    d->buildGradient(_event->size().width());
    update();
}

void ColorHueSlider::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    d->buildGradient(width());
}
