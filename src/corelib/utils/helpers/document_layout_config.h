#pragma once

#include "document_layout_device.h"

#include <QAbstractTextDocumentLayout>
#include <QTextDocument>

namespace DocumentLayoutConfig {
inline void configure(QTextDocument* document)
{
    Q_ASSERT(document);
    document->setUseDesignMetrics(true);
    auto* layout = document->documentLayout();
    if (layout->paintDevice() != DocumentLayoutDevice::device()) {
        layout->setPaintDevice(DocumentLayoutDevice::device());
    }
}
} // namespace DocumentLayoutConfig
