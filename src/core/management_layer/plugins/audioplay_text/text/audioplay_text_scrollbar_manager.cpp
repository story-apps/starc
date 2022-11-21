#include "audioplay_text_scrollbar_manager.h"

#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/time_helper.h>
#include <utils/tools/debouncer.h>

#include <QApplication>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>
#include <QTimer>
#include <QVariantAnimation>


namespace Ui {

class AudioplayTextScrollBarManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

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


    QPointer<BusinessLayer::AudioplayTextModel> model;

    QScrollBar* scrollbar = nullptr;

    AudioplayTextTimeline* timeline = nullptr;
    bool needAnimateTimelineOpacity = false;
    QTimer timelineHideTimer;
    QVariantAnimation timelineOpacityAnimation;

    Debouncer itemsColorsUpdateDebouncer;
};

AudioplayTextScrollBarManager::Implementation::Implementation(QWidget* _parent)
    : scrollbar(new QScrollBar(_parent))
    , timeline(new AudioplayTextTimeline(_parent))
    , itemsColorsUpdateDebouncer(180)
{
    timelineHideTimer.setSingleShot(true);
    timelineHideTimer.setInterval(2000);
    timelineOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    timelineOpacityAnimation.setDuration(240);
    timelineOpacityAnimation.setStartValue(1.0);
    timelineOpacityAnimation.setEndValue(0.0);
}

void AudioplayTextScrollBarManager::Implementation::updateTimelineGeometry()
{
    timeline->move(QLocale().textDirection() == Qt::LeftToRight
                       ? (timeline->parentWidget()->size().width() - timeline->width())
                       : 0,
                   0);
    timeline->resize(timeline->sizeHint().width(), timeline->parentWidget()->size().height());
    scrollbar->setFixedWidth(timeline->width());

    updateTimelineDisplayRange();
}

void AudioplayTextScrollBarManager::Implementation::updateTimelineDisplayRange()
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

void AudioplayTextScrollBarManager::Implementation::showTimelineAnimated()
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


AudioplayTextScrollBarManager::AudioplayTextScrollBarManager(QAbstractScrollArea* _parent)
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

AudioplayTextScrollBarManager::~AudioplayTextScrollBarManager() = default;

void AudioplayTextScrollBarManager::initScrollBarsSyncing()
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
    connect(d->timeline, &AudioplayTextTimeline::valueChanged, this,
            [this](std::chrono::milliseconds _value) {
                d->scrollbar->setValue(
                    d->scrollbar->maximum() * _value
                    / std::chrono::duration_cast<std::chrono::milliseconds>(d->model->duration()));
            });
    connect(d->timeline, &AudioplayTextTimeline::updateValueRequested, this, updateTimelineValue);
}

void AudioplayTextScrollBarManager::setModel(BusinessLayer::AudioplayTextModel* _model)
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

void AudioplayTextScrollBarManager::setScrollBarVisible(bool _visible)
{
    if (_visible) {
        d->needAnimateTimelineOpacity = false;
        d->showTimelineAnimated();
        d->timelineHideTimer.stop();
    } else {
        d->needAnimateTimelineOpacity = true;
        d->timelineHideTimer.start();
    }
}

bool AudioplayTextScrollBarManager::eventFilter(QObject* _watched, QEvent* _event)
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


class AudioplayTextTimeline::Implementation
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


AudioplayTextTimeline::AudioplayTextTimeline(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
}

AudioplayTextTimeline::~AudioplayTextTimeline() = default;

void AudioplayTextTimeline::setScrollable(bool _scrollable)
{
    if (d->scrollable == _scrollable) {
        return;
    }

    d->scrollable = _scrollable;
    update();
}

void AudioplayTextTimeline::setMaximum(std::chrono::milliseconds _maximum)
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

void AudioplayTextTimeline::setValue(std::chrono::milliseconds _value)
{
    if (d->minimum > _value || _value > d->maximum || d->current == _value) {
        return;
    }

    d->current = _value;
    emit valueChanged(d->current);
    update();
}

void AudioplayTextTimeline::setDisplayRange(std::chrono::milliseconds _value)
{
    if (d->minimum > _value || _value > d->maximum || d->displayRange == _value) {
        return;
    }

    d->displayRange = _value;
    update();
}

void AudioplayTextTimeline::setColors(const std::map<std::chrono::milliseconds, QColor>& _colors)
{
    if (d->colors == _colors) {
        return;
    }

    d->colors = _colors;
    update();
}

void AudioplayTextTimeline::setBookmarks(
    const std::map<std::chrono::milliseconds, QColor>& _bookmarks)
{
    if (d->bookmarks == _bookmarks) {
        return;
    }

    d->bookmarks = _bookmarks;
    update();
}

