#pragma once

#include "document_units.h"

#include <QPaintDevice>
#include <QPaintEngine>

// Metric-only device: never use this as a QPainter target.
class DocumentLayoutDevice final : public QPaintDevice
{
public:
    static QPaintDevice* device()
    {
        static DocumentLayoutDevice instance;
        return &instance;
    }

    QPaintEngine* paintEngine() const override { return nullptr; }

protected:
    int metric(PaintDeviceMetric metric) const override
    {
        switch (metric) {
        case PdmDpiX:
        case PdmDpiY:
        case PdmPhysicalDpiX:
        case PdmPhysicalDpiY:
            return static_cast<int>(DocumentUnits::Dpi);
        case PdmDevicePixelRatio:
            return 1;
        case PdmWidth:
        case PdmHeight:
            return 1;
        case PdmWidthMM:
        case PdmHeightMM:
            return 1;
        case PdmDepth:
            return 32;
        default:
            return 0;
        }
    }

private:
    DocumentLayoutDevice() = default;
};
