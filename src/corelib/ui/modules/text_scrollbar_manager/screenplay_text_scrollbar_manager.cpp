#include "screenplay_text_scrollbar_manager.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_folder_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/eighths_helper.h>
#include <utils/helpers/time_helper.h>
#include <utils/tools/debouncer.h>

#include <QAbstractScrollArea>
#include <QApplication>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>
#include <QTimer>
#include <QVariantAnimation>
#include <QtMath>


namespace Ui {

class ScreenplayTextScrollBarManager::Implementation
{
public:
    explicit Implementation(ScreenplayTextScrollBarManager* _q, QAbstractScrollArea* _parent);

    /**
     * @brief Настроить соединения таймлайна
     */
    void connectLine(ScreenplayTextTimeline* _line);
    void connectLine(ScreenplayTextPageline* _line);

    /**
     * @brief Синхронизировать таймлайн с параметрами модели
     */
    void updateLineModel();

    /**
     * @brief Обновить значение на таймлайне если он находится в режиме скрола
     */
    void updateLineValue();

    /**
     * @brief Обновить состояние таймлайна
     */
    void updateLineState();

    /**
     * @brief Скорректировать положение таймлайна в зависимости от родительского виджета
     */
    void updateLineGeometry();

    /**
     * @brief Обновить размер отображаемой области таймлайна
     */
    void updateLineDisplayRange();

    /**
     * @brief Запустить анимацию отображения таймлайна
     */
    void showLineAnimated();


    ScreenplayTextScrollBarManager* q = nullptr;

    QPointer<BusinessLayer::ScreenplayTextModel> model;

    QScrollBar* verticalScrollBar = nullptr;
    QScrollBar* horizontalScrollBar = nullptr;

    ScrollBarType type = ScrollBarType::Pageline;
    QPointer<ScreenplayTextTimeline> timeline;
    QPointer<ScreenplayTextPageline> pageline;
    bool needAnimateLineOpacity = false;
    QTimer lineHideTimer;
    QVariantAnimation lineOpacityAnimation;
    Debouncer lineUpdateDebouncer;