QSize AudioplayTextTimeline::sizeHint() const
{
    const QSize marginsDelta = QSizeF(Ui::DesignSystem::scrollBar().margins().left()
                                          + Ui::DesignSystem::scrollBar().margins().right(),
                                      Ui::DesignSystem::scrollBar().margins().top()
                                          + Ui::DesignSystem::scrollBar().margins().bottom())
                                   .toSize();
    return QSize(static_cast<int>(Ui::DesignSystem::layout().px(60)
                                  + Ui::DesignSystem::layout().px16()),
                 10)
        + marginsDelta;
}

void AudioplayTextTimeline::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(opacity());
    painter.fillRect(_event->rect(), Qt::transparent);

    painter.setFont(Ui::DesignSystem::font().caption());

    const QRectF contentRect = rect();

    //
    // Рисуем фон скролбара
    // 16 - скролбар с ползунком 4 + 8 + 4
    // 48 - правая часть от скролбара с таймлайном
    //
    const QRectF scrollbarRect(isLeftToRight() ? Ui::DesignSystem::layout().px16()
                                               : (width() - Ui::DesignSystem::layout().px24()),
                               0, Ui::DesignSystem::layout().px8(), contentRect.height());
    const auto scrollbarColor = ColorHelper::nearby(Ui::DesignSystem::color().textEditor());
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

    //
    // Рисуем закладки
    //
    for (auto iter = d->bookmarks.begin(); iter != d->bookmarks.end(); ++iter) {
        const auto color = iter->second;
        painter.setPen(QPen(color, Ui::DesignSystem::layout().px2(), Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
        painter.setBrush(color);

        //
        // Рисуем линию
        //
        const auto duration = iter->first;
        QPointF left(isLeftToRight() ? 0 : scrollbarRect.left(),
                     (height() - painter.fontMetrics().lineSpacing()) * duration / d->maximum
                         + painter.fontMetrics().lineSpacing() / 2);
        const QPointF right(isLeftToRight() ? scrollbarRect.right() : width(), left.y());
        const QLineF colorRect(left, right);

        painter.drawLine(colorRect);
        //
        // и наконечник
        //
        QPolygonF treangle;
        if (isLeftToRight()) {
            treangle << QPointF(left.x(), left.y() - Ui::DesignSystem::layout().px4())
                     << QPointF(left.x() + Ui::DesignSystem::layout().px8(), left.y())
                     << QPointF(left.x(), left.y() + Ui::DesignSystem::layout().px4());
        } else {
            treangle << QPointF(right.x(), right.y() - Ui::DesignSystem::layout().px4())
                     << QPointF(right.x() - Ui::DesignSystem::layout().px8(), right.y())
                     << QPointF(right.x(), right.y() + Ui::DesignSystem::layout().px4());
        }
        painter.drawPolygon(treangle);
    }
    painter.setBrush(Qt::NoBrush);

    //
    // Заливаем правую часть фона
    //
    const QRectF scrollbarBackgroundRect(isLeftToRight() ? scrollbarRect.right() : 0.0,
                                         scrollbarRect.top(), Ui::DesignSystem::layout().px(56),
                                         scrollbarRect.height());
    painter.fillRect(scrollbarBackgroundRect, Ui::DesignSystem::color().surface());

    //
    // Рассчитываем хэндл
    //
    const qreal handleX = scrollbarRect.center().x();
    const qreal handleY = d->maximum > std::chrono::milliseconds{ 0 }
        ? ((height() - painter.fontMetrics().lineSpacing()) * d->current / d->maximum
           + painter.fontMetrics().lineSpacing() / 2)
        : 0;
    const qreal handleSize = Ui::DesignSystem::layout().px12();
    QRectF handleRect(handleX - handleSize / 2, handleY - handleSize / 2, handleSize, handleSize);
    QRectF handleEllipseRect;
    std::chrono::duration<float> handleTopValue = std::chrono::milliseconds{ 0 };
    if (d->scrollable && d->maximum > std::chrono::milliseconds{ 0 }) {
        handleTopValue
            = d->current - d->current / std::chrono::duration<float>(d->maximum) * d->displayRange;
        const qreal handleTop = height() * handleTopValue / d->maximum;
        const qreal handleBottom = height() * (handleTopValue + d->displayRange) / d->maximum;
        const auto handleHeight = handleBottom - handleTop - Ui::DesignSystem::layout().px() * 2;
        const auto minimumHeight = painter.fontMetrics().lineSpacing() * 2.0;
        if (handleHeight >= minimumHeight) {
            handleRect
                = QRectF(scrollbarRect.left() - Ui::DesignSystem::layout().px(),
                         handleTop + Ui::DesignSystem::layout().px(),
                         scrollbarRect.width() + Ui::DesignSystem::layout().px() * 2, handleHeight);
        } else {
            handleEllipseRect = handleRect;

            const auto heightDelta = (minimumHeight - handleRect.height()) / 2.0;
            handleRect.adjust(0, -heightDelta, 0, heightDelta);
            if (handleRect.top() < 0) {
                handleRect.moveTop(0);
            } else if (handleRect.bottom() > height()) {
                handleRect.moveBottom(height());
            }
        }
    }

    //
    // Считаем область для отрисовки метки хэндла
    //
    const qreal handleTextLeft
        = isLeftToRight() ? (handleRect.right() + Ui::DesignSystem::layout().px8()) : 0.0;
    const qreal handleTextWidth = isLeftToRight()
        ? (contentRect.width() - handleTextLeft)
        : handleRect.left() - Ui::DesignSystem::layout().px8();
    const QRectF handleTopTextRect(handleTextLeft, handleRect.top(), handleTextWidth,
                                   painter.fontMetrics().lineSpacing());
    const QRectF handleBottomTextRect(handleTextLeft,
                                      handleRect.bottom() - painter.fontMetrics().lineSpacing(),
                                      handleTextWidth, painter.fontMetrics().lineSpacing());

    //
    // Рисуем метки на таймлайне
    //
    const auto tickWidth = isLeftToRight()
        ? (scrollbarRect.right() + (handleTextLeft - scrollbarRect.right()) / 2)
        : handleTextLeft + handleTextWidth
            + ((scrollbarRect.left() - (handleTextLeft + handleTextWidth)) / 2);
    const auto markColor = ColorHelper::transparent(Ui::DesignSystem::color().onTextEditor(),
                                                    Ui::DesignSystem::disabledTextOpacity());
    const qreal marksSpacing = painter.fontMetrics().lineSpacing() * 4;
    const int marksCount = (height() - painter.fontMetrics().lineSpacing()) / marksSpacing;
    const qreal marksSpacingCorrected
        = static_cast<qreal>(height() - painter.fontMetrics().lineSpacing()) / marksCount;
    qreal top = 0.0;
    for (int markIndex = 0; markIndex <= marksCount; ++markIndex) {
        const QRectF markTextRect(handleTextLeft, top, handleTextWidth,
                                  painter.fontMetrics().lineSpacing());
        const auto duartionAtMark = d->maximum * (static_cast<qreal>(markIndex) / marksCount);
        if (d->scrollable
            && (markTextRect.intersects(handleTopTextRect)
                || markTextRect.intersects(handleBottomTextRect))) {
            painter.setOpacity(opacity() * Ui::DesignSystem::focusBackgroundOpacity());
        }
        painter.setPen(QPen(scrollbarColor, Ui::DesignSystem::layout().px2()));
        painter.drawLine(scrollbarRect.right(), markTextRect.center().y(), tickWidth,
                         markTextRect.center().y());
        painter.setPen(markColor);
        painter.drawText(
            markTextRect, Qt::AlignLeft | Qt::AlignVCenter,
            TimeHelper::toString(std::chrono::duration_cast<std::chrono::seconds>(duartionAtMark)));
        if ((markTextRect.intersects(handleTopTextRect)
             || markTextRect.intersects(handleBottomTextRect))) {
            painter.setOpacity(opacity());
        }

        top += marksSpacingCorrected;
    }

    //
    // Рисуем хэндл
    //
    if (d->scrollable) {
        painter.setPen(Ui::DesignSystem::color().onTextEditor());
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
        painter.drawText(
            handleTopTextRect, Qt::AlignLeft | Qt::AlignVCenter,
            TimeHelper::toString(std::chrono::duration_cast<std::chrono::seconds>(handleTopValue)));
        painter.drawText(handleBottomTextRect, Qt::AlignLeft | Qt::AlignVCenter,
                         TimeHelper::toString(std::chrono::duration_cast<std::chrono::seconds>(
                             handleTopValue + d->displayRange)));
    }
}

void AudioplayTextTimeline::mousePressEvent(QMouseEvent* _event)
{
    if (!d->scrollable) {
        return;
    }

    if (_event->buttons().testFlag(Qt::NoButton)) {
        return;
    }

    updateValue(_event->pos());
}

void AudioplayTextTimeline::mouseMoveEvent(QMouseEvent* _event)
{
    if (!d->scrollable) {
        return;
    }

    if (_event->buttons().testFlag(Qt::NoButton)) {
        return;
    }

    updateValue(_event->pos());
}

void AudioplayTextTimeline::mouseReleaseEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);

    update();
}

void AudioplayTextTimeline::wheelEvent(QWheelEvent* _event)
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

void AudioplayTextTimeline::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    resize(sizeHint().width(), height());
}

void AudioplayTextTimeline::updateValue(const QPoint& _mousePosition)
{
    const qreal trackHeight = contentsRect().height();
    const qreal mousePosition = _mousePosition.y() - contentsMargins().left();
    const auto value = d->maximum * mousePosition / trackHeight;
    setValue(qBound(d->minimum, std::chrono::duration_cast<std::chrono::milliseconds>(value),
                    d->maximum));
}

} // namespace Ui
