#include "application_style.h"


ApplicationStyle::ApplicationStyle(QStyle* _style)
    : QProxyStyle(_style)
{
}

QRect ApplicationStyle::subControlRect(QStyle::ComplexControl _complexControl,
    const QStyleOptionComplex* _option, QStyle::SubControl _subControl, const QWidget* _widget) const
{
    if (_complexControl == CC_ScrollBar
        && (_subControl == SC_ScrollBarAddLine || _subControl == SC_ScrollBarSubLine)) {
        return {};
    }

    return QProxyStyle::subControlRect(_complexControl, _option, _subControl, _widget);
}

QStyle::SubControl ApplicationStyle::hitTestComplexControl(QStyle::ComplexControl _control,
    const QStyleOptionComplex* _option, const QPoint& _pos, const QWidget* _widget) const
{
    const SubControl _subcontrol = QProxyStyle::hitTestComplexControl(_control, _option, _pos, _widget);
    if (_control == CC_ScrollBar
        && (_subcontrol == SC_ScrollBarAddLine || _subcontrol == SC_ScrollBarSubLine)) {
            return SC_ScrollBarSlider;
    }
    return _subcontrol;
}

int ApplicationStyle::pixelMetric(QStyle::PixelMetric _metric, const QStyleOption* _option,
    const QWidget* _widget) const
{
    if (_metric == PM_ScrollBarExtent) {
        return 0;
    }

    return QProxyStyle::pixelMetric(_metric, _option, _widget);
}