    ScrollBarMode mode = ScrollBarMode::Scroll;
    bool syncingInitialized = false;
    std::chrono::milliseconds rangeStart{ 0 };
    std::chrono::milliseconds rangeLength{ 0 };
    qreal pageRangeStart = 0;
    qreal pageRangeLength = 0;
};

ScreenplayTextScrollBarManager::Implementation::Implementation(ScreenplayTextScrollBarManager* _q,
                                                               QAbstractScrollArea* _parent)
    : q(_q)
    , verticalScrollBar(_parent->verticalScrollBar())
    , horizontalScrollBar(_parent->horizontalScrollBar())
    , lineUpdateDebouncer(180)
{
    lineHideTimer.setSingleShot(true);
    lineHideTimer.setInterval(2000);
    lineOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    lineOpacityAnimation.setDuration(240);
    lineOpacityAnimation.setStartValue(1.0);
    lineOpacityAnimation.setEndValue(0.0);
}

void ScreenplayTextScrollBarManager::Implementation::connectLine(ScreenplayTextTimeline* _line)
{
    connect(
        _line, &ScreenplayTextTimeline::valueChanged, q, [this](std::chrono::milliseconds _value) {
            if (!syncingInitialized || mode != ScrollBarMode::Scroll || !model
                || model->duration().count() <= 0) {
                return;
            }
            verticalScrollBar->setValue(verticalScrollBar->maximum() * _value / model->duration());
        });
    connect(_line, &ScreenplayTextTimeline::updateValueRequested, q, [this] { updateLineState(); });

    connect(timeline, &ScreenplayTextTimeline::positionClicked, q,
            [this](std::chrono::milliseconds _position) {
                const auto index = model->indexForDuration(_position);
                if (index.isValid()) {
                    emit q->itemClicked(index);
                }
            });
}

void ScreenplayTextScrollBarManager::Implementation::connectLine(ScreenplayTextPageline* _line)
{
    connect(_line, &ScreenplayTextPageline::valueChanged, q, [this](qreal _value) {
        if (!syncingInitialized || mode != ScrollBarMode::Scroll || !model
            || model->eighths() <= 0) {
            return;
        }
        verticalScrollBar->setValue(
            static_cast<int>(verticalScrollBar->maximum() * _value / model->eighths()));
    });
    connect(_line, &ScreenplayTextPageline::updateValueRequested, q, [this] { updateLineState(); });

    connect(pageline, &ScreenplayTextPageline::positionClicked, q, [this](qreal _position) {
        const auto index = model->indexForEighths(_position);
        if (index.isValid()) {
            emit q->itemClicked(index);
        }
    });
}

void ScreenplayTextScrollBarManager::Implementation::updateLineModel()
{
    if (!timeline.isNull()) {
        timeline->setMaximum(model ? model->duration() : std::chrono::milliseconds{ 0 });
        timeline->setColors(model ? model->itemsColors()
                                  : std::map<std::chrono::milliseconds, QColor>{});
        timeline->setBookmarks(model ? model->itemsBookmarks()
                                     : std::map<std::chrono::milliseconds, QColor>{});
    }
    if (!pageline.isNull()) {
        pageline->setMaximum(model ? model->eighths() : 0);
        pageline->setColors(model ? model->itemsColorsForEighths() : std::map<qreal, QColor>{});
        pageline->setBookmarks(model ? model->itemsBookmarksForEighths()
                                     : std::map<qreal, QColor>{});
    }

    updateLineState();
}

void ScreenplayTextScrollBarManager::Implementation::updateLineValue()
{
    if (mode != ScrollBarMode::Scroll) {
        return;
    }

    QSignalBlocker timelineBlocker(timeline);
    QSignalBlocker pagelineBlocker(pageline);
    const qreal fraction = verticalScrollBar->maximum() > 0
        ? qreal(verticalScrollBar->value()) / verticalScrollBar->maximum()
        : 0;
    if (!timeline.isNull()) {
        timeline->setValue(model ? std::chrono::duration_cast<std::chrono::milliseconds>(
                                       model->duration() * fraction)
                                 : std::chrono::milliseconds{ 0 });
    }
    if (!pageline.isNull()) {
        pageline->setValue(model ? model->eighths() * fraction : 0);
    }
}

void ScreenplayTextScrollBarManager::Implementation::updateLineState()
{
    const bool scrollable = mode == ScrollBarMode::Scroll
        && verticalScrollBar->minimum() < verticalScrollBar->maximum();
    if (!timeline.isNull()) {
        timeline->setScrollable(scrollable);
    }
    if (!pageline.isNull()) {
        pageline->setScrollable(scrollable);
    }
    if (mode == ScrollBarMode::Range) {
        if (!timeline.isNull()) {
            timeline->setRange(rangeStart, rangeLength);
        }
        if (!pageline.isNull()) {
            pageline->setRange(pageRangeStart, pageRangeLength);
        }
    } else {
        updateLineDisplayRange();
        updateLineValue();
    }
}

void ScreenplayTextScrollBarManager::Implementation::updateLineGeometry()
{
    Widget* line = nullptr;
    if (type == ScrollBarType::Timeline) {
        if (timeline.isNull()) {
            timeline
                = new ScreenplayTextTimeline(verticalScrollBar->parentWidget()->parentWidget());
            timeline->installEventFilter(q);
            connectLine(timeline);
        }
        line = timeline;
        if (!pageline.isNull()) {
            pageline->hide();
            pageline->deleteLater();
        }
    } else {
        if (pageline.isNull()) {
            pageline
                = new ScreenplayTextPageline(verticalScrollBar->parentWidget()->parentWidget());
            pageline->installEventFilter(q);
            connectLine(pageline);
        }
        line = pageline;
        if (!timeline.isNull()) {
            timeline->hide();
            timeline->deleteLater();
        }
    }

    const auto bottomScrollBarDelta
        = horizontalScrollBar->maximum() > horizontalScrollBar->minimum()
        ? DesignSystem::scrollBar().minimumSize()
        : 0;
    line->move(0, line->parentWidget()->size().height() - line->height() - bottomScrollBarDelta);
    line->resize(line->parentWidget()->size().width(),
                 line->sizeHint().height() + bottomScrollBarDelta);

    updateLineModel();
}

void ScreenplayTextScrollBarManager::Implementation::updateLineDisplayRange()
{
    if (mode != ScrollBarMode::Scroll || model.isNull()) {
        return;
    }

    //
    // В pageStep текстовый документ задаёт высоту видимой области текста,
    // а в maximum всю высоту документа за вычетом одной страницы,
    // поэтому тут рассчёт идёт исходя из этого знания
    //
    const qreal total = verticalScrollBar->maximum() + qreal(verticalScrollBar->pageStep());
    const qreal fraction = total > 0 ? verticalScrollBar->pageStep() / total : 1;
    if (!timeline.isNull()) {
        timeline->setDisplayRange(
            std::chrono::duration_cast<std::chrono::milliseconds>(model->duration() * fraction));
    }
    if (!pageline.isNull()) {
        pageline->setDisplayRange(model->eighths() * fraction);
    }
}

void ScreenplayTextScrollBarManager::Implementation::showLineAnimated()
{
    if (lineOpacityAnimation.direction() == QVariantAnimation::Backward) {
        return;
    }

    if (lineOpacityAnimation.state() == QVariantAnimation::Running) {
        lineOpacityAnimation.pause();
    }
    lineOpacityAnimation.setDirection(QVariantAnimation::Backward);
    if (lineOpacityAnimation.state() == QVariantAnimation::Paused) {
        lineOpacityAnimation.resume();
    } else {
        lineOpacityAnimation.start();
    }
}


// **


ScreenplayTextScrollBarManager::ScreenplayTextScrollBarManager(QAbstractScrollArea* _parent)
    : QObject(_parent)
    , d(new Implementation(this, _parent))
{
    Q_ASSERT(_parent);

    _parent->installEventFilter(this);

    d->updateLineGeometry();

    initScrollBarsSyncing();

    connect(&d->lineHideTimer, &QTimer::timeout, this, [this] {
        d->lineOpacityAnimation.setDirection(QVariantAnimation::Forward);
        d->lineOpacityAnimation.start();
    });
    connect(&d->lineOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                if (!d->timeline.isNull()) {
                    d->timeline->setOpacity(_value.toReal());
                }
                if (!d->pageline.isNull()) {
                    d->pageline->setOpacity(_value.toReal());
                }
            });
    connect(&d->lineUpdateDebouncer, &Debouncer::gotWork, this, [this] { d->updateLineModel(); });
}

