#pragma once

#include <QtWidgets/qwidget.h>

#include <corelib_global.h>


class AbstractSliderPrivate;
class CORE_LIBRARY_EXPORT AbstractSlider : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(int singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(int pageStep READ pageStep WRITE setPageStep)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(int sliderPosition READ sliderPosition WRITE setSliderPosition NOTIFY sliderMoved)
    Q_PROPERTY(bool tracking READ hasTracking WRITE setTracking)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
    Q_PROPERTY(bool invertedAppearance READ invertedAppearance WRITE setInvertedAppearance)
    Q_PROPERTY(bool invertedControls READ invertedControls WRITE setInvertedControls)
    Q_PROPERTY(bool sliderDown READ isSliderDown WRITE setSliderDown DESIGNABLE false)
public:
    explicit AbstractSlider(QWidget* parent = nullptr);
    ~AbstractSlider();
    Qt::Orientation orientation() const;
    void setMinimum(int);
    int minimum() const;
    void setMaximum(int);
    int maximum() const;
    void setSingleStep(int);
    int singleStep() const;
    void setPageStep(int);
    int pageStep() const;
    void setTracking(bool enable);
    bool hasTracking() const;
    void setSliderDown(bool);
    bool isSliderDown() const;
    void setSliderPosition(int);
    int sliderPosition() const;
    void setInvertedAppearance(bool);
    bool invertedAppearance() const;
    void setInvertedControls(bool);
    bool invertedControls() const;
    enum SliderAction {
        SliderNoAction,
        SliderSingleStepAdd,
        SliderSingleStepSub,
        SliderPageStepAdd,
        SliderPageStepSub,
        SliderToMinimum,
        SliderToMaximum,
        SliderMove
    };
    int value() const;
    void triggerAction(SliderAction action);
public Q_SLOTS:
    void setValue(int);
    void setOrientation(Qt::Orientation);
    void setRange(int min, int max);
Q_SIGNALS:
    void valueChanged(int value);
    void sliderPressed();
    void sliderMoved(int position);
    void sliderReleased();
    void rangeChanged(int min, int max);
    void actionTriggered(int action);

protected:
    bool event(QEvent* e) override;
    void setRepeatAction(SliderAction action, int thresholdTime = 500, int repeatTime = 50);
    SliderAction repeatAction() const;
    enum SliderChange {
        SliderRangeChange,
        SliderOrientationChange,
        SliderStepsChange,
        SliderValueChange
    };
    virtual void sliderChange(SliderChange change);
    void keyPressEvent(QKeyEvent* ev) override;
    void timerEvent(QTimerEvent*) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent* e) override;
#endif
    void changeEvent(QEvent* e) override;

protected:
    AbstractSlider(AbstractSliderPrivate& dd, QWidget* parent = nullptr);

private:
    Q_DISABLE_COPY(AbstractSlider)
    Q_DECLARE_PRIVATE(AbstractSlider)
};
