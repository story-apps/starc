#include "pie.h"

#include <ui/design_system/design_system.h>

#include <QAbstractItemModel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QVariantAnimation>

namespace {
int kInvalidIndex = -1;
}


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

    /**
     * @brief Сбрасываем модель
     */
    void reset(QAbstractItemModel* _model, int _valueColumn);

    /**
     * @brief Установить модель
     */
    void setModel(QAbstractItemModel* _model, int _valueColumn);

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
    int findSelectedSlice(const QPoint& point);

    /**
     * @brief Выделить заданный слайс
     */
    void selectSlice(int _index);

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
     * @brief Анимировать выбор кусочка
     */
    void animateSelectedSlice();

    /**
     * @brief Анимировать отмену выбора кусочка
     */
    void animateDeselectedSlice();

    Pie* q = nullptr;

    std::vector<Slice> slices;
    int selectedSlice = kInvalidIndex;
    int lastSelectedSlice = kInvalidIndex;

    QPointer<QAbstractItemModel> model;
    int valueColumn = 0;

    qreal hole = 0;

    QVariantAnimation sliceOpacityAnimation;
};

Pie::Implementation::Implementation(Pie* _q, qreal _hole)
    : q(_q)
    , hole(_hole)
{
    sliceOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    sliceOpacityAnimation.setStartValue(1.0);
    sliceOpacityAnimation.setEndValue(Ui::DesignSystem::inactiveItemOpacity());
    sliceOpacityAnimation.setDuration(250);


    connect(&sliceOpacityAnimation, &QVariantAnimation::valueChanged, q, qOverload<>(&Pie::update));
}

void Pie::Implementation::reset(QAbstractItemModel* _model, int _valueColumn)
{
    if (model) {
        model->disconnect(q);
    }

    model = _model;
    valueColumn = _valueColumn;

    selectedSlice = kInvalidIndex;
    lastSelectedSlice = kInvalidIndex;

    slices.clear();
}

void Pie::Implementation::setModel(QAbstractItemModel* _model, int _valueColumn)
{
    reset(_model, _valueColumn);
    if (_model == nullptr) {
        return;
    }

    connectToModel();

    slices.reserve(model->rowCount());
    for (int i = 0; i < model->rowCount(); ++i) {
        auto slice = Implementation::Slice();
        auto ok = false;

        slice.index = model->index(i, 0);
        slice.color = model->data(model->index(i, 0), Qt::DecorationPropertyRole).value<QColor>();
        slice.value = model->data(model->index(i, _valueColumn)).toDouble(&ok);

        assert(ok && "can't cast value to double");

        slices.push_back(slice);
    }

    recalculateSlices();

    q->update();
}

void Pie::Implementation::insertSlices(const QModelIndex& _parent, int _first, int _last)
{
    Q_UNUSED(_parent)

    auto iter = slices.begin();
    std::advance(iter, _first);


    auto temp = std::vector<Pie::Implementation::Slice>();
    temp.reserve(_last - _first + 1);
    for (int i = _first; i <= _last; ++i) {
        auto slice = Implementation::Slice();
        auto ok = false;

        slice.index = model->index(i, 0);
        slice.color = model->data(model->index(i, 0), Qt::DecorationPropertyRole).value<QColor>();
        slice.value = model->data(model->index(i, 1)).toDouble(&ok);

        assert(ok && "can't cast value to double");

        temp.push_back(slice);
    }

    slices.insert(iter, temp.begin(), temp.end());

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

    if (_first >= selectedSlice || _last <= selectedSlice) {
        selectedSlice = kInvalidIndex;
        lastSelectedSlice = slices.size() ? 0 : kInvalidIndex;
    }

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

                iter->value = model->index(i, j).data().toDouble(&ok);
                assert(ok && "can't cast value to double");

                isSomethingChanged = true;
            }

            if (isContainsColor) {
                iter->color = model->index(i, j).data(Qt::DecorationPropertyRole).value<QColor>();

                isSomethingChanged = true;
            }
        }
    }

    if (isSomethingChanged) {
        recalculateSlices();
        updateSelectedSlice();

        q->update();
    }
}

