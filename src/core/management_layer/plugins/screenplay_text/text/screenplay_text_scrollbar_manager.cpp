#include "screenplay_text_scrollbar_manager.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>

#include <ui/design_system/design_system.h>

#include <utils/helpers/color_helper.h>
#include <utils/helpers/time_helper.h>

#include <QApplication>
#include <QPainter>
#include <QResizeEvent>


namespace Ui
{

class ScreenplayTextScrollBarManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    void updateTimelineGeometry();

    BusinessLayer::ScreenplayTextModel* model = nullptr;
    QScrollBar* scrollbar = nullptr;
    ScreenplayTextTimeline* timeline = nullptr;
};

ScreenplayTextScrollBarManager::Implementation::Implementation(QWidget* _parent)
    : scrollbar(new QScrollBar(_parent)),
      timeline(new ScreenplayTextTimeline(_parent))
{
}

void ScreenplayTextScrollBarManager::Implementation::updateTimelineGeometry()
{
    timeline->move(timeline->parentWidget()->size().width() - timeline->width(), 0);
    timeline->resize(timeline->sizeHint().width(), timeline->parentWidget()->size().height());
}


// **


ScreenplayTextScrollBarManager::ScreenplayTextScrollBarManager(QAbstractScrollArea* _parent)
    : QObject(_parent),
      d(new Implementation(_parent))
{
    Q_ASSERT(_parent);

    _parent->setVerticalScrollBar(d->scrollbar);
    _parent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _parent->installEventFilter(this);

    d->timeline->installEventFilter(this);
    d->updateTimelineGeometry();
}

void ScreenplayTextScrollBarManager::initScrollBarsSyncing()
{
    connect(d->scrollbar, &QScrollBar::rangeChanged, this, [this] (int _minimum, int _maximum) {
        d->timeline->setScrollable(_minimum < _maximum);
    });
    auto updateTimelineValue = [this] {
        QSignalBlocker signalBlocker(d->timeline);
        if (d->scrollbar->maximum() == 0) {
            d->timeline->setValue({});
        } else {
            const qreal scrollBarValue = d->scrollbar->value();
            const qreal scrollBarMaximum = d->scrollbar->maximum();
            const auto value = d->model->duration() * scrollBarValue / scrollBarMaximum;
            d->timeline->setValue(std::chrono::duration_cast<std::chrono::milliseconds>(value));
        }
    };
    connect(d->scrollbar, &QScrollBar::valueChanged, this, updateTimelineValue);
    connect(d->timeline, &ScreenplayTextTimeline::valueChanged, this, [this] (std::chrono::milliseconds _value) {
        d->scrollbar->setValue(d->scrollbar->maximum() * _value / std::chrono::duration_cast<std::chrono::milliseconds>(d->model->duration()));
    });
    connect(d->timeline, &ScreenplayTextTimeline::updateValueRequested, this, updateTimelineValue);
}

ScreenplayTextScrollBarManager::~ScreenplayTextScrollBarManager() = default;

void ScreenplayTextScrollBarManager::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    if (d->model == _model) {
        return;
    }

    if (d->model) {
        d->model->disconnect(this);
    }

    d->model = _model;

    if (d->model) {
        auto updateTimeline = [this] { d->timeline->setMaximum(d->model->duration()); };
        connect(d->model, &QAbstractItemModel::rowsInserted, this, updateTimeline);
        connect(d->model, &QAbstractItemModel::rowsRemoved, this, updateTimeline);
        connect(d->model, &QAbstractItemModel::dataChanged, this, updateTimeline);
        updateTimeline();
    } else {
        d->timeline->update();
    }
}

bool ScreenplayTextScrollBarManager::eventFilter(QObject* _watched, QEvent* _event)
{
    if ((_watched == d->timeline
         || _watched == d->timeline->parentWidget())
            && (_event->type() == QEvent::Resize
                || _event->type() == QEvent::Show)) {
        d->updateTimelineGeometry();
    }

    return QObject::eventFilter(_watched, _event);
}


// ****


class ScreenplayTextTimeline::Implementation {
public:
    bool scrollable = true;
    const std::chrono::milliseconds minimum = std::chrono::seconds{0};
    std::chrono::milliseconds maximum = std::chrono::seconds{10};
    std::chrono::milliseconds current = std::chrono::seconds{5};
};


// **


ScreenplayTextTimeline::ScreenplayTextTimeline(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
}

ScreenplayTextTimeline::~ScreenplayTextTimeline() = default;

void ScreenplayTextTimeline::setScrollable(bool _scrollable)
{
    if (d->scrollable == _scrollable) {
        return;
    }

    d->scrollable = _scrollable;
    update();
}

void ScreenplayTextTimeline::setMaximum(std::chrono::seconds _maximum)
{
    if (d->maximum == _maximum) {
        return;
    }

    d->maximum = _maximum;
    emit updateValueRequested();
    if (d->current > d->maximum) {
        d->current = d->maximum;
    }

    update();
}

void ScreenplayTextTimeline::setValue(std::chrono::milliseconds _value)
{
    if (d->minimum > _value || _value > d->maximum
        || d->current == _value) {
        return;
    }

    d->current = _value;
    emit valueChanged(d->current);
    update();
}

