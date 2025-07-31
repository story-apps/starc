#pragma once

#include "frame.h"
#include "private/qwidget_p.h"

#include <QtWidgets/private/qtwidgetsglobal_p.h>


class FramePrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(Frame)
public:
    FramePrivate();
    ~FramePrivate();

    void updateFrameWidth();
    void updateStyledFrameWidths();

    QRect frect;
    int frameStyle;
    short lineWidth;
    short midLineWidth;
    short frameWidth;
    short leftFrameWidth, rightFrameWidth;
    short topFrameWidth, bottomFrameWidth;

    inline void init();
};
