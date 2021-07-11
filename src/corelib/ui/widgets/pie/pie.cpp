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
    struct Slice {
        QModelIndex index;

        qreal value;

        QPainterPath path;
        QColor color;
    };

    Implementation(Pie* _q, qreal _hole = 0);
    Implementation(Pie* _q, const QAbstractItemModel* _model, int _valueColumn, qreal _hole = 0);

    /**
     * @brief Сбрасываем модель
     */
    void reset(const QAbstractItemModel* _model, int _valueColumn);

    /**
     * @brief Установить модель
     */
    void setModel(const QAbstractItemModel* _model, int _valueColumn);

    /**
     * @brief В модель пришли новые данные
     */
    void insertSlices(const QModelIndex& _parent, int _first, int _last);

    /**
     * @brief Из модели удалили данные
     */
    void removeSlices(const QModelIndex& _parent, int _first, int _last);

    /**
     * @brief В модели изменились данные
     */
    void updateSlices(const QModelIndex& _topLeft, const QModelIndex& _bottomRight,
                      const QVector<int>& _roles);

    /**
     * @brief Подключаемся к сигналам модели
     */
    void connectToModel();

    /**
     * @brief Найти выбранный кусочек
     */
    Slice* findSelectedSlice(const QPoint& point);

    /**
     * @brief Обновляем выбранный кусочек
     */
    void updateSelectedSlice();

    /**
     * @brief Расчиать геометрию кусочка
     */
    QPainterPath pieSlice(const QRectF& _rect, qreal _startAngle, qreal _arcLength,
                          qreal _hole = 0);

    /**
     * @brief Масштабировать прямоугольник по середине
     */
    QRectF scaledRectFromCenter(const QRectF& _rect, qreal _scale);

    /**
     * @brief Обновить размеры кусочков
     */
    void recalculateSlices();

    /**
     * @brief Определяем существует ли выделенный кусочек
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

    Pie* q;

    std::list<std::unique_ptr<Slice>> slices;
    const Slice* selectedSlice;
    const Slice* lastSelectedSlice;

    const QAbstractItemModel* model;
    int valueColumn;

    qreal hole;

    QVariantAnimation sliceOpacityAnimation;
};

Pie::Implementation::Implementation(Pie* _q, qreal _hole)
    : q(_q)
    , selectedSlice(nullptr)
    , lastSelectedSlice(nullptr)
    , model(nullptr)
    , valueColumn(0)
    , hole(_hole)
{
    sliceOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    sliceOpacityAnimation.setStartValue(1.0);
    sliceOpacityAnimation.setEndValue(Ui::DesignSystem::inactiveItemOpacity());
    sliceOpacityAnimation.setDuration(250);


    connect(&sliceOpacityAnimation, &QVariantAnimation::valueChanged, q, qOverload<>(&Pie::update));
}

Pie::Implementation::Implementation(Pie* _q, const QAbstractItemModel* _model, int _valueColumn,
                                    qreal _hole)
    : q(_q)
    , selectedSlice(nullptr)
    , lastSelectedSlice(nullptr)
    , model(_model)
    , valueColumn(_valueColumn)
    , hole(_hole)
{
    sliceOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    sliceOpacityAnimation.setStartValue(1.0);
    sliceOpacityAnimation.setEndValue(Ui::DesignSystem::inactiveItemOpacity());
    sliceOpacityAnimation.setDuration(250);

    connectToModel();

    connect(&sliceOpacityAnimation, &QVariantAnimation::valueChanged, q, qOverload<>(&Pie::update));
}

void Pie::Implementation::reset(const QAbstractItemModel* _model, int _valueColumn)
{
    if (model) {
        model->disconnect(q);
    }

    model = _model;
    valueColumn = _valueColumn;

    selectedSlice = nullptr;
    lastSelectedSlice = nullptr;

    slices.clear();
}

void Pie::Implementation::setModel(const QAbstractItemModel* _model, int _valueColumn)
{
    reset(_model, _valueColumn);
    if (_model == nullptr) {
        return;
    }

    connectToModel();


    for (int i = 0; i < model->rowCount(); ++i) {
        auto slice = new Implementation::Slice();
        auto ok = false;

        slice->index = model->index(i, 0);
        slice->color = model->data(model->index(i, 0), Qt::DecorationPropertyRole).value<QColor>();
        slice->value = model->data(model->index(i, _valueColumn)).toDouble(&ok);

        assert(ok && "can't cast value to double");

        slices.emplace_back(std::move(slice));
    }

    recalculateSlices();

    q->update();
}

void Pie::Implementation::insertSlices(const QModelIndex& _parent, int _first, int _last)
{
    Q_UNUSED(_parent)

    auto iter = slices.begin();
    std::advance(iter, _first);

    for (int i = _first; i <= _last; ++i) {
        auto slice = new Implementation::Slice();
        auto ok = false;

        slice->color = model->data(model->index(i, 0), Qt::DecorationPropertyRole).value<QColor>();
        slice->value = model->data(model->index(i, 1)).toDouble(&ok);

        assert(ok && "can't cast value to double");

        slices.insert(iter, std::unique_ptr<Implementation::Slice>(slice));
    }

    recalculateSlices();
    updateSelectedSlice();

    q->update();
}

void Pie::Implementation::removeSlices(const QModelIndex& _parent, int _first, int _last)
{
    Q_UNUSED(_parent)

    auto rangeBegin = slices.begin();
    auto rangeEnd = slices.begin();

    std::advance(rangeBegin, _first);
    std::advance(rangeEnd, _last);

    slices.erase(rangeBegin, rangeEnd);
    invalidteSelectedSlice();

    recalculateSlices();
    updateSelectedSlice();

    q->update();
}

void Pie::Implementation::updateSlices(const QModelIndex& _topLeft, const QModelIndex& _bottomRight,
                                       const QVector<int>& _roles)
{
    auto isContainsColor = _roles.contains(Qt::DecorationPropertyRole);
    auto isSomethingChanged = false;

    for (int i = _topLeft.row(); i <= _bottomRight.row(); ++i) {
        auto iter = slices.begin();
        std::advance(iter, i);

        for (int j = _topLeft.column(); j <= _bottomRight.column(); ++j) {
            if (j == valueColumn) {
                auto ok = false;

                iter->get()->value = model->index(i, j).data().toDouble(&ok);
                assert(ok && "can't cast value to double");

                isSomethingChanged = true;
            }

            if (isContainsColor) {
                iter->get()->color
                    = model->index(i, j).data(Qt::DecorationPropertyRole).value<QColor>();

                isSomethingChanged = true;
            }
        }
    }

    if (isSomethingChanged) {
        invalidteSelectedSlice();

        recalculateSlices();
        updateSelectedSlice();

        q->update();
    }
}

void Pie::Implementation::connectToModel()
{
    connect(model, &QAbstractItemModel::rowsInserted, q,
            [this](const QModelIndex& _parent, int _first, int _last) {
                insertSlices(_parent, _first, _last);
            });
    connect(model, &QAbstractItemModel::rowsRemoved, q,
            [this](const QModelIndex& _parent, int _first, int _last) {
                removeSlices(_parent, _first, _last);
            });
    connect(model, &QAbstractItemModel::dataChanged, q,
            [this](const QModelIndex& _topLeft, const QModelIndex& _bottomRight,
                   const QVector<int>& _roles = QVector<int>()) {
                updateSlices(_topLeft, _bottomRight, _roles);
            });
}

Pie::Implementation::Slice* Pie::Implementation::findSelectedSlice(const QPoint& point)
{
    for (const auto& slice : slices) {
        if (slice->path.contains(point)) {
            return slice.get();
        }
    }

    return nullptr;
}

void Pie::Implementation::updateSelectedSlice()
{
    auto selectedSlice = findSelectedSlice(q->QWidget::mapFromGlobal(QCursor::pos()));
    if (this->selectedSlice == selectedSlice) {
        return;
    }

    this->selectedSlice = selectedSlice;
    this->lastSelectedSlice = selectedSlice;

    emit q->itemSelected(this->selectedSlice->index);

    return;
}

QPainterPath Pie::Implementation::pieSlice(const QRectF& _rect, qreal _startAngle, qreal _arcLength,
                                           qreal _hole)
{
    QPainterPath slice;
    slice.moveTo(_rect.center());
    slice.arcTo(_rect, _startAngle, _arcLength);

    if (_hole != 0) {
        slice.arcTo(scaledRectFromCenter(_rect, _hole), _startAngle + _arcLength, -1 * _arcLength);
    }

    return slice;
}

QRectF Pie::Implementation::scaledRectFromCenter(const QRectF& _rect, qreal _scale)
{
    const auto size = _rect.size() * _scale;
    return QRectF(_rect.center() - QPointF(size.width() / 2.0, size.height() / 2.0), size);
}

void Pie::Implementation::recalculateSlices()
{
    const auto circle = 360.0;
    const auto min = std::min(q->size().width(), q->size().height());
    const QRectF region = { { (q->rect().width() - min) / 2.0, (q->rect().height() - min) / 2.0 },
                            QSize{ min, min } };
    const auto total
        = std::accumulate(slices.begin(), slices.end(), 0.0,
                          [](const qreal a, const std::unique_ptr<Implementation::Slice>& b) {
                              return a + b->value;
                          });

    qreal start = 0.0;
    for (auto it = slices.begin(); it != slices.end(); ++it) {
        const auto length = (circle * it->get()->value) / total;
        it->get()->path = pieSlice(region, start, length, hole);

        start += length;
    }
}

void Pie::Implementation::invalidteSelectedSlice()
{
    if (!selectedSlice) {
        return;
    }

    for (const auto& slice : slices) {
        if (slice.get() == selectedSlice) {
            return;
        }
    }

    selectedSlice = nullptr;
    lastSelectedSlice = slices.begin()->get();
}

void Pie::Implementation::animateSelectedSlice()
{
    sliceOpacityAnimation.setDirection(QVariantAnimation::Forward);
    sliceOpacityAnimation.start();
}

void Pie::Implementation::animateDeselectedSlice()
{
    sliceOpacityAnimation.setDirection(QVariantAnimation::Backward);
    sliceOpacityAnimation.start();
}

Pie::Pie(QWidget* _parent, qreal _hole)
    : Widget(_parent)
    , d(new Implementation(this, _hole))
{
    setMouseTracking(true);
}

Pie::Pie(const QAbstractItemModel* _model, int _valueColumn, qreal _hole, QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this, _model, _valueColumn, _hole))
{
    setMouseTracking(true);
}

Pie::~Pie() = default;

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
    d->recalculateSlices();
    update();
}

void Pie::mouseMoveEvent(QMouseEvent* _event)
{
    auto seletedSlice = d->findSelectedSlice(_event->pos());

    if (d->selectedSlice == seletedSlice) {
        return;
    }

    if (!seletedSlice && d->selectedSlice) {
        d->selectedSlice = nullptr;
        d->animateDeselectedSlice();
        return;
    }

    auto isUpdate = false;
    if (d->selectedSlice) {
        isUpdate = true;
    }

    d->selectedSlice = seletedSlice;
    d->lastSelectedSlice = seletedSlice;

    isUpdate ? update() : d->animateSelectedSlice();

    emit itemSelected(d->selectedSlice->index);
    return;
}

void Pie::setModel(const QAbstractItemModel* _model, int _valueColumn)
{
    d->setModel(_model, _valueColumn);
}

void Pie::setHole(qreal _hole)
{
    if (d->hole == _hole) {
        return;
    }

    d->hole = _hole;
    d->recalculateSlices();
    update();
}
