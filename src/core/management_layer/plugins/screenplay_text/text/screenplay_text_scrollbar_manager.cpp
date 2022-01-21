#include "screenplay_text_scrollbar_manager.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/time_helper.h>

#include <QApplication>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>
#include <QTimer>
#include <QVariantAnimation>


namespace Ui {

class ScreenplayTextScrollBarManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Скорректировать положение таймлайна в зависимости от родительского виджета
     */
    void updateTimelineGeometry();

    /**
     * @brief Запустить анимацию отображения таймлайна
     */
    void showTimelineAnimated();


    QPointer<BusinessLayer::ScreenplayTextModel> model;

    QScrollBar* scrollbar = nullptr;

    ScreenplayTextTimeline* timeline = nullptr;
    bool needAnimateTimelineOpacity = false;
    QTimer timelineHideTimer;
    QVariantAnimation timelineOpacityAnimation;
};

ScreenplayTextScrollBarManager::Implementation::Implementation(QWidget* _parent)
    : scrollbar(new QScrollBar(_parent))
    , timeline(new ScreenplayTextTimeline(_parent))
{
    timelineHideTimer.setSingleShot(true);
    timelineHideTimer.setInterval(2000);
    timelineOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    timelineOpacityAnimation.setDuration(240);
    timelineOpacityAnimation.setStartValue(1.0);
    timelineOpacityAnimation.setEndValue(0.0);
}

void ScreenplayTextScrollBarManager::Implementation::updateTimelineGeometry()
{
    timeline->move(timeline->parentWidget()->size().width() - timeline->width(), 0);
    timeline->resize(timeline->sizeHint().width(), timeline->parentWidget()->size().height());
    scrollbar->setFixedWidth(timeline->width());
}

void ScreenplayTextScrollBarManager::Implementation::showTimelineAnimated()
{
    if (timelineOpacityAnimation.direction() == QVariantAnimation::Backward) {
        return;
    }

    if (timelineOpacityAnimation.state() == QVariantAnimation::Running) {
        timelineOpacityAnimation.pause();
    }
    timelineOpacityAnimation.setDirection(QVariantAnimation::Backward);
    if (timelineOpacityAnimation.state() == QVariantAnimation::Paused) {
        timelineOpacityAnimation.resume();
    } else {
        timelineOpacityAnimation.start();
    }
}


// **


ScreenplayTextScrollBarManager::ScreenplayTextScrollBarManager(QAbstractScrollArea* _parent)
    : QObject(_parent)
    , d(new Implementation(_parent))
{
    Q_ASSERT(_parent);

    _parent->setVerticalScrollBar(d->scrollbar);
    _parent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _parent->installEventFilter(this);

    d->timeline->installEventFilter(this);
    d->updateTimelineGeometry();

    connect(&d->timelineHideTimer, &QTimer::timeout, this, [this] {
        d->timelineOpacityAnimation.setDirection(QVariantAnimation::Forward);
        d->timelineOpacityAnimation.start();
    });
    connect(&d->timelineOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { d->timeline->setOpacity(_value.toReal()); });
}

ScreenplayTextScrollBarManager::~ScreenplayTextScrollBarManager() = default;

void ScreenplayTextScrollBarManager::initScrollBarsSyncing()
{
    connect(d->scrollbar, &QScrollBar::rangeChanged, this, [this](int _minimum, int _maximum) {
        d->timeline->setScrollable(_minimum < _maximum);
    });
    auto updateTimelineValue = [this] {
        if (d->model == nullptr) {
            return;
        }

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
    connect(d->timeline, &ScreenplayTextTimeline::valueChanged, this,
            [this](std::chrono::milliseconds _value) {
                d->scrollbar->setValue(
                    d->scrollbar->maximum() * _value
                    / std::chrono::duration_cast<std::chrono::milliseconds>(d->model->duration()));
            });
    connect(d->timeline, &ScreenplayTextTimeline::updateValueRequested, this, updateTimelineValue);
}

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
        auto updateTimeline = [this] {
            d->timeline->setMaximum(d->model->duration());
            d->timeline->setColors(d->model->itemsColors());
        };
        connect(d->model, &QAbstractItemModel::rowsInserted, this, updateTimeline,
                Qt::QueuedConnection);
        connect(d->model, &QAbstractItemModel::rowsRemoved, this, updateTimeline,
                Qt::QueuedConnection);
        connect(d->model, &QAbstractItemModel::dataChanged, this, updateTimeline,
                Qt::QueuedConnection);
        updateTimeline();
    } else {
        d->timeline->update();
    }
}

