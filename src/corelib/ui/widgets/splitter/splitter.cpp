#include "splitter.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>

#include <QAction>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QVariantAnimation>


class Splitter::Implementation
{
public:
    explicit Implementation(Splitter* _parent);

    /**
     * @brief Получить список размеров виджетов в зависимости от ориентации разделителя
     */
    QVector<int> widgetsSizes() const;

    /**
     * @brief Изменить размер, сохраняя заданные пропорции
     */
    void resize(Splitter* _splitter, const QVector<qreal>& _sizes);

    /**
     * @brief Анимировать кнопку отображения тулбара
     */
    void animateShowHiddenPanelToolbar(const QPointF& _finalPosition);


    Splitter* q = nullptr;

    Qt::Orientation orientation = Qt::Horizontal;
    Widget* handle = nullptr;
    QVector<qreal> sizes;
    QVector<int> lastSizes;
    QVector<QWidget*> widgets;

    FloatingToolBar* showHiddenPanelToolbar = nullptr;
    QAction* showLeftPanelAction = nullptr;
    QAction* showRightPanelAction = nullptr;
    QVariantAnimation showHiddenPanelToolbarPositionAnimation;
    //
    bool isHideButtonAvailable = false;
    bool isHideLeftPanel = true;
    FloatingToolBar* hideVisiblePanelToolbar = nullptr;
    QAction* hideLeftPanelAction = nullptr;
    QAction* hideRightPanelAction = nullptr;
    QVariantAnimation hideVisiblePanelToolbarOpacityAnimation;
    QTimer hideVisiblePanelToolbarHidingTimer;
};

Splitter::Implementation::Implementation(Splitter* _q)
    : q(_q)
    , handle(new Widget(_q))
    , showHiddenPanelToolbar(new FloatingToolBar(_q))
    , showLeftPanelAction(new QAction(showHiddenPanelToolbar))
    , showRightPanelAction(new QAction(showHiddenPanelToolbar))
    , hideVisiblePanelToolbar(new FloatingToolBar(_q))
    , hideLeftPanelAction(new QAction(showHiddenPanelToolbar))
    , hideRightPanelAction(new QAction(showHiddenPanelToolbar))
{
    handle->setBackgroundColor(Qt::transparent);
    handle->setCursor(Qt::SplitHCursor);

    showHiddenPanelToolbar->addAction(showRightPanelAction);
    showHiddenPanelToolbar->addAction(showLeftPanelAction);
    showHiddenPanelToolbar->hide();
    showLeftPanelAction->setIconText(u8"\U000F0142");
    showRightPanelAction->setIconText(u8"\U000F0141");
    showHiddenPanelToolbarPositionAnimation.setEasingCurve(QEasingCurve::OutQuad);
    showHiddenPanelToolbarPositionAnimation.setDuration(240);
    //
    hideVisiblePanelToolbar->addAction(hideRightPanelAction);
    hideVisiblePanelToolbar->addAction(hideLeftPanelAction);
    hideVisiblePanelToolbar->hide();
    hideLeftPanelAction->setIconText(u8"\U000F0141");
    hideRightPanelAction->setIconText(u8"\U000F0142");
    hideVisiblePanelToolbarOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    hideVisiblePanelToolbarOpacityAnimation.setDuration(240);
    hideVisiblePanelToolbarOpacityAnimation.setStartValue(0.0);
    hideVisiblePanelToolbarOpacityAnimation.setEndValue(0.6);
    hideVisiblePanelToolbarHidingTimer.setSingleShot(true);
    hideVisiblePanelToolbarHidingTimer.setInterval(3000);
}

QVector<int> Splitter::Implementation::widgetsSizes() const
{
    QVector<int> sizes;
    for (auto widget : widgets) {
        sizes.append(orientation == Qt::Horizontal ? widget->width() : widget->height());
    }
    return sizes;
}

void Splitter::Implementation::resize(Splitter* _splitter, const QVector<qreal>& _sizes)
{
    //
    // ормируем список новых размеров
    //
    QVector<int> newSizes;
    for (auto& size : _sizes) {
        newSizes.append(size * 1000);
    }
    //
    // Применяем их
    //
    _splitter->setSizes(newSizes);
    //
    // Восстанавливаем значение пропорций
    //
    sizes = _sizes;
}

