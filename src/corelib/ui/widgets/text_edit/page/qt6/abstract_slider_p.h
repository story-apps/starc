#pragma once

#include "QtCore/qbasictimer.h"
#include "QtCore/qelapsedtimer.h"
#include "abstract_slider.h"
#include "private/qwidget_p.h"
#include "qstyle.h"

#include <QtWidgets/private/qtwidgetsglobal_p.h>


class AbstractSliderPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(AbstractSlider)
public:
    AbstractSliderPrivate();
    ~AbstractSliderPrivate();
    void setSteps(int single, int page);
    int minimum, maximum, pageStep, value, position, pressValue;
    /**
     * Call effectiveSingleStep() when changing the slider value.
     */
    int singleStep;
    int singleStepFromItemView; // If we have itemViews we track the views preferred singleStep
                                // value.
    bool viewMayChangeSingleStep;
    float offset_accumulated;
    uint tracking : 1;
    uint blocktracking : 1;
    uint pressed : 1;
    uint invertedAppearance : 1;
    uint invertedControls : 1;
    Qt::Orientation orientation;
    QBasicTimer repeatActionTimer;
    int repeatActionTime;
    AbstractSlider::SliderAction repeatAction;
#ifdef QT_KEYPAD_NAVIGATION
    int origValue;
    /**
     */
    bool isAutoRepeating;
    /**
     * When we're auto repeating, we multiply singleStep with this value to
     * get our effective step.
     */
    qreal repeatMultiplier;
    /**
     * The time of when the first auto repeating key press event occurs.
     */
    QElapsedTimer firstRepeat;
#endif
    inline int effectiveSingleStep() const
    {
        return singleStep
#ifdef QT_KEYPAD_NAVIGATION
            * repeatMultiplier
#endif
            ;
    }
    void itemviewChangeSingleStep(int step);
    virtual int bound(int val) const
    {
        return qMax(minimum, qMin(maximum, val));
    }
    inline int overflowSafeAdd(int add) const
    {
        int newValue = value + add;
        if (add > 0 && newValue < value)
            newValue = maximum;
        else if (add < 0 && newValue > value)
            newValue = minimum;
        return newValue;
    }
    inline void setAdjustedSliderPosition(int position)
    {
        Q_Q(AbstractSlider);
        if (q->style()->styleHint(QStyle::SH_Slider_StopMouseOverSlider, nullptr, q)) {
            if ((position > pressValue - 2 * pageStep) && (position < pressValue + 2 * pageStep)) {
                repeatAction = AbstractSlider::SliderNoAction;
                q->setSliderPosition(pressValue);
                return;
            }
        }
        q->triggerAction(repeatAction);
    }
    bool scrollByDelta(Qt::Orientation orientation, Qt::KeyboardModifiers modifiers, int delta);
};