void ScreenplayTextScrollBarManager::setScrollBarVisible(bool _visible)
{
    if (_visible) {
        d->needAnimateTimelineOpacity = false;
        d->showTimelineAnimated();
    } else {
        d->needAnimateTimelineOpacity = true;
        d->timelineHideTimer.start();
    }
}

bool ScreenplayTextScrollBarManager::eventFilter(QObject* _watched, QEvent* _event)
{
    //
    // При отображении и при изменении размера родителя, корректируем позицию таймлайна
    //
    if ((_watched == d->timeline || _watched == d->timeline->parentWidget())
        && (_event->type() == QEvent::Resize || _event->type() == QEvent::Show)) {
        d->updateTimelineGeometry();
    }
    //
    // Если курсор мыши вошёл в таймлайн, то останавливаем таймер скрытия, а если вышел - запускаем
    //
    else if (d->needAnimateTimelineOpacity && _watched == d->timeline) {
        if (_event->type() == QEvent::Enter) {
            d->timelineHideTimer.stop();
            d->showTimelineAnimated();
        } else if (_event->type() == QEvent::Leave) {
            d->timelineHideTimer.start();
        }
    }

    return QObject::eventFilter(_watched, _event);
}


// ****


class ScreenplayTextTimeline::Implementation
{
public:
    bool scrollable = true;
    const std::chrono::milliseconds minimum = std::chrono::seconds{ 0 };
    std::chrono::milliseconds maximum = std::chrono::seconds{ 10 };
    std::chrono::milliseconds current = std::chrono::seconds{ 5 };
    std::map<std::chrono::milliseconds, QColor> colors;
};


// **


ScreenplayTextTimeline::ScreenplayTextTimeline(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
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

void ScreenplayTextTimeline::setMaximum(std::chrono::milliseconds _maximum)
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
    if (d->minimum > _value || _value > d->maximum || d->current == _value) {
        return;
    }

    d->current = _value;
    emit valueChanged(d->current);
    update();
}

void ScreenplayTextTimeline::setColors(const std::map<std::chrono::milliseconds, QColor>& _colors)
{
    if (d->colors == _colors) {
        return;
    }

    d->colors = _colors;
    update();
}

QSize ScreenplayTextTimeline::sizeHint() const
{
    const QSize marginsDelta = QSizeF(Ui::DesignSystem::scrollBar().margins().left()
                                          + Ui::DesignSystem::scrollBar().margins().right(),
                                      Ui::DesignSystem::scrollBar().margins().top()
                                          + Ui::DesignSystem::scrollBar().margins().bottom())
                                   .toSize();
    return QSize(static_cast<int>(Ui::DesignSystem::layout().px48()
                                  + Ui::DesignSystem::layout().px16()),
                 10)
        + marginsDelta;
}