void Splitter::Implementation::animateShowHiddenPanelToolbar(const QPointF& _finalPosition)
{
    //
    // Показать кнопку
    //
    if (!_finalPosition.isNull()) {
        if (showHiddenPanelToolbarPositionAnimation.direction() == QVariantAnimation::Forward
            && showHiddenPanelToolbarPositionAnimation.endValue() == _finalPosition) {
            return;
        }

        if (showHiddenPanelToolbar->isVisibleTo(q)) {
            if (showHiddenPanelToolbarPositionAnimation.state() == QVariantAnimation::Running) {
                showHiddenPanelToolbarPositionAnimation.pause();
                showHiddenPanelToolbarPositionAnimation.setEndValue(_finalPosition);
                showHiddenPanelToolbarPositionAnimation.resume();
            } else {
                showHiddenPanelToolbar->move(_finalPosition.toPoint());
            }
            return;
        }

        showHiddenPanelToolbarPositionAnimation.setDirection(QVariantAnimation::Forward);
        QPointF startPosition(0, _finalPosition.y());
        if (_finalPosition.x() < 0) {
            startPosition.setX(-1 * showHiddenPanelToolbar->width());
        } else {
            startPosition.setX(_finalPosition.x() + (showHiddenPanelToolbar->width() / 2));
        }
        showHiddenPanelToolbarPositionAnimation.setStartValue(startPosition);
        showHiddenPanelToolbarPositionAnimation.setEndValue(_finalPosition);

        showHiddenPanelToolbar->move(
            showHiddenPanelToolbarPositionAnimation.startValue().toPointF().toPoint());
        showHiddenPanelToolbar->show();
    }
    //
    // Скрыть
    //
    else {
        if ((showHiddenPanelToolbarPositionAnimation.direction() == QVariantAnimation::Backward
             && showHiddenPanelToolbarPositionAnimation.state() == QVariantAnimation::Running)
            || !showHiddenPanelToolbar->isVisibleTo(q)) {
            return;
        }

        showHiddenPanelToolbarPositionAnimation.setDirection(QVariantAnimation::Backward);
        const QPointF startPosition(showHiddenPanelToolbar->pos().x() < 0
                                        ? -1 * showHiddenPanelToolbar->width()
                                        : showHiddenPanelToolbar->pos().x()
                                            + (showHiddenPanelToolbar->width() / 2),
                                    showHiddenPanelToolbar->pos().y());
        showHiddenPanelToolbarPositionAnimation.setStartValue(startPosition);
        showHiddenPanelToolbarPositionAnimation.setEndValue(QPointF(showHiddenPanelToolbar->pos()));
    }

    showHiddenPanelToolbarPositionAnimation.start();
}


// ****


Splitter::Splitter(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    d->handle->installEventFilter(this);

    connect(d->showLeftPanelAction, &QAction::triggered, this, [this] {
        if (d->lastSizes.isEmpty()) {
            d->lastSizes = { 2, 7 };
        }
        setSizes(d->lastSizes);
    });
    connect(d->showRightPanelAction, &QAction::triggered, this, [this] {
        if (d->lastSizes.isEmpty()) {
            d->lastSizes = { 7, 2 };
        }
        setSizes(d->lastSizes);
    });
    connect(d->hideLeftPanelAction, &QAction::triggered, this, [this] {
        d->lastSizes = sizes();
        setSizes({ 0, 1 });
    });
    connect(d->hideRightPanelAction, &QAction::triggered, this, [this] {
        d->lastSizes = sizes();
        setSizes({ 1, 0 });
    });
    connect(&d->showHiddenPanelToolbarPositionAnimation, &QVariantAnimation::valueChanged, this,
            [this] {
                d->showHiddenPanelToolbar->move(
                    d->showHiddenPanelToolbarPositionAnimation.currentValue().toPoint());
            });
    connect(&d->showHiddenPanelToolbarPositionAnimation, &QVariantAnimation::finished, this,
            [this] {
                if (d->showHiddenPanelToolbarPositionAnimation.direction()
                    == QVariantAnimation::Backward) {
                    d->showHiddenPanelToolbar->hide();
                }
            });
    connect(&d->hideVisiblePanelToolbarOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this] {
                d->hideVisiblePanelToolbar->setOpacity(
                    d->hideVisiblePanelToolbarOpacityAnimation.currentValue().toReal());
            });
    connect(&d->hideVisiblePanelToolbarOpacityAnimation, &QVariantAnimation::finished, this,
            [this] {
                if (d->hideVisiblePanelToolbarOpacityAnimation.direction()
                    == QVariantAnimation::Backward) {
                    d->hideVisiblePanelToolbar->hide();
                }
            });
    connect(&d->hideVisiblePanelToolbarHidingTimer, &QTimer::timeout, this, [this] {
        d->hideVisiblePanelToolbarOpacityAnimation.setDirection(QVariantAnimation::Backward);
        d->hideVisiblePanelToolbarOpacityAnimation.start();
    });
}

