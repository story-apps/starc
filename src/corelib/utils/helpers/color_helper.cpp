#include "color_helper.h"

#include <QColor>


QColor ColorHelper::transparent(const QColor& _color, qreal _alphaF)
{
    if (qFuzzyCompare(_alphaF, 1.0)) {
        return _color;
    }

    QColor transparentColor = _color;
    transparentColor.setAlphaF(_alphaF);
    return transparentColor;
}

QColor ColorHelper::colorBetween(const QColor& _lhs, const QColor& _rhs)
{
    auto mid = [](int _lhs, int _rhs) { return (_lhs + _rhs) / 2; };
    return QColor(mid(_lhs.red(), _rhs.red()), mid(_lhs.green(), _rhs.green()),
                  mid(_rhs.blue(), _lhs.blue()));
}

bool ColorHelper::isColorLight(const QColor& _color)
{
    return (_color.redF() + _color.greenF() + _color.blueF()) / 3 > 0.5;
}

QColor ColorHelper::contrasted(const QColor& _color)
{
    const auto value = isColorLight(_color) ? 0 : 255;
    return QColor(value, value, value);
}

QColor ColorHelper::inverted(const QColor& _color)
{
    return _color.rgb() ^ 0xffffff;
}

QColor ColorHelper::nearby(const QColor& _color)
{
    return isColorLight(_color) ? _color.darker(110) : _color.lighter(130);
}