ScreenplayTextScrollBarManager::~ScreenplayTextScrollBarManager() = default;

void ScreenplayTextScrollBarManager::initScrollBarsSyncing()
{
    if (!d->syncingInitialized) {
        d->syncingInitialized = true;
        connect(d->verticalScrollBar, &QScrollBar::rangeChanged, this,
                [this] { d->updateLineState(); });
        connect(d->verticalScrollBar, &QScrollBar::valueChanged, this,
                [this] { d->updateLineValue(); });

        //
        // Тут мы закладываемся на знание о том, что полоса горизонтальной прокрутки будет видна,
        // если у неё валидный интервал значений, поэтому обновляем геометрию в момент изменения
        // интревала
        //
        connect(d->horizontalScrollBar, &QScrollBar::rangeChanged, this,
                [this] { d->updateLineGeometry(); });
    }

    d->updateLineState();
}

void ScreenplayTextScrollBarManager::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    if (d->model == _model) {
        return;
    }

    if (d->model) {
        d->model->disconnect(&d->lineUpdateDebouncer);
        if (d->model->informationModel()) {
            d->model->informationModel()->disconnect(&d->lineUpdateDebouncer);
        }
    }

    d->model = _model;

    if (d->model) {
        connect(d->model, &QAbstractItemModel::rowsInserted, &d->lineUpdateDebouncer,
                &Debouncer::orderWork);
        connect(d->model, &QAbstractItemModel::rowsRemoved, &d->lineUpdateDebouncer,
                &Debouncer::orderWork);
        connect(d->model, &QAbstractItemModel::dataChanged, &d->lineUpdateDebouncer,
                &Debouncer::orderWork);
        d->lineUpdateDebouncer.orderWork();
    }

    d->updateLineModel();
}