Splitter::~Splitter() = default;

void Splitter::setHidePanelButtonAvailable(bool _available, bool _forLeftPanel)
{
    if (d->isHideButtonAvailable == _available && d->isHideLeftPanel == _forLeftPanel) {
        return;
    }

    d->isHideButtonAvailable = _available;
    d->isHideLeftPanel = _forLeftPanel;
    if (!d->isHideButtonAvailable && d->hideVisiblePanelToolbar->isVisibleTo(this)) {
        d->hideVisiblePanelToolbar->hide();
    }
}

void Splitter::setWidgets(QWidget* _first, QWidget* _second)
{
    if (d->widgets.contains(_first) && d->widgets.contains(_second)) {
        return;
    }

    if (!d->widgets.isEmpty()) {
        qDeleteAll(d->widgets);
        d->widgets.clear();
    }

    //
    // Сохраняем виджеты
    //
    auto addWidget = [this](QWidget* _widget) {
        d->widgets.append(_widget);
        if (_widget->parent() != this) {
            _widget->setParent(this);
        }
        _widget->installEventFilter(this);
    };
    addWidget(_first);
    addWidget(_second);

    //
    // Настроим дефолтный размер
    //
    setSizes({ _first->isVisibleTo(this) ? 1 : 0, _second->isVisibleTo(this) ? 1 : 0 });

    //
    // Поднимем хендл, чтобы не терять управление
    //
    d->handle->raise();
    d->showHiddenPanelToolbar->raise();
}

QVector<int> Splitter::sizes() const
{
    QVector<int> sizes;
    for (auto size : std::as_const(d->sizes)) {
        sizes.append(size * 1000);
    }
    return sizes;
}

