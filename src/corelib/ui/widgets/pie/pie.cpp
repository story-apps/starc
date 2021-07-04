#include "pie.h"

#include <ui/design_system/design_system.h>

#include <QAbstractItemModel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>

class Pie::Implementation
{
public:
    Implementation();
    Implementation(const QAbstractItemModel* model, const int valueColumn);

    /**
     * @brief Расчиать геометрию кусочка
     */
    QPainterPath pieSlice(const QRectF& _rect, qreal _startAngle, qreal _arcLength,
                          qreal _hole = 0);

    /**
     * @brief Масштабировать прямоугольник по середине
     */
    QRectF scaleRectF(const QRectF& _rect, qreal _scale);

    /**
     * @brief Обновить размеры кусочков
     */
    void recalculateSlices(const QRectF& _region);

    /**
     * @brief Сбросить выбранный кусок
     */
    void invalidteSelectedSlice();

    /**
     * @brief Анимировать выбор кусочка
     */
    void animateSelectedSlice();

    /**
     * @brief Анимировать отмену выбора кусочка
     */
    void animateDeselectedSlice();

    struct Slice {
        QModelIndex index;

        double value;

        QPainterPath path;
        QColor color;
    };

    std::list<std::unique_ptr<Slice>> slices;
    const Slice* selectedSlice = nullptr;
    const Slice* lastSelectedSlice = nullptr;

    const QAbstractItemModel* model;
    const int valueColumn;

    double hole;

    QVariantAnimation sliceOpacityAnimation;
};

Pie::Implementation::Implementation()
    : model(nullptr)
    , valueColumn(0)
{
    sliceOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    sliceOpacityAnimation.setStartValue(1.0);
    sliceOpacityAnimation.setEndValue(0.4);
    sliceOpacityAnimation.setDuration(250);
}

Pie::Implementation::Implementation(const QAbstractItemModel* model, const int valueColumn)
    : model(model)
    , valueColumn(valueColumn)
{
    sliceOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    sliceOpacityAnimation.setStartValue(1.0);
    sliceOpacityAnimation.setEndValue(0.4);
    sliceOpacityAnimation.setDuration(250);
}

QPainterPath Pie::Implementation::pieSlice(const QRectF& _rect, qreal _startAngle, qreal _arcLength,
                                           qreal _hole)
{
    QPainterPath slice;
    slice.moveTo(_rect.center());
    slice.arcTo(_rect, _startAngle, _arcLength);

    if (_hole != 0) {
        slice.arcTo(scaleRectF(_rect, _hole), _startAngle + _arcLength, -1 * _arcLength);
    }

    return slice;
}

QRectF Pie::Implementation::scaleRectF(const QRectF& _rect, qreal _scale)
{
    auto size = QSizeF(_rect.width() * _scale, _rect.height() * _scale);
    return QRectF(_rect.center() - QPointF(size.width() / 2.0, size.height() / 2.0), size);
}

void Pie::Implementation::recalculateSlices(const QRectF& _region)
{
    auto total
        = std::accumulate(slices.begin(), slices.end(), 0.0,
                          [](const double a, const std::unique_ptr<Implementation::Slice>& b) {
                              return a + b->value;
                          });
    auto circle = 360.0;

    for (auto [it, start] = std::tuple{ slices.begin(), 0.0 }; it != slices.end(); ++it) {
        auto length = (circle * it->get()->value) / total;
        it->get()->path = pieSlice(_region, start, length, hole);

        start += length;
    }
}

void Pie::Implementation::invalidteSelectedSlice()
{
    if (selectedSlice) {
        selectedSlice = lastSelectedSlice = slices.begin()->get();
    } else {
        selectedSlice = lastSelectedSlice = nullptr;
    }
}

void Pie::Implementation::animateSelectedSlice()
{
    sliceOpacityAnimation.setCurrentTime(0);
    sliceOpacityAnimation.setDirection(QVariantAnimation::Forward);
    sliceOpacityAnimation.start();
}

void Pie::Implementation::animateDeselectedSlice()
{
    sliceOpacityAnimation.setCurrentTime(0);
    sliceOpacityAnimation.setDirection(QVariantAnimation::Backward);
    sliceOpacityAnimation.start();
}

Pie::Pie(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    this->setMouseTracking(true);

    connect(&d->sliceOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });
}

Pie::Pie(const QAbstractItemModel* model, const int valueColumn, QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(model, valueColumn))
{
    this->setMouseTracking(true);

    connectSignals(model);

    connect(&d->sliceOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });
}

Pie::~Pie()
{
}

void Pie::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (d->sliceOpacityAnimation.state() == QVariantAnimation::Running) {
        painter.setOpacity(d->sliceOpacityAnimation.currentValue().toReal());
    } else if (d->selectedSlice) {
        painter.setOpacity(d->sliceOpacityAnimation.endValue().toReal());
    }

    for (const auto& slice : d->slices) {
        if (d->lastSelectedSlice == slice.get()) {
            continue;
        }
        painter.fillPath(slice->path, slice->color);
    }

    if (d->lastSelectedSlice != nullptr) {
        painter.setOpacity(1.0);
        painter.fillPath(d->lastSelectedSlice->path, d->lastSelectedSlice->color);
    }
}

