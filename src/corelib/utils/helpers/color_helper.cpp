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
    if ((_color.hue() < 20 || _color.hue() > 220) && _color.saturationF() > 0.7) {
        return false;
    } else {
        return _color.valueF() > 0.55;
    }
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

QColor ColorHelper::nearby(const QColor& _color, int _f)
{
    return isColorLight(_color) ? _color.darker(_f) : _color.lighter(_f + 20);
}

QString ColorHelper::toString(const QColor& _color)
{
    if (!_color.isValid()) {
        return {};
    }

    return _color.name();
}

QColor ColorHelper::fromString(const QString& _colorName)
{
    if (_colorName.isEmpty()) {
        return {};
    }

    return QColor(_colorName);
}

QColor ColorHelper::forText(const QString& _text)
{
    ushort hash = 0;
    for (int characterIndex = 0; characterIndex < _text.length(); ++characterIndex) {
        hash += _text.at(characterIndex).unicode() + ((hash << 5) - hash);
    }
    hash = hash % 360;
    return QColor::fromHsl(hash, 255 * 0.4, 255 * 0.6);
}

QColor ColorHelper::forNumber(int _number)
{
    const int kMax = 9;
    while (_number > kMax) {
        _number = _number % kMax;
    }
    switch (_number) {
    default:
    case 0:
        return "#48C6FC";
    case 1:
        return "#FF4081";
    case 2:
        return "#BD00FF";
    case 3:
        return "#FEC62E";
    case 4:
        return "#5D34FF";
    case 5:
        return "#08D070";
    case 6:
        return "#0038FF";
    case 7:
        return "#FF9036";
    case 8:
        return "#A6DC0D";
    case 9:
        return "#FE3235";
    }
}