void Pie::Implementation::connectToModel()
{
    connect(model, &QAbstractItemModel::modelReset, q, [this] { setModel(model, valueColumn); });
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

int Pie::Implementation::findSelectedSlice(const QPoint& point)
{
    for (decltype(slices)::size_type i = 0; i < slices.size(); ++i) {
        if (slices[i].path.contains(point)) {
            return i;
        }
    }

    return kInvalidIndex;
}

void Pie::Implementation::selectSlice(int _sliceIndex)
{
    if (selectedSlice == _sliceIndex) {
        return;
    }

    if (_sliceIndex == kInvalidIndex || _sliceIndex >= static_cast<int>(slices.size())) {
        selectedSlice = kInvalidIndex;
        animateDeselectedSlice();
        emit q->currentIndexChanged({});
        return;
    }

    auto isUpdate = false;
    if (selectedSlice != kInvalidIndex) {
        isUpdate = true;
    }

    selectedSlice = _sliceIndex;
    lastSelectedSlice = _sliceIndex;

    isUpdate ? q->update() : animateSelectedSlice();

    emit q->currentIndexChanged(slices[selectedSlice].index);
}


void Pie::Implementation::updateSelectedSlice()
{
    auto selectedSlice = findSelectedSlice(q->QWidget::mapFromGlobal(QCursor::pos()));
    if (selectedSlice == this->selectedSlice) {
        return;
    }

    if (selectedSlice != kInvalidIndex) {
        this->selectedSlice = selectedSlice;
        this->lastSelectedSlice = selectedSlice;

        emit q->currentIndexChanged(slices[selectedSlice].index);
    }

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
    const auto min = std::min(q->contentsRect().width(), q->contentsRect().height());
    const QRectF region
        = { { (q->contentsRect().width() - min) / 2.0, (q->contentsRect().height() - min) / 2.0 },
            QSize{ min, min } };
    const auto total = std::accumulate(
        slices.begin(), slices.end(), 0.0,
        [](const qreal a, const Implementation::Slice& b) { return a + b.value; });

    qreal start = 0.0;
    for (auto it = slices.begin(); it != slices.end(); ++it) {
        const auto length = (circle * it->value) / total;
        it->path = pieSlice(region, start, length, hole);

        start += length;
    }
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

Pie::~Pie() = default;

void Pie::setModel(QAbstractItemModel* _model, int _valueColumn)
{
    d->setModel(_model, _valueColumn);
}

void Pie::setCurrentItem(const QModelIndex& _index)
{
    d->selectSlice(_index.row());
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

QSize Pie::sizeHint() const
{
    return QSize(contentsMargins().left() + minimumWidth() + contentsMargins().right(),
                 contentsMargins().top() + minimumHeight() + contentsMargins().bottom());
}

void Pie::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    QPainter painter(this);
    painter.translate(contentsRect().topLeft());
    painter.setRenderHint(QPainter::Antialiasing);

    if (d->sliceOpacityAnimation.state() == QVariantAnimation::Running) {
        painter.setOpacity(d->sliceOpacityAnimation.currentValue().toReal());
    } else if (d->selectedSlice != kInvalidIndex) {
        painter.setOpacity(d->sliceOpacityAnimation.endValue().toReal());
    }

    for (int i = 0; d->lastSelectedSlice != kInvalidIndex && i < d->lastSelectedSlice; ++i) {
        painter.fillPath(d->slices[i].path, d->slices[i].color);
    }

    for (decltype(d->slices)::size_type i
         = d->lastSelectedSlice != kInvalidIndex ? d->lastSelectedSlice : 0;
         i < d->slices.size(); ++i) {
        painter.fillPath(d->slices[i].path, d->slices[i].color);
    }

    if (d->lastSelectedSlice != kInvalidIndex) {
        painter.setOpacity(1.0);
        painter.fillPath(d->slices[d->lastSelectedSlice].path,
                         d->slices[d->lastSelectedSlice].color);
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
    auto selectedSlice = d->findSelectedSlice(_event->pos() - contentsRect().topLeft());
    d->selectSlice(selectedSlice);
}
