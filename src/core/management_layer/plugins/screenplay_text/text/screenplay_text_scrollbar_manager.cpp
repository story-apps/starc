#include "screenplay_text_scrollbar_manager.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/time_helper.h>
#include <utils/tools/debouncer.h>

#include <QAbstractScrollArea>
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
    explicit Implementation(QAbstractScrollArea* _parent);

    /**
     * @brief Скорректировать положение таймлайна в зависимости от родительского виджета
     */
    void updateTimelineGeometry();

    /**
     * @brief Обновить размер отображаемой области таймлайна
     */
    void updateTimelineDisplayRange();

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

    Debouncer itemsColorsUpdateDebouncer;
};

ScreenplayTextScrollBarManager::Implementation::Implementation(QAbstractScrollArea* _parent)
    : scrollbar(_parent->verticalScrollBar())
    , timeline(new ScreenplayTextTimeline(_parent))
    , itemsColorsUpdateDebouncer(180)
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
    timeline->move(0, timeline->parentWidget()->size().height() - timeline->height());
    timeline->resize(timeline->parentWidget()->size().width(), timeline->sizeHint().height());

    updateTimelineDisplayRange();
}

void ScreenplayTextScrollBarManager::Implementation::updateTimelineDisplayRange()
{
    if (model.isNull() || scrollbar->maximum() == 0) {
        return;
    }

    //
    // В pageStep текстовый документ задаёт высоту видимой области текста,
    // а в maximum всю высоту документа за вычетом одной страницы,
    // поэтому тут рассчёт идёт исходя из этого знания
    //
    timeline->setDisplayRange(model->duration() * scrollbar->pageStep()
                              / (scrollbar->maximum() + scrollbar->pageStep()));
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

    _parent->installEventFilter(this);

    d->timeline->installEventFilter(this);
    d->updateTimelineGeometry();

    connect(&d->timelineHideTimer, &QTimer::timeout, this, [this] {
        d->timelineOpacityAnimation.setDirection(QVariantAnimation::Forward);
        d->timelineOpacityAnimation.start();
    });
    connect(&d->timelineOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { d->timeline->setOpacity(_value.toReal()); });
    connect(&d->itemsColorsUpdateDebouncer, &Debouncer::gotWork, this, [this] {
        if (d->model == nullptr) {
            return;
        }

        d->timeline->setMaximum(d->model->duration());
        d->timeline->setColors(d->model->itemsColors());
        d->timeline->setBookmarks(d->model->itemsBookmarks());
        d->updateTimelineDisplayRange();
    });
}

ScreenplayTextScrollBarManager::~ScreenplayTextScrollBarManager() = default;

