#include "string_helper.h"

#include <QHash>
#include <QMarginsF>
#include <QRectF>
#include <QString>

namespace {
const QHash<Qt::Alignment, QString> kAlignmentToString
    = { { Qt::AlignLeft, "left" },    { Qt::AlignHCenter, "hcenter" },
        { Qt::AlignRight, "right" },  { Qt::AlignJustify, "justify" },
        { Qt::AlignTop, "top" },      { Qt::AlignVCenter, "vcenter" },
        { Qt::AlignBottom, "bottom" } };
}


Qt::Alignment alignmentFromString(const QString& _alignment)
{
    Qt::Alignment result;
    const auto parts = _alignment.split(",", Qt::SkipEmptyParts);
    for (const auto& align : parts) {
        result |= kAlignmentToString.key(align);
    }
    return result;
}

QMarginsF marginsFromString(const QString& _margins)
{
    QStringList margins = _margins.split(",");
    return QMarginsF(
        margins.value(0, 0).simplified().toDouble(), margins.value(1, 0).simplified().toDouble(),
        margins.value(2, 0).simplified().toDouble(), margins.value(3, 0).simplified().toDouble());
}

QRectF rectFromString(const QString& _rect)
{
    QStringList rect = _rect.split(",");
    return QRectF(
        rect.value(0, 0).simplified().toDouble(), rect.value(1, 0).simplified().toDouble(),
        rect.value(2, 0).simplified().toDouble(), rect.value(3, 0).simplified().toDouble());
}

QString toString(bool _value)
{
    return _value ? "true" : "false";
}

QString toString(int _value)
{
    return QString::number(_value);
}

QString toString(qreal _value)
{
    return QString::number(_value);
}

QString toString(Qt::Alignment _alignment)
{
    auto result = kAlignmentToString.value(_alignment & Qt::AlignHorizontal_Mask);
    const auto vAlign = kAlignmentToString.value(_alignment & Qt::AlignVertical_Mask);
    if (!vAlign.isEmpty()) {
        if (!result.isEmpty()) {
            result.append(",");
        }
        result += vAlign;
    }
    return result;
}

QString toString(const QMarginsF& _margins)
{
    return QString("%1,%2,%3,%4")
        .arg(_margins.left())
        .arg(_margins.top())
        .arg(_margins.right())
        .arg(_margins.bottom());
}

QString toString(const QRectF& _rect)
{
    return QString("%1,%2,%3,%4")
        .arg(_rect.left())
        .arg(_rect.top())
        .arg(_rect.width())
        .arg(_rect.height());
}