void Pie::resizeEvent(QResizeEvent* _event)
{
    Q_UNUSED(_event);
    recalculateSlices();
}

void Pie::mouseMoveEvent(QMouseEvent* _event)
{
    for (const auto& slice : d->slices) {
        if (slice->path.contains(_event->pos())) {
            if (d->selectedSlice != slice.get()) {
                if (d->selectedSlice == nullptr) {
                    d->animateSelectedSlice();
                } else {
                    update();
                }

                d->selectedSlice = slice.get();
                d->lastSelectedSlice = d->selectedSlice;

                emit itemSelected(slice->index);
                return;
            } else {
                return;
            }
        }
    }

    if (d->selectedSlice != nullptr) {
        d->selectedSlice = nullptr;
        d->animateDeselectedSlice();
    }
}

void Pie::recalculateSlices()
{
    auto min = std::min(size().width(), size().height());
    d->recalculateSlices(
        { { (rect().width() - min) / 2.0, (rect().height() - min) / 2.0 }, QSize{ min, min } });

    update();
}

void Pie::insertSlices(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)

    auto iter = d->slices.begin();
    std::advance(iter, first);

    for (int i = first; i <= last; i++) {
        auto slice = new Implementation::Slice();
        auto ok = false;

        slice->color
            = d->model->data(d->model->index(i, 0), Qt::DecorationPropertyRole).value<QColor>();
        slice->value = d->model->data(d->model->index(i, 1)).toDouble(&ok);

        assert(ok && "can't cast value to double");

        d->slices.insert(iter, std::unique_ptr<Implementation::Slice>(slice));
    }

    recalculateSlices();
    updateSelectedSlice();
}

void Pie::removeSlices(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)

    auto rangeBegin = d->slices.begin();
    auto rangeEnd = d->slices.begin();

    std::advance(rangeBegin, first);
    std::advance(rangeEnd, last);

    d->slices.erase(rangeBegin, rangeEnd);
    d->invalidteSelectedSlice();

    recalculateSlices();
    updateSelectedSlice();
}

void Pie::changeData(const QModelIndex& topLeft, const QModelIndex& bottomRight,
                     const QVector<int>& roles)
{

    auto isContainsColor = roles.contains(Qt::DecorationPropertyRole);
    auto isSomethingChanged = false;

    for (int i = topLeft.row(); i <= bottomRight.row(); i++) {
        auto iter = d->slices.begin();
        std::advance(iter, i);

        for (int j = topLeft.column(); j <= bottomRight.column(); j++) {
            if (j == d->valueColumn) {
                auto ok = false;

                iter->get()->value = d->model->index(i, j).data().toDouble(&ok);
                assert(ok && "can't cast value to double");

                isSomethingChanged = true;
            }

            if (isContainsColor) {
                iter->get()->color
                    = d->model->index(i, j).data(Qt::DecorationPropertyRole).value<QColor>();

                isSomethingChanged = true;
            }
        }
    }

    if (isSomethingChanged) {
        d->invalidteSelectedSlice();

        recalculateSlices();
        updateSelectedSlice();
    }
}

void Pie::updateSelectedSlice()
{
    auto event = QMouseEvent(QEvent::None, QWidget::mapFromGlobal(QCursor::pos()),
                             Qt::MouseButton::NoButton, Qt::MouseButton::NoButton,
                             Qt::KeyboardModifier::NoModifier);
    mouseMoveEvent(&event);
}

void Pie::connectSignals(const QAbstractItemModel* model)
{
    assert(model && "model is nullptr");

    connect(model, &QAbstractItemModel::rowsInserted, this, &Pie::insertSlices);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &Pie::removeSlices);
    connect(model, &QAbstractItemModel::dataChanged, this, &Pie::changeData);
}

void Pie::setModel(const QAbstractItemModel* model, const int valueColumn)
{
    assert(model && "model is nullptr");
    assert(valueColumn < model->columnCount() && "value index more than model column count");

    connectSignals(model);

    d.reset(new Implementation(model, valueColumn));
    connect(&d->sliceOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });

    for (int i = 0; i < d->model->rowCount(); i++) {
        auto slice = new Implementation::Slice();
        auto ok = false;

        slice->index = d->model->index(i, 0);
        slice->color
            = d->model->data(d->model->index(i, 0), Qt::DecorationPropertyRole).value<QColor>();
        slice->value = d->model->data(d->model->index(i, valueColumn)).toDouble(&ok);

        assert(ok && "can't cast value to double");

        d->slices.emplace_back(std::move(slice));
    }

    recalculateSlices();
}

void Pie::setHole(const double _hole)
{
    d->hole = _hole;
}
