#include "color_helper.h"

#include <QColor>


QColor ColorHelper::colorBetween(const QColor& _lhs, const QColor& _rhs)
{
    auto mid = [] (int _lhs, int _rhs) {
        return (_lhs + _rhs) / 2;
    };
    return QColor(mid(_lhs.red(), _rhs.red()),
                  mid(_lhs.green(), _rhs.green()),
                  mid(_rhs.blue(), _lhs.blue()));
}
