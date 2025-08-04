#include "scroll_bar.h"

#include <ui/design_system/design_system.h>
#include <utils/tools/debouncer.h>

#include <QAbstractScrollArea>
#include <QEvent>
#include <QPainter>
#include <QStyleOption>
#include <QVariantAnimation>


class ScrollBar::Implementation
{
public:
    explicit Implementation(ScrollBar* _q, bool _stickToParent);

    void correctScrollbarGeomerty();
    void maximizeScrollbar();
    void minimizeScrollbar();

    ScrollBar* q = nullptr;

    bool stickToParent = false;

    QColor backgroundColor;
    QColor handleColor;

    QVariantAnimation widthAnimation;
    Debouncer widthAnimationDebouncer;
    QAbstractAnimation::Direction widthAnimationDirection;
};

ScrollBar::Implementation::Implementation(ScrollBar* _q, bool _stickToParent)
    : q(_q)
    , stickToParent(_stickToParent)
    , widthAnimationDebouncer(60)
{
    widthAnimation.setDuration(120);
    widthAnimation.setStartValue(Ui::DesignSystem::scrollBar().minimumSize());
    widthAnimation.setEndValue(Ui::DesignSystem::scrollBar().maximumSize());
    widthAnimation.setEasingCurve(QEasingCurve::OutQuad);
}

void ScrollBar::Implementation::correctScrollbarGeomerty()
{
    if (stickToParent) {
        if (q->orientation() == Qt::Vertical) {
            if (q->isLeftToRight()) {
                q->move(q->parentWidget()->width() - q->width(), 0);
            } else {
                q->move(0, 0);
            }
            q->resize(q->sizeHint().width(), q->parentWidget()->height());
        } else {
            q->move(0, q->parentWidget()->height() - q->height());
            q->resize(q->parentWidget()->width(), q->sizeHint().height());
        }
    } else {
        q->updateGeometry();
    }
}

void ScrollBar::Implementation::maximizeScrollbar()
{
    //
    // Если пользователь просто пронёс курсор мимо, то игнорируем это событие
    //
    if (widthAnimationDebouncer.hasPendingWork()
        && widthAnimationDirection == QVariantAnimation::Backward) {
        widthAnimationDebouncer.abortWork();
        return;
    }

    widthAnimationDirection = QVariantAnimation::Forward;
    widthAnimationDebouncer.orderWork();
}

void ScrollBar::Implementation::minimizeScrollbar()
{
    //
    // Если пользователь просто пронёс курсор мимо, то игнорируем это событие
    //
    if (widthAnimationDebouncer.hasPendingWork()
        && widthAnimationDirection == QVariantAnimation::Forward) {
        widthAnimationDebouncer.abortWork();
        return;
    }

    widthAnimationDirection = QVariantAnimation::Backward;
    widthAnimationDebouncer.orderWork();
}


// ****


ScrollBar::ScrollBar(QWidget* _parent, bool _stickToParent)
    : QScrollBar(_parent)
    , d(new Implementation(this, _stickToParent))
{
    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground);

    if (_parent != nullptr) {
        _parent->installEventFilter(this);
    }

    connect(&d->widthAnimation, &QVariantAnimation::valueChanged, this,
            [this] { d->correctScrollbarGeomerty(); });
    connect(&d->widthAnimationDebouncer, &Debouncer::gotWork, this, [this] {
        if (d->widthAnimationDirection == QVariantAnimation::Forward && maximum() <= minimum()) {
            return;
        }

        d->widthAnimation.setDirection(d->widthAnimationDirection);
        d->widthAnimation.start();

        //
        // Настраиваем длительность ожидания таким образом, чтобы полоса прокрутки сжималась
        // не так быстро и пользователь мог вернуться для взаимодействия с ней
        //
        d->widthAnimationDebouncer.setDelay(
            d->widthAnimationDirection == QVariantAnimation::Forward ? 800 : 60);
    });
}

ScrollBar::~ScrollBar() = default;

QSize ScrollBar::sizeHint() const
{
    const QSize marginsDelta = QSizeF(Ui::DesignSystem::scrollBar().margins().left()
                                          + Ui::DesignSystem::scrollBar().margins().right(),
                                      Ui::DesignSystem::scrollBar().margins().top()
                                          + Ui::DesignSystem::scrollBar().margins().bottom())
                                   .toSize();
    const qreal directedSize = std::max(Ui::DesignSystem::scrollBar().minimumSize(),
                                        d->widthAnimation.currentValue().toReal());
    if (orientation() == Qt::Vertical) {
        return QSize(static_cast<int>(directedSize), 10) + marginsDelta;
    } else {
        return QSize(10, static_cast<int>(directedSize)) + marginsDelta;
    }
}

bool ScrollBar::eventFilter(QObject* _watched, QEvent* _event)
{
    if (d->stickToParent) {
        switch (_event->type()) {
        //
        // Всегда остаёмся наверху
        //
        case QEvent::ChildAdded: {
            raise();
            break;
        }

        //
        // Обновим положение и размер при изменении размера родителя
        //
        case QEvent::Resize: {
            d->correctScrollbarGeomerty();
            if (auto scrollView = qobject_cast<QAbstractScrollArea*>(parentWidget())) {
                const auto syncSource = orientation() == Qt::Vertical
                    ? scrollView->verticalScrollBar()
                    : scrollView->horizontalScrollBar();
                setSingleStep(syncSource->singleStep());
                setPageStep(syncSource->pageStep());
            }
            break;
        }

        default: {
            break;
        }
        }
    }

    return QScrollBar::eventFilter(_watched, _event);
}

void ScrollBar::setHandleColor(const QColor& _handleColor)
{
    if (d->handleColor == _handleColor) {
        return;
    }

    d->handleColor = _handleColor;
    update();
}

void ScrollBar::setBackgroundColor(const QColor& _backgroundColor)
{
    if (d->backgroundColor == _backgroundColor) {
        return;
    }

    d->backgroundColor = _backgroundColor;
    update();
}

void ScrollBar::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    QPainter painter(this);
    painter.fillRect(rect(), Qt::transparent);

    if (maximum() <= minimum()) {
        return;
    }

    //
    // Настраиваем видимость полосы прокрутки
    //
    const qreal opacity = d->widthAnimation.currentValue().toReal();
    painter.setOpacity(opacity);

    //
    // Рисуем фон скролбара
    //
    const auto backgroundColor = d->backgroundColor.isValid()
        ? d->backgroundColor
        : Ui::DesignSystem::scrollBar().backgroundColor();
    const QRectF contentRect
        = QRectF(rect()).marginsRemoved(Ui::DesignSystem::scrollBar().margins());
    painter.fillRect(contentRect, backgroundColor);

    //
    // Рисуем хэндл
    //
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    const QRectF handle
        = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);
    const auto handleColor
        = d->handleColor.isValid() ? d->handleColor : Ui::DesignSystem::scrollBar().handleColor();
    painter.fillRect(handle, handleColor);
}

#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
void ScrollBar::enterEvent(QEnterEvent* _event)
#else
void ScrollBar::enterEvent(QEvent* _event)
#endif
{
    Q_UNUSED(_event)
    d->maximizeScrollbar();
}

void ScrollBar::leaveEvent(QEvent* _event)
{
    Q_UNUSED(_event)
    d->minimizeScrollbar();
}
