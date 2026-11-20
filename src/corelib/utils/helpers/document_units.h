#pragma once

#include <QMarginsF>
#include <QSizeF>
#include <QtGlobal>

// Document coordinates are logical units at 96 DPI, not physical screen pixels.
namespace DocumentUnits {
constexpr qreal Dpi = 96.0;
constexpr qreal MmPerInch = 25.4;
constexpr qreal PtPerInch = 72.0;

constexpr qreal mmToDu(qreal mm) { return mm * Dpi / MmPerInch; }
constexpr qreal duToMm(qreal du) { return du * MmPerInch / Dpi; }
constexpr qreal ptToDu(qreal pt) { return pt * Dpi / PtPerInch; }
constexpr qreal duToPt(qreal du) { return du * PtPerInch / Dpi; }

inline QSizeF mmToDu(const QSizeF& size)
{
    return { mmToDu(size.width()), mmToDu(size.height()) };
}

inline QMarginsF mmToDu(const QMarginsF& margins)
{
    return { mmToDu(margins.left()), mmToDu(margins.top()),
             mmToDu(margins.right()), mmToDu(margins.bottom()) };
}
} // namespace DocumentUnits