void Splitter::setSizes(const QVector<int>& _sizes)
{
    if (topLevelWidget()->isMinimized()) {
        return;
    }

    Q_ASSERT(d->widgets.size() == _sizes.size());

    auto newSizes = _sizes;
    const auto sizesMax = std::accumulate(newSizes.begin(), newSizes.end(), 0);
    const auto sizeDelta = static_cast<qreal>(width()) / sizesMax;
    auto lessThan24px
        = [sizeDelta](int _size) { return sizeDelta * _size < Ui::DesignSystem::layout().px24(); };

    //
    // Если одна из сторон становится меньше 24 пикселей, скроем её автоматически
    //
    if (!newSizes.contains(0)) {
        if (newSizes.constFirst() < newSizes.constLast() && lessThan24px(newSizes.constFirst())) {
            newSizes.last() += newSizes.first();
            newSizes.first() = 0;
        } else if (newSizes.constFirst() > newSizes.constLast()
                   && lessThan24px(newSizes.constLast())) {
            newSizes.first() += newSizes.last();
            newSizes.last() = 0;
        }
    }


    //
    // Автоматом распределяем пространство между вложенными виджетами
    //
    d->sizes.clear();
    for (auto size : newSizes) {
        d->sizes.append(static_cast<qreal>(size) / sizesMax);
    }
    auto sizes = d->sizes;
    auto widgets = d->widgets;
    auto widgetsSizes = newSizes;
    for (auto& widgetSize : widgetsSizes) {
        widgetSize *= sizeDelta;
    }
    if (isRightToLeft()) {
        std::reverse(sizes.begin(), sizes.end());
        std::reverse(widgets.begin(), widgets.end());
        std::reverse(widgetsSizes.begin(), widgetsSizes.end());
    }
    QRect widgetGeometry = rect();
    for (int index = 0; index < d->widgets.size(); ++index) {
        auto widget = widgets[index];
        const auto widgetSize = widgetsSizes[index];

        //
        // Последний виджет занимает всю оставшуюся область,
        // которая может возникнуть из-за остатка от деления
        //
        if (widget == widgets.constLast()) {
            widgetGeometry.setRight(width());
            widgetGeometry.setBottom(height());
        } else {
            if (d->orientation == Qt::Horizontal) {
                //
                // Геометрия, при установке в неё ширину, считает на два пикселя меньше, поэтому
                // тут добавляем дельту, которая будет компенсировать этот недостаток
                //
                const auto widthDelta = widgetSize > 0 ? (isRightToLeft() ? -2 : 1) : 0;
                widgetGeometry.setWidth(widgetSize + widthDelta);
            } else {
                widgetGeometry.setHeight(widgetSize);
            }
        }
        //
        // Позиционируем виджет
        //
        widget->setGeometry(widgetGeometry);
        //
        // И корректируем геометрию следующего
        // +1, чтобы виджеты не накладывались друг на друга
        //
        if (d->orientation == Qt::Horizontal) {
            widgetGeometry.setLeft(widgetGeometry.right() + 1);
        } else {
            widgetGeometry.setTop(widgetGeometry.bottom() + 1);
        }
    }

    //
    // Позиционируем хэндл
    //
    const QRect handleGeometry(widgets.constFirst()->geometry().right() - 4, 0,
                               newSizes.contains(0) ? 0 : 9, height());
    d->handle->setGeometry(handleGeometry);
    //
    // ... и кнопку отображения скрытой панели
    //
    const auto minimumVisibleSize = 0.005;
    if ((sizes.constFirst() <= minimumVisibleSize && widgets.constFirst()->isVisibleTo(this))
        || (sizes.constLast() <= minimumVisibleSize && widgets.constLast()->isVisibleTo(this))) {
        if (sizes.constFirst() <= minimumVisibleSize) {
            d->showHiddenPanelToolbar->setActionCustomWidth(d->showRightPanelAction,
                                                            Ui::DesignSystem::layout().px8());
            d->showHiddenPanelToolbar->clearActionCustomWidth(d->showLeftPanelAction);
        } else {
            d->showHiddenPanelToolbar->setActionCustomWidth(d->showLeftPanelAction,
                                                            Ui::DesignSystem::layout().px8());
            d->showHiddenPanelToolbar->clearActionCustomWidth(d->showRightPanelAction);
        }
        d->showHiddenPanelToolbar->resize(d->showHiddenPanelToolbar->sizeHint());

        d->animateShowHiddenPanelToolbar(
            QPointF(handleGeometry.center().x() - (d->showHiddenPanelToolbar->width() / 2),
                    (height() - d->showHiddenPanelToolbar->height()) / 2));
    } else {
        d->animateShowHiddenPanelToolbar({});
    }
    //
    // ... и кнопку скрытия панели
    //
    if ((sizes.constFirst() <= minimumVisibleSize && widgets.constFirst()->isVisibleTo(this))
        || (sizes.constLast() <= minimumVisibleSize && widgets.constLast()->isVisibleTo(this))) {
        d->hideVisiblePanelToolbar->hide();
    } else {
        if (d->isHideLeftPanel) {
            d->hideVisiblePanelToolbar->setActionCustomWidth(d->hideRightPanelAction,
                                                             Ui::DesignSystem::layout().px8());
            d->hideVisiblePanelToolbar->clearActionCustomWidth(d->hideLeftPanelAction);
            d->widgets.constLast()->raise();
            d->handle->raise();
            d->hideVisiblePanelToolbar->raise();
            d->widgets.constFirst()->raise();
            d->hideVisiblePanelToolbar->resize(d->hideVisiblePanelToolbar->sizeHint());

            d->hideVisiblePanelToolbar->move(
                QPoint(handleGeometry.center().x() - (d->hideVisiblePanelToolbar->width() / 2),
                       (height() - d->hideVisiblePanelToolbar->height()) / 2));
        } else {
            d->hideVisiblePanelToolbar->setActionCustomWidth(d->hideLeftPanelAction,
                                                             Ui::DesignSystem::layout().px8());
            d->hideVisiblePanelToolbar->clearActionCustomWidth(d->hideRightPanelAction);
            d->widgets.constFirst()->raise();
            d->handle->raise();
            d->hideVisiblePanelToolbar->raise();
            d->widgets.constLast()->raise();
            d->hideVisiblePanelToolbar->resize(d->hideVisiblePanelToolbar->sizeHint());

            d->hideVisiblePanelToolbar->move(
                QPoint(handleGeometry.center().x() - (d->hideVisiblePanelToolbar->width() / 2),
                       (height() - d->hideVisiblePanelToolbar->height()) / 2));
        }
    }
    d->showHiddenPanelToolbar->raise();
}