void ScreenplayTextTimeline::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(opacity());
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
    const auto scrollbarColor = ColorHelper::nearby(Ui::DesignSystem::color().surface());
    painter.fillRect(scrollbarRect, scrollbarColor);
    //
    // Рисуем дополнительные цвета скролбара
    //
    if (d->colors.size() > 0) {
        painter.setOpacity(opacity() * DesignSystem::inactiveTextOpacity());

        auto startDuration = d->colors.begin()->first;
        auto endDuration = startDuration;
        QColor color = d->colors.begin()->second;
        auto paintColor = [this, &painter, scrollbarRect, &startDuration, &endDuration, &color] {
            if (!color.isValid()) {
                return;
            }

            const QRectF colorRect(
                QPointF(scrollbarRect.left(),
                        (height() - painter.fontMetrics().lineSpacing()) * startDuration
                                / d->maximum
                            + painter.fontMetrics().lineSpacing() / 2),
                QPointF(scrollbarRect.right(),
                        (height() - painter.fontMetrics().lineSpacing()) * endDuration / d->maximum
                            + painter.fontMetrics().lineSpacing() / 2));
            painter.fillRect(colorRect, color);
        };
        for (auto iter = std::next(d->colors.begin()); iter != d->colors.end(); ++iter) {
            endDuration = iter->first;
            paintColor();

            startDuration = endDuration;
            color = iter->second;
        }

        endDuration = d->maximum;
        paintColor();

        painter.setOpacity(opacity());
    }
    const QRectF scrollbarBackgroundRect(scrollbarRect.right(), scrollbarRect.top(),
                                         Ui::DesignSystem::layout().px62(), scrollbarRect.height());
    painter.fillRect(scrollbarBackgroundRect, Ui::DesignSystem::color().surface());

    //
    // Рисуем хэндл
    //
    const qreal handleX = scrollbarRect.center().x();
    const qreal handleY = d->maximum > std::chrono::milliseconds{ 0 }
        ? ((height() - painter.fontMetrics().lineSpacing()) * d->current / d->maximum
           + painter.fontMetrics().lineSpacing() / 2)
        : 0;
    const qreal handleSize = Ui::DesignSystem::layout().px12();
    const QRectF handleRect
        = QRectF(handleX - handleSize / 2, handleY - handleSize / 2, handleSize, handleSize);
    if (d->scrollable) {
        painter.setPen(
            QPen(Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::layout().px2()));
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
    const auto tickRight = scrollbarRect.right() + (handleTextLeft - scrollbarRect.right()) / 2;
    const auto markColor = ColorHelper::transparent(Ui::DesignSystem::color().onBackground(),
                                                    Ui::DesignSystem::disabledTextOpacity());
    const auto markWidth = contentRect.width() - handleTextLeft;
    const qreal marksSpacing = painter.fontMetrics().lineSpacing() * 4;
    const int marksCount = (height() - painter.fontMetrics().lineSpacing()) / marksSpacing;
    const qreal marksSpacingCorrected
        = static_cast<qreal>(height() - painter.fontMetrics().lineSpacing()) / marksCount;
    qreal top = 0.0;
    for (int markIndex = 0; markIndex <= marksCount; ++markIndex) {
        const QRectF markTextRect(handleTextLeft, top, markWidth,
                                  painter.fontMetrics().lineSpacing());
        const auto duartionAtMark = d->maximum * (static_cast<qreal>(markIndex) / marksCount);
        if (d->scrollable && markTextRect.intersects(handleTextRect)) {
            painter.setOpacity(opacity() * Ui::DesignSystem::focusBackgroundOpacity());
        }
        painter.setPen(QPen(scrollbarColor, Ui::DesignSystem::layout().px2()));
        painter.drawLine(scrollbarRect.right(), markTextRect.center().y(), tickRight,
                         markTextRect.center().y());
        painter.setPen(markColor);
        painter.drawText(
            markTextRect, Qt::AlignLeft | Qt::AlignVCenter,
            TimeHelper::toString(std::chrono::duration_cast<std::chrono::seconds>(duartionAtMark)));
        if (markTextRect.intersects(handleTextRect)) {
            painter.setOpacity(opacity());
        }

        top += marksSpacingCorrected;
    }

    //
    // Рисуем метку хэндла
    //
    if (d->scrollable) {
        painter.setPen(Ui::DesignSystem::color().onBackground());
        painter.drawText(
            handleTextRect, Qt::AlignLeft | Qt::AlignVCenter,
            TimeHelper::toString(std::chrono::duration_cast<std::chrono::seconds>(d->current)));
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

    auto cloneEvent
        = new QWheelEvent(_event->position(), _event->globalPosition(), _event->pixelDelta(),
                          _event->angleDelta(), _event->buttons(), _event->modifiers(),
                          _event->phase(), _event->inverted(), _event->source());
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
    setValue(qBound(d->minimum, std::chrono::duration_cast<std::chrono::milliseconds>(value),
                    d->maximum));
}

} // namespace Ui