void ScreenplayTextScrollBarManager::setScrollBarType(ScrollBarType _type)
{
    if (d->type == _type) {
        return;
    }

    d->type = _type;
    d->updateLineGeometry();
    initScrollBarsSyncing();
}

void ScreenplayTextScrollBarManager::setScrollBarVisible(bool _visible, bool _animate)
{
    if (!_animate) {
        if (!d->timeline.isNull()) {
            d->timeline->setVisible(_visible);
        }
        if (!d->pageline.isNull()) {
            d->pageline->setVisible(_visible);
        }
        return;
    }

    if (_visible) {
        d->needAnimateLineOpacity = false;
        d->showLineAnimated();
        d->lineHideTimer.stop();
    } else {
        d->needAnimateLineOpacity = true;
        d->lineHideTimer.start();
    }
}

void ScreenplayTextScrollBarManager::setScrollBarMode(ScrollBarMode _mode)
{
    if (d->mode == _mode) {
        return;
    }

    d->mode = _mode;
    d->updateLineState();
}

void ScreenplayTextScrollBarManager::selectItem(BusinessLayer::TextModelItem* _item)
{
    if (_item == nullptr) {
        d->rangeStart = {};
        d->rangeLength = {};
        d->pageRangeStart = {};
        d->pageRangeLength = {};
    } else if (_item->type() == BusinessLayer::TextModelItemType::Folder) {
        const auto folderItem = static_cast<BusinessLayer::ScreenplayTextModelFolderItem*>(_item);
        if (d->type == ScrollBarType::Timeline) {
            d->rangeStart = d->model->itemStartsDuration(folderItem);
            d->rangeLength = folderItem->duration();
        } else {
            d->pageRangeStart = d->model->itemStartsEighths(folderItem);
            d->pageRangeLength = folderItem->eighths();
        }
    } else if (_item->type() == BusinessLayer::TextModelItemType::Group) {
        const auto sceneItem = static_cast<BusinessLayer::ScreenplayTextModelSceneItem*>(_item);
        if (d->type == ScrollBarType::Timeline) {
            d->rangeStart = d->model->itemStartsDuration(sceneItem);
            d->rangeLength = sceneItem->duration();
        } else {
            d->pageRangeStart = d->model->itemStartsEighths(sceneItem);
            d->pageRangeLength = sceneItem->eighths();
        }
    }

    d->updateLineState();
}

int ScreenplayTextScrollBarManager::scrollBarHeight() const
{
    if (d->type == ScrollBarType::Timeline) {
        return d->timeline->height();
    } else {
        return d->pageline->height();
    }
}


bool ScreenplayTextScrollBarManager::eventFilter(QObject* _watched, QEvent* _event)
{
    //
    // При отображении и при изменении размера родителя, корректируем позицию таймлайна
    //
    if (((!d->timeline.isNull()
          && (_watched == d->timeline || _watched == d->timeline->parentWidget()))
         || (!d->pageline.isNull()
             && (_watched == d->pageline || _watched == d->pageline->parentWidget())))
        && (_event->type() == QEvent::Resize || _event->type() == QEvent::Show)) {
        d->updateLineGeometry();
    }
    //
    // Если курсор мыши вошёл в таймлайн, то останавливаем таймер скрытия, а если вышел - запускаем
    //
    else if (d->needAnimateLineOpacity && _watched == d->timeline) {
        if (_event->type() == QEvent::Enter) {
            d->lineHideTimer.stop();
            d->showLineAnimated();
        } else if (_event->type() == QEvent::Leave) {
            d->lineHideTimer.start();
        }
    }

    return QObject::eventFilter(_watched, _event);
}


