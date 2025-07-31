#include "abstract_slider.h"

#include "abstract_slider_p.h"
#include "qaccessible.h"
#include "qdebug.h"
#include "qevent.h"

#include <private/qapplication_p.h>

#include <qapplication.h>

#include <limits.h>

/*!
    \class AbstractSlider
    \brief The AbstractSlider class provides an integer value within a range.
    \ingroup abstractwidgets
    \inmodule QtWidgets
    The class is designed as a common super class for widgets like
    QScrollBar, QSlider and QDial.
    Here are the main properties of the class:
    \list 1
    \li \l value: The bounded integer that AbstractSlider maintains.
    \li \l minimum: The lowest possible value.
    \li \l maximum: The highest possible value.
    \li \l singleStep: The smaller of two natural steps that an
    abstract sliders provides and typically corresponds to the user
    pressing an arrow key.
    \li \l pageStep: The larger of two natural steps that an abstract
    slider provides and typically corresponds to the user pressing
    PageUp or PageDown.
    \li \l tracking: Whether slider tracking is enabled.
    \li \l sliderPosition: The current position of the slider. If \l
    tracking is enabled (the default), this is identical to \l value.
    \endlist
    Unity (1) may be viewed as a third step size. setValue() lets you
    set the current value to any integer in the allowed range, not
    just minimum() + \e n * singleStep() for integer values of \e n.
    Some widgets may allow the user to set any value at all; others
    may just provide multiples of singleStep() or pageStep().
    AbstractSlider emits a comprehensive set of signals:
    \table
    \header \li Signal \li Emitted when
    \row \li \l valueChanged()
         \li the value has changed. The \l tracking
            determines whether this signal is emitted during user
            interaction.
    \row \li \l sliderPressed()
         \li the user starts to drag the slider.
    \row \li \l sliderMoved()
         \li the user drags the slider.
    \row \li \l sliderReleased()
         \li the user releases the slider.
    \row \li \l actionTriggered()
         \li a slider action was triggered.
    \row \li \l rangeChanged()
         \li a the range has changed.
    \endtable
    AbstractSlider provides a virtual sliderChange() function that is
    well suited for updating the on-screen representation of
    sliders. By calling triggerAction(), subclasses trigger slider
    actions. Two helper functions QStyle::sliderPositionFromValue() and
    QStyle::sliderValueFromPosition() help subclasses and styles to map
    screen coordinates to logical range values.
    \sa QAbstractSpinBox, QSlider, QDial, QScrollBar, {Sliders Example}
*/
/*!
    \enum AbstractSlider::SliderAction
    \value SliderNoAction
    \value SliderSingleStepAdd
    \value SliderSingleStepSub
    \value SliderPageStepAdd
    \value SliderPageStepSub
    \value SliderToMinimum
    \value SliderToMaximum
    \value SliderMove
*/
/*!
    \fn void AbstractSlider::valueChanged(int value)
    This signal is emitted when the slider value has changed, with the
    new slider \a value as argument.
*/
/*!
    \fn void AbstractSlider::sliderPressed()
    This signal is emitted when the user presses the slider with the
    mouse, or programmatically when setSliderDown(true) is called.
    \sa sliderReleased(), sliderMoved(), isSliderDown()
*/
/*!
    \fn void AbstractSlider::sliderMoved(int value)
    This signal is emitted when sliderDown is true and the slider moves. This
    usually happens when the user is dragging the slider. The \a value
    is the new slider position.
    This signal is emitted even when tracking is turned off.
    \sa setTracking(), valueChanged(), isSliderDown(),
    sliderPressed(), sliderReleased()
*/
/*!
    \fn void AbstractSlider::sliderReleased()
    This signal is emitted when the user releases the slider with the
    mouse, or programmatically when setSliderDown(false) is called.
    \sa sliderPressed(), sliderMoved(), sliderDown
*/
/*!
    \fn void AbstractSlider::rangeChanged(int min, int max)
    This signal is emitted when the slider range has changed, with \a
    min being the new minimum, and \a max being the new maximum.
    \sa minimum, maximum
*/
/*!
    \fn void AbstractSlider::actionTriggered(int action)
    This signal is emitted when the slider action \a action is
    triggered. Actions are \l SliderSingleStepAdd, \l
    SliderSingleStepSub, \l SliderPageStepAdd, \l SliderPageStepSub,
    \l SliderToMinimum, \l SliderToMaximum, and \l SliderMove.
    When the signal is emitted, the \l sliderPosition has been
    adjusted according to the action, but the \l value has not yet
    been propagated (meaning the valueChanged() signal was not yet
    emitted), and the visual display has not been updated. In slots
    connected to this signal you can thus safely adjust any action by
    calling setSliderPosition() yourself, based on both the action and
    the slider's value.
    \sa triggerAction()
*/
/*!
    \enum AbstractSlider::SliderChange
    \value SliderRangeChange
    \value SliderOrientationChange
    \value SliderStepsChange
    \value SliderValueChange
*/
AbstractSliderPrivate::AbstractSliderPrivate()
    : minimum(0)
    , maximum(99)
    , pageStep(10)
    , value(0)
    , position(0)
    , pressValue(-1)
    , singleStep(1)
    , singleStepFromItemView(-1)
    , viewMayChangeSingleStep(true)
    , offset_accumulated(0)
    , tracking(true)
    , blocktracking(false)
    , pressed(false)
    , invertedAppearance(false)
    , invertedControls(false)
    , orientation(Qt::Horizontal)
    , repeatAction(AbstractSlider::SliderNoAction)
