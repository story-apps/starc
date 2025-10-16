#include "page_text_edit_scroll_bar.h"

#include "qapplication.h"
#include "qcursor.h"
#include "qevent.h"
#include "qpainter.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qstylepainter.h"
#if QT_CONFIG(menu)
#include "qmenu.h"
#endif
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qpointer.h>
#if QT_CONFIG(accessibility)
#include "qaccessible.h"
#endif
#include "page_text_edit_scroll_bar_p.h"

#include <limits.h>

/*!
    \class PageTextEditScrollBar
    \brief The PageTextEditScrollBar widget provides a vertical or horizontal scroll bar.
    \ingroup basicwidgets
    \inmodule QtWidgets
    A scroll bar is a control that enables the user to access parts of a
    document that is larger than the widget used to display it. It provides
    a visual indication of the user's current position within the document
    and the amount of the document that is visible. Scroll bars are usually
    equipped with other controls that enable more accurate navigation.
    Qt displays scroll bars in a way that is appropriate for each platform.
    If you need to provide a scrolling view onto another widget, it may be
    more convenient to use the QScrollArea class because this provides a
    viewport widget and scroll bars. PageTextEditScrollBar is useful if you need to
    implement similar functionality for specialized widgets using QAbstractScrollArea;
    for example, if you decide to subclass QAbstractItemView.
    For most other situations where a slider control is used to obtain a value
    within a given range, the QSlider class may be more appropriate for your
    needs.
    \table
    \row \li \image PageTextEditScrollBar-picture.png
    \li Scroll bars typically include four separate controls: a slider,
    scroll arrows, and a page control.
    \list
    \li a. The slider provides a way to quickly go to any part of the
    document, but does not support accurate navigation within large
    documents.
    \li b. The scroll arrows are push buttons which can be used to accurately
    navigate to a particular place in a document. For a vertical scroll bar
    connected to a text editor, these typically move the current position one
    "line" up or down, and adjust the position of the slider by a small
    amount. In editors and list boxes a "line" might mean one line of text;
    in an image viewer it might mean 20 pixels.
    \li c. The page control is the area over which the slider is dragged (the
    scroll bar's background). Clicking here moves the scroll bar towards
    the click by one "page". This value is usually the same as the length of
    the slider.
    \endlist
    \endtable
    Each scroll bar has a value that indicates how far the slider is from
    the start of the scroll bar; this is obtained with value() and set
    with setValue(). This value always lies within the range of values
    defined for the scroll bar, from \l{AbstractSlider::minimum()}{minimum()}
    to \l{AbstractSlider::minimum()}{maximum()} inclusive. The range of
    acceptable values can be set with setMinimum() and setMaximum().
    At the minimum value, the top edge of the slider (for a vertical scroll
    bar) or left edge (for a horizontal scroll bar) will be at the top (or
    left) end of the scroll bar. At the maximum value, the bottom (or right)
    edge of the slider will be at the bottom (or right) end of the scroll bar.
    The length of the slider is usually related to the value of the page step,
    and typically represents the proportion of the document area shown in a
    scrolling view. The page step is the amount that the value changes by
    when the user presses the \uicontrol{Page Up} and \uicontrol{Page Down} keys, and is
    set with setPageStep(). Smaller changes to the value defined by the
    line step are made using the cursor keys, and this quantity is set with
    \l{AbstractSlider::}{setSingleStep()}.
    Note that the range of values used is independent of the actual size
    of the scroll bar widget. You do not need to take this into account when
    you choose values for the range and the page step.
    The range of values specified for the scroll bar are often determined
    differently to those for a QSlider because the length of the slider
    needs to be taken into account. If we have a document with 100 lines,
    and we can only show 20 lines in a widget, we may wish to construct a
    scroll bar with a page step of 20, a minimum value of 0, and a maximum
    value of 80. This would give us a scroll bar with five "pages".
    \table
    \row \li \inlineimage PageTextEditScrollBar-values.png
    \li The relationship between a document length, the range of values used
    in a scroll bar, and the page step is simple in many common situations.
    The scroll bar's range of values is determined by subtracting a
    chosen page step from some value representing the length of the document.
    In such cases, the following equation is useful:
    \e{document length} = maximum() - minimum() + pageStep().
    \endtable
    PageTextEditScrollBar only provides integer ranges. Note that although
    PageTextEditScrollBar handles very large numbers, scroll bars on current
    screens cannot usefully represent ranges above about 100,000 pixels.
    Beyond that, it becomes difficult for the user to control the
    slider using either the keyboard or the mouse, and the scroll
    arrows will have limited use.
    PageTextEditScrollBar inherits a comprehensive set of signals from AbstractSlider:
    \list
    \li \l{AbstractSlider::valueChanged()}{valueChanged()} is emitted when the
       scroll bar's value has changed. The tracking() determines whether this
       signal is emitted during user interaction.
    \li \l{AbstractSlider::rangeChanged()}{rangeChanged()} is emitted when the
       scroll bar's range of values has changed.
    \li \l{AbstractSlider::sliderPressed()}{sliderPressed()} is emitted when
       the user starts to drag the slider.
    \li \l{AbstractSlider::sliderMoved()}{sliderMoved()} is emitted when the user
       drags the slider.
    \li \l{AbstractSlider::sliderReleased()}{sliderReleased()} is emitted when
       the user releases the slider.
    \li \l{AbstractSlider::actionTriggered()}{actionTriggered()} is emitted
       when the scroll bar is changed by user interaction or via the
       \l{AbstractSlider::triggerAction()}{triggerAction()} function.
    \endlist
    A scroll bar can be controlled by the keyboard, but it has a
    default focusPolicy() of Qt::NoFocus. Use setFocusPolicy() to
    enable keyboard interaction with the scroll bar:
    \list
         \li Left/Right move a horizontal scroll bar by one single step.
         \li Up/Down move a vertical scroll bar by one single step.
         \li PageUp moves up one page.
         \li PageDown moves down one page.
         \li Home moves to the start (minimum).
         \li End moves to the end (maximum).
     \endlist
    The slider itself can be controlled by using the
    \l{AbstractSlider::triggerAction()}{triggerAction()} function to simulate
    user interaction with the scroll bar controls. This is useful if you have
    many different widgets that use a common range of values.
    Most GUI styles use the pageStep() value to calculate the size of the
    slider.
    \sa QScrollArea, QSlider, QDial, QSpinBox, {Sliders Example}
*/
bool PageTextEditScrollBarPrivate::updateHoverControl(const QPoint& pos)
{
    Q_Q(PageTextEditScrollBar);
    QRect lastHoverRect = hoverRect;
    QStyle::SubControl lastHoverControl = hoverControl;
    bool doesHover = q->testAttribute(Qt::WA_Hover);
    if (lastHoverControl != newHoverControl(pos) && doesHover) {
        q->update(lastHoverRect);
        q->update(hoverRect);
        return true;
    }
    return !doesHover;
}
QStyle::SubControl PageTextEditScrollBarPrivate::newHoverControl(const QPoint& pos)
{
    Q_Q(PageTextEditScrollBar);
    QStyleOptionSlider opt;
    q->initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    hoverControl = q->style()->hitTestComplexControl(QStyle::CC_ScrollBar, &opt, pos, q);
    if (hoverControl == QStyle::SC_None)
        hoverRect = QRect();
    else
        hoverRect = q->style()->subControlRect(QStyle::CC_ScrollBar, &opt, hoverControl, q);
    return hoverControl;
}
void PageTextEditScrollBarPrivate::setTransient(bool value)
{
    Q_Q(PageTextEditScrollBar);
    if (transient != value) {
        transient = value;
        if (q->isVisible()) {
            QStyleOptionSlider opt;
            q->initStyleOption(&opt);
            if (q->style()->styleHint(QStyle::SH_ScrollBar_Transient, &opt, q))
                q->update();
        } else if (!transient) {
            q->show();
        }
    }
}
void PageTextEditScrollBarPrivate::flash()
{
    Q_Q(PageTextEditScrollBar);
    QStyleOptionSlider opt;
    q->initStyleOption(&opt);
    if (!flashed && q->style()->styleHint(QStyle::SH_ScrollBar_Transient, &opt, q)) {
        flashed = true;
        if (!q->isVisible())
            q->show();
        else
            q->update();
    }
    if (!flashTimer)
        flashTimer = q->startTimer(0);
}
void PageTextEditScrollBarPrivate::activateControl(uint control, int threshold)
{
    AbstractSlider::SliderAction action = AbstractSlider::SliderNoAction;
    switch (control) {
    case QStyle::SC_ScrollBarAddPage:
        action = AbstractSlider::SliderPageStepAdd;
        break;
    case QStyle::SC_ScrollBarSubPage:
        action = AbstractSlider::SliderPageStepSub;
        break;
    case QStyle::SC_ScrollBarAddLine:
        action = AbstractSlider::SliderSingleStepAdd;
        break;
    case QStyle::SC_ScrollBarSubLine:
        action = AbstractSlider::SliderSingleStepSub;
        break;
    case QStyle::SC_ScrollBarFirst:
        action = AbstractSlider::SliderToMinimum;
        break;
    case QStyle::SC_ScrollBarLast:
        action = AbstractSlider::SliderToMaximum;
        break;
    default:
        break;
    }
    if (action) {
        q_func()->setRepeatAction(action, threshold);
        q_func()->triggerAction(action);
    }
}
void PageTextEditScrollBarPrivate::stopRepeatAction()
{
    Q_Q(PageTextEditScrollBar);
    QStyle::SubControl tmp = pressedControl;
    q->setRepeatAction(AbstractSlider::SliderNoAction);
    pressedControl = QStyle::SC_None;
    if (tmp == QStyle::SC_ScrollBarSlider)
        q->setSliderDown(false);
    QStyleOptionSlider opt;
    q->initStyleOption(&opt);
    q->repaint(q->style()->subControlRect(QStyle::CC_ScrollBar, &opt, tmp, q));
}
/*!
    Initialize \a option with the values from this PageTextEditScrollBar. This method
    is useful for subclasses when they need a QStyleOptionSlider, but don't want
    to fill in all the information themselves.
    \sa QStyleOption::initFrom()
*/
void PageTextEditScrollBar::initStyleOption(QStyleOptionSlider* option) const
{
    if (!option)
        return;
    Q_D(const PageTextEditScrollBar);
    option->initFrom(this);
    option->subControls = QStyle::SC_None;
    option->activeSubControls = QStyle::SC_None;
    option->orientation = d->orientation;
    option->minimum = d->minimum;
    option->maximum = d->maximum;
    option->sliderPosition = d->position;
    option->sliderValue = d->value;
    option->singleStep = d->singleStep;
    option->pageStep = d->pageStep;
    option->upsideDown = d->invertedAppearance;
    if (d->orientation == Qt::Horizontal)
        option->state |= QStyle::State_Horizontal;
    if ((d->flashed || !d->transient)
        && style()->styleHint(QStyle::SH_ScrollBar_Transient, option, this))
        option->state |= QStyle::State_On;
}
#define HORIZONTAL (d_func()->orientation == Qt::Horizontal)
#define VERTICAL !HORIZONTAL
/*!
    Constructs a vertical scroll bar.
    The \a parent argument is sent to the QWidget constructor.
    The \l {AbstractSlider::minimum} {minimum} defaults to 0, the
    \l {AbstractSlider::maximum} {maximum} to 99, with a
    \l {AbstractSlider::singleStep} {singleStep} size of 1 and a
    \l {AbstractSlider::pageStep} {pageStep} size of 10, and an
    initial \l {AbstractSlider::value} {value} of 0.
*/
PageTextEditScrollBar::PageTextEditScrollBar(QWidget* parent)
    : PageTextEditScrollBar(Qt::Vertical, parent)
{
}
/*!
    Constructs a scroll bar with the given \a orientation.
    The \a parent argument is passed to the QWidget constructor.
    The \l {AbstractSlider::minimum} {minimum} defaults to 0, the
    \l {AbstractSlider::maximum} {maximum} to 99, with a
    \l {AbstractSlider::singleStep} {singleStep} size of 1 and a
    \l {AbstractSlider::pageStep} {pageStep} size of 10, and an
    initial \l {AbstractSlider::value} {value} of 0.
*/
PageTextEditScrollBar::PageTextEditScrollBar(Qt::Orientation orientation, QWidget* parent)
    : AbstractSlider(*new PageTextEditScrollBarPrivate, parent)
{
    d_func()->orientation = orientation;
    d_func()->init();
}
/*!
    Destroys the scroll bar.
*/
PageTextEditScrollBar::~PageTextEditScrollBar()
{
}
void PageTextEditScrollBarPrivate::init()
{
    Q_Q(PageTextEditScrollBar);
    invertedControls = true;
    pressedControl = hoverControl = QStyle::SC_None;
    pointerOutsidePressedControl = false;
    QStyleOption opt;
    opt.initFrom(q);
    transient = q->style()->styleHint(QStyle::SH_ScrollBar_Transient, &opt, q);
    flashed = false;
    flashTimer = 0;
    q->setFocusPolicy(Qt::NoFocus);
    QSizePolicy sp(QSizePolicy::Minimum, QSizePolicy::Fixed, QSizePolicy::Slider);
    if (orientation == Qt::Vertical)
        sp.transpose();
    q->setSizePolicy(sp);
    q->setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    q->setAttribute(Qt::WA_OpaquePaintEvent);
}
#ifndef QT_NO_CONTEXTMENU
/*! \reimp */
void PageTextEditScrollBar::contextMenuEvent(QContextMenuEvent* event)
{
    if (!style()->styleHint(QStyle::SH_ScrollBar_ContextMenu, nullptr, this)) {
        AbstractSlider::contextMenuEvent(event);
        return;
    }
#if QT_CONFIG(menu)
    bool horiz = HORIZONTAL;
    QPointer<QMenu> menu = new QMenu(this);
    QAction* actScrollHere = menu->addAction(("Scroll here"));
    menu->addSeparator();
    QAction* actScrollTop = menu->addAction(horiz ? ("Left edge") : ("Top"));
    QAction* actScrollBottom = menu->addAction(horiz ? ("Right edge") : ("Bottom"));
    menu->addSeparator();
    QAction* actPageUp = menu->addAction(horiz ? ("Page left") : ("Page up"));
    QAction* actPageDn = menu->addAction(horiz ? ("Page right") : ("Page down"));
    menu->addSeparator();
    QAction* actScrollUp = menu->addAction(horiz ? ("Scroll left") : ("Scroll up"));
    QAction* actScrollDn = menu->addAction(horiz ? ("Scroll right") : ("Scroll down"));
    QAction* actionSelected = menu->exec(event->globalPos());
    delete menu;
    if (actionSelected == nullptr)
        /* do nothing */;
    else if (actionSelected == actScrollHere)
        setValue(d_func()->pixelPosToRangeValue(horiz ? event->pos().x() : event->pos().y()));
    else if (actionSelected == actScrollTop)
        triggerAction(AbstractSlider::SliderToMinimum);
    else if (actionSelected == actScrollBottom)
        triggerAction(AbstractSlider::SliderToMaximum);
    else if (actionSelected == actPageUp)
        triggerAction(AbstractSlider::SliderPageStepSub);
    else if (actionSelected == actPageDn)
        triggerAction(AbstractSlider::SliderPageStepAdd);
    else if (actionSelected == actScrollUp)
        triggerAction(AbstractSlider::SliderSingleStepSub);
    else if (actionSelected == actScrollDn)
        triggerAction(AbstractSlider::SliderSingleStepAdd);
#endif // QT_CONFIG(menu)
}
#endif // QT_NO_CONTEXTMENU
/*! \reimp */
QSize PageTextEditScrollBar::sizeHint() const
{
    ensurePolished();
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    int TextEditScrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent, &opt, this);
    int TextEditScrollBarSliderMin
        = style()->pixelMetric(QStyle::PM_ScrollBarSliderMin, &opt, this);
    QSize size;
    if (opt.orientation == Qt::Horizontal)
        size = QSize(TextEditScrollBarExtent * 2 + TextEditScrollBarSliderMin,
                     TextEditScrollBarExtent);
    else
        size = QSize(TextEditScrollBarExtent,
                     TextEditScrollBarExtent * 2 + TextEditScrollBarSliderMin);
    return style()->sizeFromContents(QStyle::CT_ScrollBar, &opt, size, this);
}
/*!\reimp */
void PageTextEditScrollBar::sliderChange(SliderChange change)
{
    AbstractSlider::sliderChange(change);
}
/*!
    \reimp
*/
bool PageTextEditScrollBar::event(QEvent* event)
{
    Q_D(PageTextEditScrollBar);
    switch (event->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        if (const QHoverEvent* he = static_cast<const QHoverEvent*>(event))
            d_func()->updateHoverControl(he->position().toPoint());
        break;
    case QEvent::StyleChange: {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        d_func()->setTransient(style()->styleHint(QStyle::SH_ScrollBar_Transient, &opt, this));
        break;
    }
    case QEvent::Timer:
        if (static_cast<QTimerEvent*>(event)->timerId() == d->flashTimer) {
            QStyleOptionSlider opt;
            initStyleOption(&opt);
            if (d->flashed && style()->styleHint(QStyle::SH_ScrollBar_Transient, &opt, this)) {
                d->flashed = false;
                update();
            }
            killTimer(d->flashTimer);
            d->flashTimer = 0;
        }
        break;
    default:
        break;
    }
    return AbstractSlider::event(event);
}
/*!
    \reimp
*/
#if QT_CONFIG(wheelevent)
void PageTextEditScrollBar::wheelEvent(QWheelEvent* event)
{
    event->ignore();
    bool horizontal = qAbs(event->angleDelta().x()) > qAbs(event->angleDelta().y());
    // The vertical wheel can be used to scroll a horizontal TextEditScrollBar, but only if
    // there is no simultaneous horizontal wheel movement.  This is to avoid chaotic
    // scrolling on touchpads.
    if (!horizontal && event->angleDelta().x() != 0 && orientation() == Qt::Horizontal)
        return;
    // TextEditScrollBar is a special case - in vertical mode it reaches minimum
    // value in the upper position, however QSlider's minimum value is on
    // the bottom. So we need to invert the value, but since the TextEditScrollBar is
    // inverted by default, we need to invert the delta value only for the
    // horizontal orientation.
    int delta = horizontal ? -event->angleDelta().x() : event->angleDelta().y();
    Q_D(PageTextEditScrollBar);
    if (d->scrollByDelta(horizontal ? Qt::Horizontal : Qt::Vertical, event->modifiers(), delta))
        event->accept();
    if (event->phase() == Qt::ScrollBegin)
        d->setTransient(false);
    else if (event->phase() == Qt::ScrollEnd)
        d->setTransient(true);
}
#endif
/*!
    \reimp
*/
void PageTextEditScrollBar::paintEvent(QPaintEvent*)
{
    Q_D(PageTextEditScrollBar);
    QStylePainter p(this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    if (d->pressedControl) {
        opt.activeSubControls = (QStyle::SubControl)d->pressedControl;
        if (!d->pointerOutsidePressedControl)
            opt.state |= QStyle::State_Sunken;
    } else {
        opt.activeSubControls = (QStyle::SubControl)d->hoverControl;
    }
    p.drawComplexControl(QStyle::CC_ScrollBar, opt);
}
/*!
    \reimp
*/
void PageTextEditScrollBar::mousePressEvent(QMouseEvent* e)
{
    Q_D(PageTextEditScrollBar);
    if (d->repeatActionTimer.isActive())
        d->stopRepeatAction();
    bool midButtonAbsPos
        = style()->styleHint(QStyle::SH_ScrollBar_MiddleClickAbsolutePosition, nullptr, this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    opt.keyboardModifiers = e->modifiers();
    if (d->maximum == d->minimum // no range
        || (e->buttons() & (~e->button())) // another button was clicked before
        || !(e->button() == Qt::LeftButton || (midButtonAbsPos && e->button() == Qt::MiddleButton)))
        return;
    d->pressedControl
        = style()->hitTestComplexControl(QStyle::CC_ScrollBar, &opt, e->position().toPoint(), this);
    d->pointerOutsidePressedControl = false;
    QRect sr
        = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);
    QPoint click = e->position().toPoint();
    QPoint pressValue = click - sr.center() + sr.topLeft();
    d->pressValue = d->orientation == Qt::Horizontal ? d->pixelPosToRangeValue(pressValue.x())
                                                     : d->pixelPosToRangeValue(pressValue.y());
    if (d->pressedControl == QStyle::SC_ScrollBarSlider) {
        d->clickOffset = HORIZONTAL ? (click.x() - sr.x()) : (click.y() - sr.y());
        d->snapBackPosition = d->position;
    }
    if ((d->pressedControl == QStyle::SC_ScrollBarAddPage
         || d->pressedControl == QStyle::SC_ScrollBarSubPage)
        && ((midButtonAbsPos && e->button() == Qt::MiddleButton)
            || (style()->styleHint(QStyle::SH_ScrollBar_LeftClickAbsolutePosition, &opt, this)
                && e->button() == Qt::LeftButton))) {
        int sliderLength = HORIZONTAL ? sr.width() : sr.height();
        setSliderPosition(d->pixelPosToRangeValue(
            (HORIZONTAL ? e->position().toPoint().x() : e->position().toPoint().y())
            - sliderLength / 2));
        d->pressedControl = QStyle::SC_ScrollBarSlider;
        d->clickOffset = sliderLength / 2;
    }
    const int initialDelay = 500; // default threshold
    QElapsedTimer time;
    time.start();
    d->activateControl(d->pressedControl, initialDelay);
    repaint(style()->subControlRect(QStyle::CC_ScrollBar, &opt, d->pressedControl, this));
    if (time.elapsed() >= initialDelay && d->repeatActionTimer.isActive()) {
        // It took more than 500ms (the initial timer delay) to process
        // the control activation and repaint(), we therefore need
        // to restart the timer in case we have a pending mouse release event;
        // otherwise we'll get a timer event right before the release event,
        // causing the repeat action to be invoked twice on a single mouse click.
        // 50ms is the default repeat time (see activateControl/setRepeatAction).
        d->repeatActionTimer.start(50, this);
    }
    if (d->pressedControl == QStyle::SC_ScrollBarSlider)
        setSliderDown(true);
}
/*!
    \reimp
*/
void PageTextEditScrollBar::mouseReleaseEvent(QMouseEvent* e)
{
    Q_D(PageTextEditScrollBar);
    if (!d->pressedControl)
        return;
    if (e->buttons() & (~e->button())) // some other button is still pressed
        return;
    d->stopRepeatAction();
}
/*!
    \reimp
*/
void PageTextEditScrollBar::mouseMoveEvent(QMouseEvent* e)
{
    Q_D(PageTextEditScrollBar);
    if (!d->pressedControl)
        return;
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    if (!(e->buttons() & Qt::LeftButton
          || ((e->buttons() & Qt::MiddleButton)
              && style()->styleHint(QStyle::SH_ScrollBar_MiddleClickAbsolutePosition, &opt, this))))
        return;
    if (d->pressedControl == QStyle::SC_ScrollBarSlider) {
        QPoint click = e->position().toPoint();
        int newPosition
            = d->pixelPosToRangeValue((HORIZONTAL ? click.x() : click.y()) - d->clickOffset);
        int m = style()->pixelMetric(QStyle::PM_MaximumDragDistance, &opt, this);
        if (m >= 0) {
            QRect r = rect();
            r.adjust(-m, -m, m, m);
            if (!r.contains(e->position().toPoint()))
                newPosition = d->snapBackPosition;
        }
        setSliderPosition(newPosition);
    } else if (!style()->styleHint(QStyle::SH_ScrollBar_ScrollWhenPointerLeavesControl, &opt,
                                   this)) {
        if (style()->styleHint(QStyle::SH_ScrollBar_RollBetweenButtons, &opt, this)
            && d->pressedControl & (QStyle::SC_ScrollBarAddLine | QStyle::SC_ScrollBarSubLine)) {
            QStyle::SubControl newSc = style()->hitTestComplexControl(
                QStyle::CC_ScrollBar, &opt, e->position().toPoint(), this);
            if (newSc == d->pressedControl && !d->pointerOutsidePressedControl)
                return; // nothing to do
            if (newSc & (QStyle::SC_ScrollBarAddLine | QStyle::SC_ScrollBarSubLine)) {
                d->pointerOutsidePressedControl = false;
                QRect scRect = style()->subControlRect(QStyle::CC_ScrollBar, &opt, newSc, this);
                scRect
                    |= style()->subControlRect(QStyle::CC_ScrollBar, &opt, d->pressedControl, this);
                d->pressedControl = newSc;
                d->activateControl(d->pressedControl, 0);
                update(scRect);
                return;
            }
        }
        // stop scrolling when the mouse pointer leaves a control
        // similar to push buttons
        QRect pr = style()->subControlRect(QStyle::CC_ScrollBar, &opt, d->pressedControl, this);
        if (pr.contains(e->position().toPoint()) == d->pointerOutsidePressedControl) {
            if ((d->pointerOutsidePressedControl = !d->pointerOutsidePressedControl)) {
                d->pointerOutsidePressedControl = true;
                setRepeatAction(SliderNoAction);
                repaint(pr);
            } else {
                d->activateControl(d->pressedControl);
            }
        }
    }
}
int PageTextEditScrollBarPrivate::pixelPosToRangeValue(int pos) const
{
    Q_Q(const PageTextEditScrollBar);
    QStyleOptionSlider opt;
    q->initStyleOption(&opt);
    QRect gr
        = q->style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove, q);
    QRect sr
        = q->style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, q);
    int sliderMin, sliderMax, sliderLength;
    if (orientation == Qt::Horizontal) {
        sliderLength = sr.width();
        sliderMin = gr.x();
        sliderMax = gr.right() - sliderLength + 1;
        if (q->layoutDirection() == Qt::RightToLeft)
            opt.upsideDown = !opt.upsideDown;
    } else {
        sliderLength = sr.height();
        sliderMin = gr.y();
        sliderMax = gr.bottom() - sliderLength + 1;
    }
    return QStyle::sliderValueFromPosition(minimum, maximum, pos - sliderMin, sliderMax - sliderMin,
                                           opt.upsideDown);
}
/*! \reimp
 */
void PageTextEditScrollBar::hideEvent(QHideEvent*)
{
    Q_D(PageTextEditScrollBar);
    if (d->pressedControl) {
        d->pressedControl = QStyle::SC_None;
        setRepeatAction(SliderNoAction);
    }
}