// ****


namespace {
QRectF lineTrack(const QWidget* _widget)
{
    const qreal width
        = qMax(qreal{ 0 }, _widget->width() - DesignSystem::scrollBar().minimumSize());
    const qreal inset = qMin(width / 2, DesignSystem::layout().px(42) / 2);
    return QRectF(inset, 0, qMax(qreal{ 0 }, width - 2 * inset), _widget->height());
}
} // namespace


class ScreenplayTextTimeline::Implementation
{
public:
    bool scrollable = true;
    bool explicitRange = false;
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

    d->displayRange = qMin(d->displayRange, d->maximum);
    if (d->explicitRange) {
        d->current = qMin(d->current, d->maximum - d->displayRange);
    }

    update();
}

void ScreenplayTextTimeline::setValue(std::chrono::milliseconds _value)
{
    if (d->minimum > _value || _value > d->maximum) {
        return;
    }

    const bool wasInExplicitRange = d->explicitRange;
    d->explicitRange = false;
    if (d->current == _value && !wasInExplicitRange) {
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
    if (d->explicitRange) {
        d->current = qMin(d->current, d->maximum - d->displayRange);
    }
    update();
}

void ScreenplayTextTimeline::setRange(std::chrono::milliseconds _start,
                                      std::chrono::milliseconds _length)
{
    d->explicitRange = true;
    d->displayRange = qBound(std::chrono::milliseconds{ 0 }, _length, d->maximum);
    d->current = qBound(std::chrono::milliseconds{ 0 }, _start, d->maximum - d->displayRange);
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
    const auto track = lineTrack(this);
    if (track.width() <= 0) {
        return;
    }
    const auto valueToX = [this, track](auto _value) -> qreal {
        return d->maximum > decltype(d->maximum){ 0 }
            ? track.left() + track.width() * _value / d->maximum
            : track.left();
    };

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
        auto paintColor
            = [this, &painter, scrollbarRect, &startDuration, &endDuration, &color, &valueToX] {
                  if (!color.isValid() || d->maximum.count() == 0) {
                      return;
                  }

                  const QRectF colorRect(QPointF(valueToX(startDuration), scrollbarRect.top()),
                                         QPointF(valueToX(endDuration), scrollbarRect.bottom()));
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
        const QPointF top(valueToX(duration), 0);
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
    const qreal handleX = valueToX(d->current);
    const qreal handleY = scrollbarRect.center().y();
    const qreal handleSize = DesignSystem::layout().px12();
    QRectF handleRect(handleX - handleSize / 2, handleY - handleSize / 2, handleSize, handleSize);
    QRectF handleEllipseRect;
    std::chrono::duration<double> handleLeftValue = std::chrono::milliseconds{ 0 };
    if (d->maximum > std::chrono::milliseconds{ 0 }) {
        handleLeftValue = d->explicitRange
            ? d->current
            : d->current - d->current / std::chrono::duration<double>(d->maximum) * d->displayRange;
        const qreal handleLeft = valueToX(handleLeftValue);
        const qreal handleRight = valueToX(handleLeftValue + d->displayRange);
        const auto handleWidth = handleRight - handleLeft - DesignSystem::layout().px() * 2;
        const auto minimumWidth = painter.fontMetrics().lineSpacing() * 2.0;
        if (d->explicitRange || handleWidth >= minimumWidth) {
            const auto inset = d->explicitRange ? qreal{ 0 } : DesignSystem::layout().px();
            handleRect
                = QRectF(handleLeft + inset, scrollbarRect.top() - DesignSystem::layout().px(),
                         qMax(qreal{ 0 }, handleRight - handleLeft - 2 * inset),
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
    const qreal handleTextWidth = DesignSystem::layout().px(42);
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
    const int marksCount = qMax(1, static_cast<int>(track.width() / marksSpacing));
    const qreal marksSpacingCorrected = track.width() / marksCount;
    qreal left = track.left() - handleTextWidth / 2;
    for (int markIndex = 0; markIndex <= marksCount; ++markIndex) {
        const QRectF markTextRect(left, handleTextTop, handleTextWidth,
                                  painter.fontMetrics().lineSpacing());
        const auto duartionAtMark = d->maximum * (static_cast<qreal>(markIndex) / marksCount);
        if ((d->scrollable
             || (d->explicitRange && d->displayRange > std::chrono::milliseconds{ 0 }))
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
    if (d->scrollable || (d->explicitRange && d->displayRange > std::chrono::milliseconds{ 0 })) {
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
    const auto track = lineTrack(this);
    const bool clicked = _event->button() == Qt::LeftButton && track.width() > 0
        && _event->pos().x() >= track.left() && _event->pos().x() <= track.right()
        && d->maximum.count() > 0;
    const qreal position
        = clicked ? d->maximum.count() * (_event->pos().x() - track.left()) / track.width() : 0;

    if (d->scrollable && _event->buttons() != Qt::NoButton) {
        updateValue(_event->pos());
    }
    if (clicked) {
        emit positionClicked(std::chrono::milliseconds{ qRound64(position) });
    }
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
    const auto track = lineTrack(this);
    const qreal trackWidth = track.width();
    if (trackWidth <= 0) {
        return;
    }
    const qreal mousePosition = _mousePosition.x() - track.left();
    const auto value = d->maximum * mousePosition / trackWidth;
    const auto boundedValue = qBound(
        d->minimum, std::chrono::duration_cast<std::chrono::milliseconds>(value), d->maximum);
    if (d->explicitRange) {
        const auto previous = d->current;
        setRange(boundedValue, d->displayRange);
        if (d->current != previous) {
            emit valueChanged(d->current);
        }
    } else {
        setValue(boundedValue);
    }
}

// ****


class ScreenplayTextPageline::Implementation
{
public:
    bool scrollable = true;
    bool explicitRange = false;
    const qreal minimum = 0;
    qreal maximum = 10;
    qreal current = 0;
    qreal displayRange = 5;
    std::map<qreal, QColor> colors;
    std::map<qreal, QColor> bookmarks;
};


// **


ScreenplayTextPageline::ScreenplayTextPageline(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
}

ScreenplayTextPageline::~ScreenplayTextPageline() = default;

void ScreenplayTextPageline::setScrollable(bool _scrollable)
{
    if (d->scrollable == _scrollable) {
        return;
    }

    d->scrollable = _scrollable;
    update();
}

void ScreenplayTextPageline::setMaximum(qreal _maximum)
{
    if (!qIsFinite(_maximum)) {
        return;
    }
    _maximum = qMax(qreal{ 0 }, _maximum);
    if (qFuzzyCompare(d->maximum, _maximum)) {
        return;
    }

    d->maximum = _maximum;
    emit updateValueRequested();
    if (d->current > d->maximum) {
        d->current = d->maximum;
    }

    d->displayRange = qMin(d->displayRange, d->maximum);
    if (d->explicitRange) {
        d->current = qMin(d->current, d->maximum - d->displayRange);
    }

    update();
}

void ScreenplayTextPageline::setValue(qreal _value)
{
    if (!qIsFinite(_value) || d->minimum > _value || _value > d->maximum) {
        return;
    }

    const bool wasInExplicitRange = d->explicitRange;
    d->explicitRange = false;
    if (qFuzzyCompare(d->current, _value) && !wasInExplicitRange) {
        return;
    }

    d->current = _value;
    emit valueChanged(d->current);
    update();
}

void ScreenplayTextPageline::setDisplayRange(qreal _value)
{
    if (!qIsFinite(_value) || d->minimum > _value || _value > d->maximum
        || qFuzzyCompare(d->displayRange, _value)) {
        return;
    }

    d->displayRange = _value;
    if (d->explicitRange) {
        d->current = qMin(d->current, d->maximum - d->displayRange);
    }
    update();
}

void ScreenplayTextPageline::setRange(qreal _start, qreal _length)
{
    if (!qIsFinite(_start) || !qIsFinite(_length)) {
        return;
    }
    d->explicitRange = true;
    d->displayRange = qBound(qreal{ 0 }, _length, d->maximum);
    d->current = qBound(qreal{ 0 }, _start, d->maximum - d->displayRange);
    update();
}

void ScreenplayTextPageline::setColors(const std::map<qreal, QColor>& _colors)
{
    if (d->colors == _colors) {
        return;
    }

    d->colors = _colors;
    update();
}

void ScreenplayTextPageline::setBookmarks(const std::map<qreal, QColor>& _bookmarks)
{
    if (d->bookmarks == _bookmarks) {
        return;
    }

    d->bookmarks = _bookmarks;
    update();
}

QSize ScreenplayTextPageline::sizeHint() const
{
    const QSize marginsDelta = QSizeF(DesignSystem::scrollBar().margins().left()
                                          + DesignSystem::scrollBar().margins().right(),
                                      DesignSystem::scrollBar().margins().top()
                                          + DesignSystem::scrollBar().margins().bottom())
                                   .toSize();
    return QSize(10, static_cast<int>(DesignSystem::layout().px(56))) + marginsDelta;
}

void ScreenplayTextPageline::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(opacity());
    painter.fillRect(_event->rect(), Qt::transparent);

    painter.setFont(DesignSystem::font().caption());

    const QRectF contentRect = rect().adjusted(0, 0, -DesignSystem::scrollBar().minimumSize(), 0);
    const auto track = lineTrack(this);
    if (track.width() <= 0) {
        return;
    }
    const auto valueToX = [this, track](auto _value) -> qreal {
        return d->maximum > decltype(d->maximum){ 0 }
            ? track.left() + track.width() * _value / d->maximum
            : track.left();
    };

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
        auto paintColor
            = [this, &painter, scrollbarRect, &startDuration, &endDuration, &color, &valueToX] {
                  if (!color.isValid() || qFuzzyCompare(d->maximum, 0.0)) {
                      return;
                  }

                  const QRectF colorRect(QPointF(valueToX(startDuration), scrollbarRect.top()),
                                         QPointF(valueToX(endDuration), scrollbarRect.bottom()));
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
        const QPointF top(valueToX(duration), 0);
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
    const qreal handleX = valueToX(d->current);
    const qreal handleY = scrollbarRect.center().y();
    const qreal handleSize = DesignSystem::layout().px12();
    QRectF handleRect(handleX - handleSize / 2, handleY - handleSize / 2, handleSize, handleSize);
    QRectF handleEllipseRect;
    qreal handleLeftValue = 0.0;
    if (d->maximum > qreal{ 0 }) {
        handleLeftValue = d->explicitRange ? d->current
                                           : d->current - d->current / d->maximum * d->displayRange;
        const qreal handleLeft = valueToX(handleLeftValue);
        const qreal handleRight = valueToX(handleLeftValue + d->displayRange);
        const auto handleWidth = handleRight - handleLeft - DesignSystem::layout().px() * 2;
        const auto minimumWidth = painter.fontMetrics().lineSpacing() * 2.0;
        if (d->explicitRange || handleWidth >= minimumWidth) {
            const auto inset = d->explicitRange ? qreal{ 0 } : DesignSystem::layout().px();
            handleRect
                = QRectF(handleLeft + inset, scrollbarRect.top() - DesignSystem::layout().px(),
                         qMax(qreal{ 0 }, handleRight - handleLeft - 2 * inset),
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
    const qreal handleTextWidth = DesignSystem::layout().px(42);
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

    const int marksCount = qMax(1, qCeil(d->maximum / 8.0));
    const qreal marksDefaultSpacing = track.width() / marksCount;
    const int paintOnIndex = qMax(1, qCeil(handleTextWidth / marksDefaultSpacing));
    qreal left = track.left() - handleTextWidth / 2;
    for (int markIndex = 0; markIndex <= marksCount; ++markIndex) {
        const auto durationAtMark = qMin(d->maximum, markIndex * 8.0);
        left = valueToX(durationAtMark) - handleTextWidth / 2;
        const QRectF markTextRect(left, handleTextTop, handleTextWidth,
                                  painter.fontMetrics().lineSpacing());
        if ((d->scrollable || (d->explicitRange && d->displayRange > 0.0))
            && (markTextRect.intersects(handleLeftTextRect)
                || markTextRect.intersects(handleRightTextRect))) {
            painter.setOpacity(opacity() * DesignSystem::focusBackgroundOpacity());
        }
        painter.setPen(QPen(scrollbarColor, DesignSystem::layout().px2()));
        painter.drawLine(markTextRect.center().x(), scrollbarRect.bottom(),
                         markTextRect.center().x(), scrollbarRect.bottom() + tickHeight);

        if (markIndex % paintOnIndex == 0) {
            painter.setPen(markColor);
            painter.drawText(markTextRect, Qt::AlignCenter,
                             markIndex == 0 ? "0" : EighthsHelper::toString(durationAtMark));
            if (markTextRect.intersects(handleLeftTextRect)
                || markTextRect.intersects(handleRightTextRect)) { }
        }
        painter.setOpacity(opacity());
    }

    //
    // Рисуем хэндл
    //
    if (d->scrollable || (d->explicitRange && d->displayRange > 0.0)) {
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
                         EighthsHelper::toString(handleLeftValue));
        painter.drawText(handleRightTextRect, Qt::AlignCenter,
                         EighthsHelper::toString(handleLeftValue + d->displayRange));
    }
}

void ScreenplayTextPageline::mousePressEvent(QMouseEvent* _event)
{
    const auto track = lineTrack(this);
    const bool clicked = _event->button() == Qt::LeftButton && track.width() > 0
        && _event->pos().x() >= track.left() && _event->pos().x() <= track.right()
        && d->maximum > 0;
    const qreal position
        = clicked ? d->maximum * (_event->pos().x() - track.left()) / track.width() : 0;

    if (d->scrollable && _event->buttons() != Qt::NoButton) {
        updateValue(_event->pos());
    }
    if (clicked) {
        emit positionClicked(position);
    }
}

void ScreenplayTextPageline::mouseMoveEvent(QMouseEvent* _event)
{
    if (!d->scrollable) {
        return;
    }

    if (_event->buttons().testFlag(Qt::NoButton)) {
        return;
    }

    updateValue(_event->pos());
}

void ScreenplayTextPageline::mouseReleaseEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);

    update();
}

void ScreenplayTextPageline::wheelEvent(QWheelEvent* _event)
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

void ScreenplayTextPageline::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    resize(sizeHint().width(), height());
}

void ScreenplayTextPageline::updateValue(const QPoint& _mousePosition)
{
    const auto track = lineTrack(this);
    const qreal trackWidth = track.width();
    if (trackWidth <= 0) {
        return;
    }
    const qreal mousePosition = _mousePosition.x() - track.left();
    const qreal value = d->maximum * mousePosition / trackWidth;
    const auto boundedValue = qBound(d->minimum, value, d->maximum);
    if (d->explicitRange) {
        const auto previous = d->current;
        setRange(boundedValue, d->displayRange);
        if (!qFuzzyCompare(d->current, previous)) {
            emit valueChanged(d->current);
        }
    } else {
        setValue(boundedValue);
    }
}

} // namespace Ui