#ifdef QT_KEYPAD_NAVIGATION
    , isAutoRepeating(false)
    , repeatMultiplier(1)
{
    firstRepeat.invalidate();
#else
{
#endif
}
AbstractSliderPrivate::~AbstractSliderPrivate()
{
}
/*!
    Sets the slider's minimum to \a min and its maximum to \a max.
    If \a max is smaller than \a min, \a min becomes the only legal
    value.
    \sa minimum, maximum
*/
void AbstractSlider::setRange(int min, int max)
{
    Q_D(AbstractSlider);
    int oldMin = d->minimum;
    int oldMax = d->maximum;
    d->minimum = min;
    d->maximum = qMax(min, max);
    if (oldMin != d->minimum || oldMax != d->maximum) {
        sliderChange(SliderRangeChange);
        emit rangeChanged(d->minimum, d->maximum);
        setValue(d->value); // re-bound
    }
}
void AbstractSliderPrivate::setSteps(int single, int page)
{
    Q_Q(AbstractSlider);
    singleStep = qAbs(single);
    pageStep = qAbs(page);
    q->sliderChange(AbstractSlider::SliderStepsChange);
}
/*!
    Constructs an abstract slider.
    The \a parent argument is sent to the QWidget constructor.
    The \l minimum defaults to 0, the \l maximum to 99, with a \l
    singleStep size of 1 and a \l pageStep size of 10, and an initial
    \l value of 0.
*/
AbstractSlider::AbstractSlider(QWidget* parent)
    : QWidget(*new AbstractSliderPrivate, parent, {})
{
}
/*! \internal */
AbstractSlider::AbstractSlider(AbstractSliderPrivate& dd, QWidget* parent)
    : QWidget(dd, parent, {})
{
}
/*!
    Destroys the slider.
*/
AbstractSlider::~AbstractSlider()
{
}
/*!
    \property AbstractSlider::orientation
    \brief the orientation of the slider
    The orientation must be \l Qt::Vertical (the default) or \l
    Qt::Horizontal.
*/
void AbstractSlider::setOrientation(Qt::Orientation orientation)
{
    Q_D(AbstractSlider);
    if (d->orientation == orientation)
        return;
    d->orientation = orientation;
    if (!testAttribute(Qt::WA_WState_OwnSizePolicy)) {
        setSizePolicy(sizePolicy().transposed());
        setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    }
    update();
    updateGeometry();
}
Qt::Orientation AbstractSlider::orientation() const
{
    Q_D(const AbstractSlider);
    return d->orientation;
}
/*!
    \property AbstractSlider::minimum
    \brief the sliders's minimum value
    When setting this property, the \l maximum is adjusted if
    necessary to ensure that the range remains valid. Also the
    slider's current value is adjusted to be within the new range.
*/
void AbstractSlider::setMinimum(int min)
{
    Q_D(AbstractSlider);
    setRange(min, qMax(d->maximum, min));
}
int AbstractSlider::minimum() const
{
    Q_D(const AbstractSlider);
    return d->minimum;
}
/*!
    \property AbstractSlider::maximum
    \brief the slider's maximum value
    When setting this property, the \l minimum is adjusted if
    necessary to ensure that the range remains valid.  Also the
    slider's current value is adjusted to be within the new range.
*/
void AbstractSlider::setMaximum(int max)
{
    Q_D(AbstractSlider);
    setRange(qMin(d->minimum, max), max);
}
int AbstractSlider::maximum() const
{
    Q_D(const AbstractSlider);
    return d->maximum;
}
/*!
    \property AbstractSlider::singleStep
    \brief the single step.
    The smaller of two natural steps that an
    abstract sliders provides and typically corresponds to the user
    pressing an arrow key.
    If the property is modified during an auto repeating key event, behavior
    is undefined.
    \sa pageStep
*/
void AbstractSlider::setSingleStep(int step)
{
    Q_D(AbstractSlider);
    d->viewMayChangeSingleStep = (step < 0);
    if (step < 0 && d->singleStepFromItemView > 0)
        step = d->singleStepFromItemView;
    if (step != d->singleStep)
        d->setSteps(step, d->pageStep);
}
int AbstractSlider::singleStep() const
{
    Q_D(const AbstractSlider);
    return d->singleStep;
}
/*!
    \property AbstractSlider::pageStep
    \brief the page step.
    The larger of two natural steps that an abstract slider provides
    and typically corresponds to the user pressing PageUp or PageDown.
    \sa singleStep
*/
void AbstractSlider::setPageStep(int step)
{
    Q_D(AbstractSlider);
    if (step != d->pageStep)
        d->setSteps(d->singleStep, step);
}
int AbstractSlider::pageStep() const
{
    Q_D(const AbstractSlider);
    return d->pageStep;
}
/*!
    \property AbstractSlider::tracking
    \brief whether slider tracking is enabled
    If tracking is enabled (the default), the slider emits the
    valueChanged() signal while the slider is being dragged. If
    tracking is disabled, the slider emits the valueChanged() signal
    only when the user releases the slider.
    \sa sliderDown
*/
void AbstractSlider::setTracking(bool enable)
{
    Q_D(AbstractSlider);
    d->tracking = enable;
}
bool AbstractSlider::hasTracking() const
{
    Q_D(const AbstractSlider);
    return d->tracking;
}
/*!
    \property AbstractSlider::sliderDown
    \brief whether the slider is pressed down.
    The property is set by subclasses in order to let the abstract
    slider know whether or not \l tracking has any effect.
    Changing the slider down property emits the sliderPressed() and
    sliderReleased() signals.
*/
void AbstractSlider::setSliderDown(bool down)
{
    Q_D(AbstractSlider);
    bool doEmit = d->pressed != down;
    d->pressed = down;
    if (doEmit) {
        if (down)
            emit sliderPressed();
        else
            emit sliderReleased();
    }
    if (!down && d->position != d->value)
        triggerAction(SliderMove);
}
bool AbstractSlider::isSliderDown() const
{
    Q_D(const AbstractSlider);
    return d->pressed;
}
/*!
    \property AbstractSlider::sliderPosition
    \brief the current slider position
    If \l tracking is enabled (the default), this is identical to \l value.
*/
void AbstractSlider::setSliderPosition(int position)
{
    Q_D(AbstractSlider);
    position = d->bound(position);
    if (position == d->position)
        return;
    d->position = position;
    if (!d->tracking)
        update();
    if (d->pressed)
        emit sliderMoved(position);
    if (d->tracking && !d->blocktracking)
        triggerAction(SliderMove);
}
int AbstractSlider::sliderPosition() const
{
    Q_D(const AbstractSlider);
    return d->position;
}
/*!
    \property AbstractSlider::value
    \brief the slider's current value
    The slider forces the value to be within the legal range: \l
    minimum <= \c value <= \l maximum.
    Changing the value also changes the \l sliderPosition.
*/
int AbstractSlider::value() const
{
    Q_D(const AbstractSlider);
    return d->value;
}
void AbstractSlider::setValue(int value)
{
    Q_D(AbstractSlider);
    value = d->bound(value);
    if (d->value == value && d->position == value)
        return;
    // delay signal emission until sliderChanged() has been called
    const bool emitValueChanged = (value != d->value);
    d->value = value;
    if (d->position != value) {
        d->position = value;
        if (d->pressed)
            emit sliderMoved(d->position);
    }
#if QT_CONFIG(accessibility)
    QAccessibleValueChangeEvent event(this, d->value);
    QAccessible::updateAccessibility(&event);
#endif
    sliderChange(SliderValueChange);
    if (emitValueChanged)
        emit valueChanged(value);
}
/*!
    \property AbstractSlider::invertedAppearance
    \brief whether or not a slider shows its values inverted.
    If this property is \c false (the default), the minimum and maximum will
    be shown in its classic position for the inherited widget. If the
    value is true, the minimum and maximum appear at their opposite location.
    Note: This property makes most sense for sliders and dials. For
    scroll bars, the visual effect of the scroll bar subcontrols depends on
    whether or not the styles understand inverted appearance; most styles
    ignore this property for scroll bars.
*/
bool AbstractSlider::invertedAppearance() const
{
    Q_D(const AbstractSlider);
    return d->invertedAppearance;
}
void AbstractSlider::setInvertedAppearance(bool invert)
{
    Q_D(AbstractSlider);
    d->invertedAppearance = invert;
    update();
}
/*!
    \property AbstractSlider::invertedControls
    \brief whether or not the slider inverts its wheel and key events.
    If this property is \c false, scrolling the mouse wheel "up" and using keys
    like page up will increase the slider's value towards its maximum. Otherwise
    pressing page up will move value towards the slider's minimum.
*/
bool AbstractSlider::invertedControls() const
{
    Q_D(const AbstractSlider);
    return d->invertedControls;
}
void AbstractSlider::setInvertedControls(bool invert)
{
    Q_D(AbstractSlider);
    d->invertedControls = invert;
}
/*!  Triggers a slider \a action.  Possible actions are \l
  SliderSingleStepAdd, \l SliderSingleStepSub, \l SliderPageStepAdd,
  \l SliderPageStepSub, \l SliderToMinimum, \l SliderToMaximum, and \l
  SliderMove.
  \sa actionTriggered()
 */
void AbstractSlider::triggerAction(SliderAction action)
{
    Q_D(AbstractSlider);
    d->blocktracking = true;
    switch (action) {
    case SliderSingleStepAdd:
        setSliderPosition(d->overflowSafeAdd(d->effectiveSingleStep()));
        break;
    case SliderSingleStepSub:
        setSliderPosition(d->overflowSafeAdd(-d->effectiveSingleStep()));
        break;
    case SliderPageStepAdd:
        setSliderPosition(d->overflowSafeAdd(d->pageStep));
        break;
    case SliderPageStepSub:
        setSliderPosition(d->overflowSafeAdd(-d->pageStep));
        break;
    case SliderToMinimum:
        setSliderPosition(d->minimum);
        break;
    case SliderToMaximum:
        setSliderPosition(d->maximum);
        break;
    case SliderMove:
    case SliderNoAction:
        break;
    };
    emit actionTriggered(action);
    d->blocktracking = false;
    setValue(d->position);
}
/*!  Sets action \a action to be triggered repetitively in intervals
of \a repeatTime, after an initial delay of \a thresholdTime.
\sa triggerAction(), repeatAction()
 */
void AbstractSlider::setRepeatAction(SliderAction action, int thresholdTime, int repeatTime)
{
    Q_D(AbstractSlider);
    if ((d->repeatAction = action) == SliderNoAction) {
        d->repeatActionTimer.stop();
    } else {
        d->repeatActionTime = repeatTime;
        d->repeatActionTimer.start(thresholdTime, this);
    }
}
/*!
  Returns the current repeat action.
  \sa setRepeatAction()
 */
AbstractSlider::SliderAction AbstractSlider::repeatAction() const
{
    Q_D(const AbstractSlider);
    return d->repeatAction;
}
/*!\reimp
 */
void AbstractSlider::timerEvent(QTimerEvent* e)
{
    Q_D(AbstractSlider);
    if (e->timerId() == d->repeatActionTimer.timerId()) {
        if (d->repeatActionTime) { // was threshold time, use repeat time next time
            d->repeatActionTimer.start(d->repeatActionTime, this);
            d->repeatActionTime = 0;
        }
        if (d->repeatAction == SliderPageStepAdd)
            d->setAdjustedSliderPosition(d->overflowSafeAdd(d->pageStep));
        else if (d->repeatAction == SliderPageStepSub)
            d->setAdjustedSliderPosition(d->overflowSafeAdd(-d->pageStep));
        else
            triggerAction(d->repeatAction);
    }
}
/*!
    Reimplement this virtual function to track slider changes such as
    \l SliderRangeChange, \l SliderOrientationChange, \l
    SliderStepsChange, or \l SliderValueChange. The default
    implementation only updates the display and ignores the \a change
    parameter.
 */
void AbstractSlider::sliderChange(SliderChange)
{
    update();
}
bool AbstractSliderPrivate::scrollByDelta(Qt::Orientation orientation,
                                          Qt::KeyboardModifiers modifiers, int delta)
{
    Q_Q(AbstractSlider);
    int stepsToScroll = 0;
    // in Qt scrolling to the right gives negative values.
    if (orientation == Qt::Horizontal)
        delta = -delta;
    qreal offset = qreal(delta) / 120;
    if ((modifiers & Qt::ControlModifier) || (modifiers & Qt::ShiftModifier)) {
        // Scroll one page regardless of delta:
        stepsToScroll = qBound(-pageStep, int(offset * pageStep), pageStep);
        offset_accumulated = 0;
    } else {
        // Calculate how many lines to scroll. Depending on what delta is (and
        // offset), we might end up with a fraction (e.g. scroll 1.3 lines). We can
        // only scroll whole lines, so we keep the reminder until next event.
        qreal stepsToScrollF =
#if QT_CONFIG(wheelevent)
            QApplication::wheelScrollLines() *
#endif
            offset * effectiveSingleStep();
        // Check if wheel changed direction since last event:
        if (offset_accumulated != 0 && (offset / offset_accumulated) < 0)
            offset_accumulated = 0;
        offset_accumulated += stepsToScrollF;
        // Don't scroll more than one page in any case:
        stepsToScroll = qBound(-pageStep, int(offset_accumulated), pageStep);
        offset_accumulated -= int(offset_accumulated);
        if (stepsToScroll == 0) {
            // We moved less than a line, but might still have accumulated partial scroll,
            // unless we already are at one of the ends.
            const float effective_offset
                = invertedControls ? -offset_accumulated : offset_accumulated;
            if (effective_offset > 0.f && value < maximum)
                return true;
            if (effective_offset < 0.f && value > minimum)
                return true;
            offset_accumulated = 0;
            return false;
        }
    }
    if (invertedControls)
        stepsToScroll = -stepsToScroll;
    int prevValue = value;
    position = bound(overflowSafeAdd(stepsToScroll)); // value will be updated by triggerAction()
    q->triggerAction(AbstractSlider::SliderMove);
    if (prevValue == value) {
        offset_accumulated = 0;
        return false;
    }
    return true;
}
/*!
    \reimp
*/
#if QT_CONFIG(wheelevent)
void AbstractSlider::wheelEvent(QWheelEvent* e)
{
    Q_D(AbstractSlider);
    e->ignore();
    bool vertical = bool(e->angleDelta().y());
    int delta = vertical ? e->angleDelta().y() : e->angleDelta().x();
    if (e->inverted())
        delta = -delta;
    if (d->scrollByDelta(vertical ? Qt::Vertical : Qt::Horizontal, e->modifiers(), delta))
        e->accept();
}
#endif
/*!
    \reimp
*/
void AbstractSlider::keyPressEvent(QKeyEvent* ev)
{
    Q_D(AbstractSlider);
    SliderAction action = SliderNoAction;
#ifdef QT_KEYPAD_NAVIGATION
    if (ev->isAutoRepeat()) {
        if (!d->firstRepeat.isValid())
            d->firstRepeat.start();
        else if (1 == d->repeatMultiplier) {
            // This is the interval in milli seconds which one key repetition
            // takes.
            const int repeatMSecs = d->firstRepeat.elapsed();
            /**
             * The time it takes to currently navigate the whole slider.
             */
            const qreal currentTimeElapse = (qreal(maximum()) / singleStep()) * repeatMSecs;
            /**
             * This is an arbitrarily determined constant in msecs that
             * specifies how long time it should take to navigate from the
             * start to the end(excluding starting key auto repeat).
             */
            const int SliderRepeatElapse = 2500;
            d->repeatMultiplier = currentTimeElapse / SliderRepeatElapse;
        }
    } else if (d->firstRepeat.isValid()) {
        d->firstRepeat.invalidate();
        d->repeatMultiplier = 1;
    }
#endif
    switch (ev->key()) {
#ifdef QT_KEYPAD_NAVIGATION
    case Qt::Key_Select:
        if (QApplicationPrivate::keypadNavigationEnabled())
            setEditFocus(!hasEditFocus());
        else
            ev->ignore();
        break;
    case Qt::Key_Back:
        if (QApplicationPrivate::keypadNavigationEnabled() && hasEditFocus()) {
            setValue(d->origValue);
            setEditFocus(false);
        } else
            ev->ignore();
        break;
#endif
    case Qt::Key_Left:
#ifdef QT_KEYPAD_NAVIGATION
        // In QApplication::KeypadNavigationDirectional, we want to change the slider
        // value if there is no left/right navigation possible and if this slider is not
        // inside a tab widget.
        if (QApplicationPrivate::keypadNavigationEnabled()
            && (!hasEditFocus()
                    && QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
                || d->orientation == Qt::Vertical
                || !hasEditFocus()
                    && (QWidgetPrivate::canKeypadNavigate(Qt::Horizontal)
                        || QWidgetPrivate::inTabWidget(this)))) {
            ev->ignore();
            return;
        }
        if (QApplicationPrivate::keypadNavigationEnabled() && d->orientation == Qt::Vertical)
            action = d->invertedControls ? SliderSingleStepSub : SliderSingleStepAdd;
        else
#endif
            if (isRightToLeft())
            action = d->invertedControls ? SliderSingleStepSub : SliderSingleStepAdd;
        else
            action = !d->invertedControls ? SliderSingleStepSub : SliderSingleStepAdd;
        break;
    case Qt::Key_Right:
#ifdef QT_KEYPAD_NAVIGATION
        // Same logic as in Qt::Key_Left
        if (QApplicationPrivate::keypadNavigationEnabled()
            && (!hasEditFocus()
                    && QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
                || d->orientation == Qt::Vertical
                || !hasEditFocus()
                    && (QWidgetPrivate::canKeypadNavigate(Qt::Horizontal)
                        || QWidgetPrivate::inTabWidget(this)))) {
            ev->ignore();
            return;
        }
        if (QApplicationPrivate::keypadNavigationEnabled() && d->orientation == Qt::Vertical)
            action = d->invertedControls ? SliderSingleStepAdd : SliderSingleStepSub;
        else
#endif
            if (isRightToLeft())
            action = d->invertedControls ? SliderSingleStepAdd : SliderSingleStepSub;
        else
            action = !d->invertedControls ? SliderSingleStepAdd : SliderSingleStepSub;
        break;
    case Qt::Key_Up:
#ifdef QT_KEYPAD_NAVIGATION
        // In QApplication::KeypadNavigationDirectional, we want to change the slider
        // value if there is no up/down navigation possible.
        if (QApplicationPrivate::keypadNavigationEnabled()
            && (QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
                || d->orientation == Qt::Horizontal
                || !hasEditFocus() && QWidgetPrivate::canKeypadNavigate(Qt::Vertical))) {
            ev->ignore();
            break;
        }
#endif
        action = d->invertedControls ? SliderSingleStepSub : SliderSingleStepAdd;
        break;
    case Qt::Key_Down:
#ifdef QT_KEYPAD_NAVIGATION
        // Same logic as in Qt::Key_Up
        if (QApplicationPrivate::keypadNavigationEnabled()
            && (QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
                || d->orientation == Qt::Horizontal
                || !hasEditFocus() && QWidgetPrivate::canKeypadNavigate(Qt::Vertical))) {
            ev->ignore();
            break;
        }
#endif
        action = d->invertedControls ? SliderSingleStepAdd : SliderSingleStepSub;
        break;
    case Qt::Key_PageUp:
        action = d->invertedControls ? SliderPageStepSub : SliderPageStepAdd;
        break;
    case Qt::Key_PageDown:
        action = d->invertedControls ? SliderPageStepAdd : SliderPageStepSub;
        break;
    case Qt::Key_Home:
        action = SliderToMinimum;
        break;
    case Qt::Key_End:
        action = SliderToMaximum;
        break;
    default:
        ev->ignore();
        break;
    }
    if (action)
        triggerAction(action);
}
/*!
    \reimp
*/
void AbstractSlider::changeEvent(QEvent* ev)
{
    Q_D(AbstractSlider);
    switch (ev->type()) {
    case QEvent::EnabledChange:
        if (!isEnabled()) {
            d->repeatActionTimer.stop();
            setSliderDown(false);
        }
        Q_FALLTHROUGH();
    default:
        QWidget::changeEvent(ev);
    }
}
/*!
    \reimp
*/
bool AbstractSlider::event(QEvent* e)
{
#ifdef QT_KEYPAD_NAVIGATION
    Q_D(AbstractSlider);
    switch (e->type()) {
    case QEvent::FocusIn:
        d->origValue = d->value;
        break;
    default:
        break;
    }
#endif
    return QWidget::event(e);
}
// This function is called from itemviews when doing scroll per pixel (on updateGeometries())
// It will not have any effect if there has been a call to setSingleStep with
// a 'reasonable' value (since viewMayChangeSingleStep will be set to false).
// (If setSingleStep is called with -1 it will however allow the views to change singleStep.)
void AbstractSliderPrivate::itemviewChangeSingleStep(int step)
{
    singleStepFromItemView = step;
    if (viewMayChangeSingleStep && singleStep != step)
        setSteps(step, pageStep);
}