QSize ScreenplayTextTimeline::sizeHint() const
{
    const QSize marginsDelta = QSizeF(Ui::DesignSystem::scrollBar().margins().left()
                                      + Ui::DesignSystem::scrollBar().margins().right(),
                                      Ui::DesignSystem::scrollBar().margins().top()
                                      + Ui::DesignSystem::scrollBar().margins().bottom()).toSize();
    return QSize(static_cast<int>(Ui::DesignSystem::layout().px48() + Ui::DesignSystem::layout().px16()),
                 10) + marginsDelta;
}

void ScreenplayTextTimeline::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::transparent);

    painter.setFont(Ui::DesignSystem::font().caption());

    const QRectF contentRect = rect();

    //
    // Рисуем фон скролбара
    // 16 - скролбар с ползунком 4 + 8 + 4
    // 48 - правая часть от скролбара с таймлайном
    //
    const QRectF scrollbarRect(Ui::DesignSystem::layout().px4(), 0,
                               Ui::DesignSystem::layout().px8(), contentRect.height());
    painter.fillRect(scrollbarRect, Ui::DesignSystem::color().background());
    const QRectF scrollbarBackgroundRect(scrollbarRect.right(), scrollbarRect.top(),
                                         Ui::DesignSystem::layout().px62(), scrollbarRect.height());
    painter.fillRect(scrollbarBackgroundRect, Ui::DesignSystem::color().surface());

    //
    // Рисуем хэндл
    //
    const qreal handleX = scrollbarRect.center().x();
    const qreal handleY = (height() - painter.fontMetrics().lineSpacing()) * d->current / d->maximum
                          + painter.fontMetrics().lineSpacing() / 2;
    const qreal handleSize = Ui::DesignSystem::layout().px12();
    const QRectF handleRect = QRectF(handleX - handleSize / 2, handleY - handleSize / 2,
                                     handleSize, handleSize);
    if (d->scrollable) {
        painter.setPen(QPen(Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::layout().px2()));
        painter.drawEllipse(handleRect);
    }

    //
    // Считаем область для отрисовки метки хэндла
    //
    const qreal handleTextLeft = handleRect.right() + Ui::DesignSystem::layout().px8();
    const QRectF handleTextRect(handleTextLeft, handleRect.top(),
                                contentRect.width() - handleTextLeft, handleRect.height());

    //
    // Рисуем метки на таймлайне
    //
    painter.setPen(ColorHelper::transparent(Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::disabledTextOpacity()));
    const qreal marksSpacing = painter.fontMetrics().lineSpacing() * 4;
    const int marksCount = (height() - painter.fontMetrics().lineSpacing()) / marksSpacing;
    const qreal marksSpacingCorrected = static_cast<qreal>(height() - painter.fontMetrics().lineSpacing()) / marksCount;
    qreal top = 0.0;
    for (int markIndex = 0; markIndex <= marksCount; ++markIndex) {
        const QRectF markTextRect(handleTextLeft, top,
                                  contentRect.width() - handleTextLeft, painter.fontMetrics().lineSpacing());
        const auto duartionAtMark = d->maximum * (static_cast<qreal>(markIndex) / marksCount);
        if (d->scrollable
            && markTextRect.intersects(handleTextRect)) {
            painter.setOpacity(Ui::DesignSystem::focusBackgroundOpacity());
        }
        painter.drawText(markTextRect, Qt::AlignLeft | Qt::AlignVCenter,
                         TimeHelper::toString(std::chrono::duration_cast<std::chrono::seconds>(duartionAtMark)));
        if (markTextRect.intersects(handleTextRect)) {
            painter.setOpacity(1.0);
        }

        top += marksSpacingCorrected;
    }

    //
    // Рисуем метку хэндла
    //
    if (d->scrollable) {
        painter.setPen(Ui::DesignSystem::color().onBackground());
        painter.drawText(handleTextRect, Qt::AlignLeft | Qt::AlignVCenter,TimeHelper::toString(std::chrono::duration_cast<std::chrono::seconds>(d->current)));
    }
}

void ScreenplayTextTimeline::mousePressEvent(QMouseEvent* _event)
{
    if (!d->scrollable) {
        return;
    }

    if (_event->buttons().testFlag(Qt::NoButton)) {
        return;
    }

    updateValue(_event->pos());
}

void ScreenplayTextTimeline::mouseMoveEvent(QMouseEvent* _event)
{
    if (!d->scrollable) {
        return;
    }

    if (_event->buttons().testFlag(Qt::NoButton)) {
        return;
    }

    updateValue(_event->pos());
}

void ScreenplayTextTimeline::mouseReleaseEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);

    update();
}

void ScreenplayTextTimeline::wheelEvent(QWheelEvent* _event)
{
    if (!d->scrollable) {
        return;
    }

    auto scrollArea = qobject_cast<QAbstractScrollArea*>(parentWidget());
    if (scrollArea == nullptr) {
        return;
    }

    auto cloneEvent = new QWheelEvent(_event->pos(), _event->delta(), _event->buttons(), _event->modifiers(), _event->orientation());
    QApplication::postEvent(scrollArea->verticalScrollBar(), cloneEvent);
}

void ScreenplayTextTimeline::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    resize(sizeHint().width(), height());
}

void ScreenplayTextTimeline::updateValue(const QPoint& _mousePosition)
{
    const qreal trackHeight = contentsRect().height();
    const qreal mousePosition = _mousePosition.y() - contentsMargins().left();
    const auto value = d->maximum * mousePosition / trackHeight;
    setValue(qBound(d->minimum, std::chrono::duration_cast<std::chrono::milliseconds>(value), d->maximum));
}

} // namespace Ui