void ScreenplayTextScrollBarManager::initScrollBarsSyncing()
{
    connect(d->scrollbar, &QScrollBar::rangeChanged, this, [this](int _minimum, int _maximum) {
        d->timeline->setScrollable(_minimum < _maximum);
        d->updateTimelineDisplayRange();
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
        d->model->disconnect(&d->itemsColorsUpdateDebouncer);
    }

    d->model = _model;

    if (d->model) {
        connect(d->model, &QAbstractItemModel::rowsInserted, &d->itemsColorsUpdateDebouncer,
                &Debouncer::orderWork);
        connect(d->model, &QAbstractItemModel::rowsRemoved, &d->itemsColorsUpdateDebouncer,
                &Debouncer::orderWork);
        connect(d->model, &QAbstractItemModel::dataChanged, &d->itemsColorsUpdateDebouncer,
                &Debouncer::orderWork);
        d->itemsColorsUpdateDebouncer.orderWork();
    } else {
        d->timeline->update();
    }
}

void ScreenplayTextScrollBarManager::setScrollBarVisible(bool _visible, bool _animate)
{
    if (!_animate) {
        d->timeline->setVisible(_visible);
        return;
    }

    if (_visible) {
        d->needAnimateTimelineOpacity = false;
        d->showTimelineAnimated();
        d->timelineHideTimer.stop();
    } else {
        d->needAnimateTimelineOpacity = true;
        d->timelineHideTimer.start();
    }
}

int ScreenplayTextScrollBarManager::scrollBarWidth() const
{
    return d->timeline->width();
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
    std::chrono::milliseconds current = std::chrono::seconds{ 0 };
    std::chrono::milliseconds displayRange = std::chrono::seconds{ 5 };
    std::map<std::chrono::milliseconds, QColor> colors;
    std::map<std::chrono::milliseconds, QColor> bookmarks;
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

void ScreenplayTextTimeline::setDisplayRange(std::chrono::milliseconds _value)
{
    if (d->minimum > _value || _value > d->maximum || d->displayRange == _value) {
        return;
    }

    d->displayRange = _value;
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

void ScreenplayTextTimeline::setBookmarks(
    const std::map<std::chrono::milliseconds, QColor>& _bookmarks)
{
    if (d->bookmarks == _bookmarks) {
        return;
    }

    d->bookmarks = _bookmarks;
    update();
}

QSize ScreenplayTextTimeline::sizeHint() const
{
    const QSize marginsDelta = QSizeF(DesignSystem::scrollBar().margins().left()
                                          + DesignSystem::scrollBar().margins().right(),
                                      DesignSystem::scrollBar().margins().top()
                                          + DesignSystem::scrollBar().margins().bottom())
                                   .toSize();
    return QSize(10, static_cast<int>(DesignSystem::layout().px(56))) + marginsDelta;
}

void ScreenplayTextTimeline::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(opacity());
    painter.fillRect(_event->rect(), Qt::transparent);

    painter.setFont(DesignSystem::font().caption());

    const QRectF contentRect = rect().adjusted(0, 0, -DesignSystem::scrollBar().minimumSize(), 0);

    //
    // Рисуем фон скролбара
    // 16 - скролбар с ползунком 4 + 8 + 4
    // 48 - нижняя часть от скролбара с таймлайном
    //
    const QRectF scrollbarRect(0, DesignSystem::layout().px16(), contentRect.width(),
                               DesignSystem::layout().px8());
    const auto scrollbarColor = ColorHelper::nearby(DesignSystem::color().surface());
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
            if (!color.isValid() || d->maximum.count() == 0) {
                return;
            }

            const QRectF colorRect(
                QPointF((scrollbarRect.width() - painter.fontMetrics().lineSpacing())
                                * startDuration / d->maximum
                            + painter.fontMetrics().lineSpacing() / 2,
                        scrollbarRect.top()),
                QPointF((scrollbarRect.width() - painter.fontMetrics().lineSpacing()) * endDuration
                                / d->maximum
                            + painter.fontMetrics().lineSpacing() / 2,
                        scrollbarRect.bottom()));
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

    //
    // Рисуем закладки
    //
    for (auto iter = d->bookmarks.begin(); iter != d->bookmarks.end(); ++iter) {
        const auto color = iter->second;
        painter.setPen(
            QPen(color, DesignSystem::layout().px2(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(color);

        //
        // Рисуем линию
        //
        const auto duration = iter->first;
        const QPointF top((contentRect.width() - painter.fontMetrics().lineSpacing()) * duration
                                  / d->maximum
                              + painter.fontMetrics().lineSpacing() / 2,
                          0);
        const QPointF bottom(top.x(), scrollbarRect.bottom());
        const QLineF colorRect(top, bottom);
        painter.drawLine(colorRect);
        //
        // и наконечник
        //
        QPolygonF treangle;
        treangle << QPointF(top.x() - DesignSystem::layout().px4(), top.y())
                 << QPointF(top.x(), top.y() + DesignSystem::layout().px8())
                 << QPointF(top.x() + DesignSystem::layout().px4(), top.y());
        painter.drawPolygon(treangle);
    }
    painter.setBrush(Qt::NoBrush);

    //
    // Заливаем нижнюю часть фона
    //
    const QRectF scrollbarBackgroundRect(0, scrollbarRect.bottom(), scrollbarRect.width(),
                                         DesignSystem::layout().px(56));
    painter.fillRect(scrollbarBackgroundRect, DesignSystem::color().surface());

    //
    // Рассчитываем хэндл
    //
    const qreal handleX = d->maximum > std::chrono::milliseconds{ 0 }
        ? ((contentRect.width() - painter.fontMetrics().lineSpacing()) * d->current / d->maximum
           + painter.fontMetrics().lineSpacing() / 2)
        : 0;
    const qreal handleY = scrollbarRect.center().y();
    const qreal handleSize = DesignSystem::layout().px12();
    QRectF handleRect(handleX - handleSize / 2, handleY - handleSize / 2, handleSize, handleSize);
    QRectF handleEllipseRect;
    std::chrono::duration<float> handleLeftValue = std::chrono::milliseconds{ 0 };
    if (d->scrollable && d->maximum > std::chrono::milliseconds{ 0 }) {
        handleLeftValue
            = d->current - d->current / std::chrono::duration<float>(d->maximum) * d->displayRange;
        const qreal handleLeft = contentRect.width() * handleLeftValue / d->maximum;
        const qreal handleRight
            = contentRect.width() * (handleLeftValue + d->displayRange) / d->maximum;
        const auto handleWidth = handleRight - handleLeft - DesignSystem::layout().px() * 2;
        const auto minimumWidth = painter.fontMetrics().lineSpacing() * 2.0;
        if (handleWidth >= minimumWidth) {
            handleRect = QRectF(handleLeft + DesignSystem::layout().px(),
                                scrollbarRect.top() - DesignSystem::layout().px(), handleWidth,
                                scrollbarRect.height() + DesignSystem::layout().px() * 2);
        } else {
            handleEllipseRect = handleRect;

            const auto widthDelta = (minimumWidth - handleRect.width()) / 2.0;
            handleRect.adjust(-widthDelta, 0, widthDelta, 0);
            if (handleRect.left() < 0) {
                handleRect.moveLeft(0);
            } else if (handleRect.right() > contentRect.width()) {
                handleRect.moveRight(contentRect.width());
            }
        }
    }

    //
    // Считаем область для отрисовки метки хэндла
    //
    const qreal handleTextTop = handleRect.top() + DesignSystem::layout().px16();
    const qreal handleTextWidth = DesignSystem::layout().px(36);
    QRectF handleLeftTextRect;
    QRectF handleRightTextRect;
    if (handleRect.width() > handleTextWidth * 2 + DesignSystem::layout().px8()) {
        handleLeftTextRect = QRectF(handleRect.left(), handleTextTop, handleTextWidth,
                                    painter.fontMetrics().lineSpacing());
        handleRightTextRect = QRectF(handleRect.right() - handleTextWidth, handleTextTop,
                                     handleTextWidth, painter.fontMetrics().lineSpacing());
    } else {
        const auto sideDelta
            = (handleTextWidth * 2 + DesignSystem::layout().px8() - handleRect.width()) / 2;
        handleLeftTextRect = QRectF(handleRect.left() - sideDelta, handleTextTop, handleTextWidth,
                                    painter.fontMetrics().lineSpacing());
        handleRightTextRect
            = QRectF(handleRect.right() - handleTextWidth + sideDelta, handleTextTop,
                     handleTextWidth, painter.fontMetrics().lineSpacing());

        if (handleLeftTextRect.left() < 0) {
            handleLeftTextRect.moveLeft(0);
            handleRightTextRect.moveLeft(handleLeftTextRect.right() + DesignSystem::layout().px8());
        } else if (handleRightTextRect.right() > contentRect.width()) {
            handleRightTextRect.moveRight(contentRect.width());
            handleLeftTextRect.moveLeft(handleRightTextRect.left() - DesignSystem::layout().px8()
                                        - handleTextWidth);
        }
    }

    //
    // Рисуем метки на таймлайне
    //
    const auto tickHeight = DesignSystem::layout().px4();
    const auto markColor = ColorHelper::transparent(DesignSystem::color().onSurface(),
                                                    DesignSystem::disabledTextOpacity());
    const qreal marksSpacing = handleTextWidth * 1.4;
    const int marksCount = (contentRect.width() - handleTextWidth) / marksSpacing;
    const qreal marksSpacingCorrected
        = static_cast<qreal>(contentRect.width() - handleTextWidth) / marksCount;
    qreal left = 0.0;
    for (int markIndex = 0; markIndex <= marksCount; ++markIndex) {
        const QRectF markTextRect(left, handleTextTop, handleTextWidth,
                                  painter.fontMetrics().lineSpacing());
        const auto duartionAtMark = d->maximum * (static_cast<qreal>(markIndex) / marksCount);
        if (d->scrollable
            && (markTextRect.intersects(handleLeftTextRect)
                || markTextRect.intersects(handleRightTextRect))) {
            painter.setOpacity(opacity() * DesignSystem::focusBackgroundOpacity());
        }
        painter.setPen(QPen(scrollbarColor, DesignSystem::layout().px2()));
        painter.drawLine(markTextRect.center().x(), scrollbarRect.bottom(),
                         markTextRect.center().x(), scrollbarRect.bottom() + tickHeight);
        painter.setPen(markColor);
        painter.drawText(
            markTextRect, Qt::AlignCenter,
            TimeHelper::toString(std::chrono::duration_cast<std::chrono::seconds>(duartionAtMark)));
        if (markTextRect.intersects(handleLeftTextRect)
            || markTextRect.intersects(handleRightTextRect)) {
            painter.setOpacity(opacity());
        }

        left += marksSpacingCorrected;
    }

    //
    // Рисуем хэндл
    //
    if (d->scrollable) {
        painter.setPen(Ui::DesignSystem::color().onSurface());
        //
        // ... область
        //
        if (handleEllipseRect.isValid()) {
            painter.drawEllipse(handleEllipseRect);
        } else {
            painter.drawRoundedRect(handleRect, DesignSystem::layout().px2(),
                                    DesignSystem::layout().px2());
        }
        //
        // ... метка
        //
        painter.drawText(handleLeftTextRect, Qt::AlignCenter,
                         TimeHelper::toString(
                             std::chrono::duration_cast<std::chrono::seconds>(handleLeftValue)));
        painter.drawText(handleRightTextRect, Qt::AlignCenter,
                         TimeHelper::toString(std::chrono::duration_cast<std::chrono::seconds>(
                             handleLeftValue + d->displayRange)));
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
    const qreal trackWidth = contentsRect().width();
    const qreal mousePosition = _mousePosition.x() - contentsMargins().left();
    const auto value = d->maximum * mousePosition / trackWidth;
    setValue(qBound(d->minimum, std::chrono::duration_cast<std::chrono::milliseconds>(value),
                    d->maximum));
}

} // namespace Ui
