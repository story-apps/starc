#pragma once

#include "abstract_slider_p.h"
#include "page_text_edit_scroll_bar.h"
#include "qstyle.h"

#include <QtWidgets/private/qtwidgetsglobal_p.h>


class PageTextEditScrollBarPrivate : public AbstractSliderPrivate
{
    Q_DECLARE_PUBLIC(PageTextEditScrollBar)
public:
    QStyle::SubControl pressedControl;
    bool pointerOutsidePressedControl;
    int clickOffset, snapBackPosition;
    void activateControl(uint control, int threshold = 500);
    void stopRepeatAction();
    int pixelPosToRangeValue(int pos) const;
    void init();
    bool updateHoverControl(const QPoint& pos);
    QStyle::SubControl newHoverControl(const QPoint& pos);
    QStyle::SubControl hoverControl;
    QRect hoverRect;
    bool transient;
    void setTransient(bool value);
    bool flashed;
    int flashTimer;
    void flash();
};
