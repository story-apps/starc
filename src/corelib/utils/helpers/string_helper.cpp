#include "string_helper.h"

#include <QHash>
#include <QMarginsF>
#include <QRectF>
#include <QString>

namespace {

const QHash<Qt::Alignment, QString> kAlignmentToString = {
    { Qt::AlignLeft, QLatin1String("left") },     { Qt::AlignHCenter, QLatin1String("hcenter") },
    { Qt::AlignRight, QLatin1String("right") },   { Qt::AlignJustify, QLatin1String("justify") },
    { Qt::AlignTop, QLatin1String("top") },       { Qt::AlignVCenter, QLatin1String("vcenter") },
    { Qt::AlignBottom, QLatin1String("bottom") },
};

const QHash<QPageSize::PageSizeId, QString> kPageSizeIdToString = {
    { QPageSize::A4, QLatin1String("A4") },
    { QPageSize::Letter, QLatin1String("Letter") },
};

} // namespace


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

QPageSize::PageSizeId pageSizeIdFromString(const QString& _pageSize)
{
    return kPageSizeIdToString.key(_pageSize, QPageSize::A4);
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

QString toString(QPageSize::PageSizeId _pageSize)
{
    return kPageSizeIdToString.value(_pageSize, "A4");
}
