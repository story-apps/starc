#include "splitter.h"

#include <QMouseEvent>
#include <QResizeEvent>


class Splitter::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить список размеров виджетов в зависимости от ориентации разделителя
     */
    QVector<int> widgetsSizes() const;

    /**
     * @brief Изменить размер, сохраняя заданные пропорции
     */
    void resize(Splitter* _splitter, const QVector<qreal>& _sizes);

    Qt::Orientation orientation = Qt::Horizontal;
    Widget* handle = nullptr;
    QVector<qreal> sizes;
    QVector<QWidget*> widgets;
};

Splitter::Implementation::Implementation(QWidget* _parent)
    : handle(new Widget(_parent))
{
    handle->setBackgroundColor(Qt::transparent);
    handle->setCursor(Qt::SplitHCursor);
}

QVector<int> Splitter::Implementation::widgetsSizes() const
{
    QVector<int> sizes;
    for (auto widget : widgets) {
        sizes.append(orientation == Qt::Horizontal
                     ? widget->width()
                     : widget->height());
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


// ****


Splitter::Splitter(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    d->handle->installEventFilter(this);
}

Splitter::~Splitter() = default;

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
    auto addWidget = [this] (QWidget* _widget){
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
    setSizes({ _first->isVisible() ? 1 : 0,
               _second->isVisible() ? 1 : 0 });

    //
    // Поднимем хендл, чтобы не терять управление
    //
    d->handle->raise();
}

void Splitter::setSizes(const QVector<int>& _sizes)
{
     Q_ASSERT(d->widgets.size() == _sizes.size());

    //
    // Автоматом распределяем пространство между вложенными виджетами
    //
    d->sizes.clear();
    const auto sizesMax = std::accumulate(_sizes.begin(), _sizes.end(), 0);
    for (auto size : _sizes) {
        d->sizes.append(static_cast<qreal>(size) / sizesMax);
    }
    const auto sizeDelta = static_cast<qreal>(width()) / sizesMax;
    auto widgets = d->widgets;
    auto widgetsSizes = _sizes;
    for (auto& widgetSize : widgetsSizes) {
        widgetSize *= sizeDelta;
    }
    if (isRightToLeft()) {
        std::reverse(widgets.begin(), widgets.end());
        std::reverse(widgetsSizes.begin(), widgetsSizes.end());
    }
    QRect widgetGeometry = geometry();
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
                widgetGeometry.setWidth(widgetSize);
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
        //
        if (d->orientation == Qt::Horizontal) {
            widgetGeometry.moveLeft(widgetGeometry.right());
        } else {
            widgetGeometry.moveTop(widgetGeometry.bottom());
        }
    }

    //
    // Позиционируем хэндл
    //
    const QRect handleGeometry(widgets.constFirst()->geometry().right() - 2, 0,
                               5, height());
    d->handle->setGeometry(handleGeometry);
}

QByteArray Splitter::saveState() const
{
    QByteArray state;
    QDataStream stream(&state, QIODevice::WriteOnly);
    for (auto size : d->sizes) {
        stream << size;
    }
    return state;
}
#include <QDebug>
void Splitter::restoreState(const QByteArray& _state)
{
    qDebug() << Q_FUNC_INFO;
    auto state = _state;
    QDataStream stream(&state, QIODevice::ReadOnly);
    qreal size = 0;
    QVector<qreal> sizes;
    while (!stream.atEnd()) {
        stream >> size;
        sizes.append(size);
    }

    //
    // Избегаем кривых данных
    //
    if (sizes.size() != 2) {
        qDebug() << sizes;
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
    if (_event->type() == QEvent::LayoutDirectionChange) {
        setSizes(d->widgetsSizes());
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
                const auto firstWidgetSize = std::min(std::max(0, mapFromGlobal(mouseEvent->globalPos()).x()),
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
            auto widget = qobject_cast<QWidget*>(_watched);
            const auto widgetIndex = d->widgets.indexOf(widget);
            if (widgetIndex == -1) {
                break;
            }

            if (d->sizes.at(widgetIndex) > 0) {
                break;
            }

            int widgetSize = widget->property(lastSizeKey).toInt();
            if (widgetSize <= 0) {
                const auto widgetSizeHint = widget->sizeHint();
                widgetSize = d->orientation == Qt::Horizontal ? widgetSizeHint.width()
                                                              : widgetSizeHint.height();
            }
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
            auto widget = qobject_cast<QWidget*>(_watched);
            const auto widgetIndex = d->widgets.indexOf(widget);
            if (widgetIndex == -1) {
                break;
            }

            if (d->sizes.at(widgetIndex) == 0) {
                break;
            }

            widget->setProperty(lastSizeKey, d->orientation == Qt::Horizontal ? widget->width()
                                                                              : widget->height());
            if (widget == d->widgets.constFirst()) {
                setSizes({ 0, 1 });
            } else {
                setSizes({ 1, 0 });
            }
            break;
        }

        default: break;
    }

    return Widget::eventFilter(_watched, _event);
}