QByteArray Splitter::saveState() const
{
    QByteArray state;
    QDataStream stream(&state, QIODevice::WriteOnly);
    for (auto size : std::as_const(d->sizes)) {
        stream << size;
    }
    return state;
}

void Splitter::restoreState(const QByteArray& _state)
{
    auto state = _state;
    QDataStream stream(&state, QIODevice::ReadOnly);
    qreal size = 0;
    QVector<qreal> sizes;
    while (!stream.atEnd()) {
        stream >> size;
        sizes.append(std::min(1.0, std::max(0.0, size)));
    }

    //
    // Избегаем кривых данных
    //
    if (sizes.size() != 2) {
        return;
    }

    //
    // Восстановим состояние
    //
    d->resize(this, sizes);
}

bool Splitter::event(QEvent* _event)
{
    //
    // Если произошла смена направления компоновки, переустановим текущие размеры,
    // чтобы перекомпоновать содержимое
    //
    switch (_event->type()) {
    case QEvent::LayoutDirectionChange: {
        setSizes(d->widgetsSizes());
        break;
    }

    default:
        break;
    }

    return Widget::event(_event);
}

void Splitter::resizeEvent(QResizeEvent* _event)
{
    Q_UNUSED(_event)

    d->resize(this, d->sizes);
}

bool Splitter::eventFilter(QObject* _watched, QEvent* _event)
{
    //
    // Если сам виджет скрыт, то не делаем никакую обработку
    //
    if (!isVisible()) {
        return false;
    }

    const char* lastSizeKey = "last-size";

    switch (_event->type()) {
    //
    // При смещении разделителя, корректируем размеры вложенных виджетов
    //
    case QEvent::MouseMove: {
        if (_watched == d->handle) {
            auto mouseEvent = static_cast<QMouseEvent*>(_event);
            const auto maxSize = d->orientation == Qt::Horizontal ? width() : height();
            const auto firstWidgetSize = std::min(
                std::max(0, mapFromGlobal(mouseEvent->globalPos()).x()),
                maxSize + 1); // Без единицы от второго виджета торчит однопиксельный край
            if (isLeftToRight()) {
                setSizes({ firstWidgetSize, maxSize - firstWidgetSize });
            } else {
                setSizes({ maxSize - firstWidgetSize, firstWidgetSize });
            }
        }
        break;
    }

    //
    // При отображении одного из вложенных виджетов, пробуем восстановить его размер
    //
    case QEvent::Show: {
        d->handle->setCursor(Qt::SplitHCursor);
        d->handle->installEventFilter(this);

        auto widget = qobject_cast<QWidget*>(_watched);
        const auto widgetIndex = d->widgets.indexOf(widget);
        if (widgetIndex == -1) {
            break;
        }

        if (d->sizes.at(widgetIndex) > 0) {
            break;
        }

        const auto widgetSize = widget->property(lastSizeKey).toInt();
        const auto maxSize = d->orientation == Qt::Horizontal ? width() : height();
        if (widget == d->widgets.constFirst()) {
            setSizes({ widgetSize, maxSize - widgetSize });
        } else {
            setSizes({ maxSize - widgetSize, widgetSize });
        }
        break;
    }

    //
    // При скрытии одного из вложенных виджетов, используем занимаемую им область под видимый
    //
    case QEvent::Hide: {
        d->handle->setCursor(Qt::ArrowCursor);
        d->handle->removeEventFilter(this);

        auto widget = qobject_cast<QWidget*>(_watched);
        const auto widgetIndex = d->widgets.indexOf(widget);
        if (widgetIndex == -1) {
            break;
        }

        if (d->sizes.at(widgetIndex) == 0) {
            d->animateShowHiddenPanelToolbar({});
            break;
        }

        widget->setProperty(lastSizeKey,
                            d->orientation == Qt::Horizontal ? widget->width() : widget->height());
        if (widget == d->widgets.constFirst()) {
            setSizes({ 0, 1 });
        } else {
            setSizes({ 1, 0 });
        }
        d->hideVisiblePanelToolbar->hide();
        break;
    }

    case QEvent::Enter: {
        if (!d->isHideButtonAvailable) {
            break;
        }

        auto widget = qobject_cast<QWidget*>(_watched);
        const auto widgetIndex = d->widgets.indexOf(widget);
        if (widgetIndex == -1) {
            break;
        }

        if (widgetIndex == 0) {
            if (d->isHideLeftPanel) {
                d->hideVisiblePanelToolbarHidingTimer.stop();
                if (!d->hideVisiblePanelToolbar->isVisibleTo(this)) {
                    d->hideVisiblePanelToolbar->setOpacity(0.0);
                    d->hideVisiblePanelToolbar->setVisible(true);
                    d->hideVisiblePanelToolbarOpacityAnimation.setDirection(
                        QVariantAnimation::Forward);
                    d->hideVisiblePanelToolbarOpacityAnimation.start();
                }
            } else if (d->hideVisiblePanelToolbar->isVisibleTo(this)) {
                d->hideVisiblePanelToolbarHidingTimer.start();
            }
        } else if (widgetIndex == 1) {
            if (!d->isHideLeftPanel) {
                d->hideVisiblePanelToolbarHidingTimer.stop();
                if (!d->hideVisiblePanelToolbar->isVisibleTo(this)) {
                    d->hideVisiblePanelToolbar->setOpacity(0.0);
                    d->hideVisiblePanelToolbar->setVisible(true);
                    d->hideVisiblePanelToolbarOpacityAnimation.setDirection(
                        QVariantAnimation::Forward);
                    d->hideVisiblePanelToolbarOpacityAnimation.start();
                }
            } else if (d->hideVisiblePanelToolbar->isVisibleTo(this)) {
                d->hideVisiblePanelToolbarHidingTimer.start();
            }
        }
        break;
    }

    case QEvent::Leave: {
        if (!d->isHideButtonAvailable) {
            break;
        }

        auto widget = qobject_cast<QWidget*>(_watched);
        const auto widgetIndex = d->widgets.indexOf(widget);
        if (widgetIndex == -1) {
            break;
        }

        if (widgetIndex == 0) {
            d->hideVisiblePanelToolbarHidingTimer.start();
        }
        break;
    }

    default:
        break;
    }

    return Widget::eventFilter(_watched, _event);
}

void Splitter::updateTranslations()
{
    d->showLeftPanelAction->setToolTip(tr("Show hidden panel"));
    d->showRightPanelAction->setToolTip(tr("Show hidden panel"));
    d->hideLeftPanelAction->setToolTip(tr("Hide left panel"));
    d->hideRightPanelAction->setToolTip(tr("Hide right panel"));
}

void Splitter::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    d->showHiddenPanelToolbar->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->showHiddenPanelToolbar->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->hideVisiblePanelToolbar->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->hideVisiblePanelToolbar->setTextColor(Ui::DesignSystem::color().onPrimary());

    d->resize(this, d->sizes);
}
