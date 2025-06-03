#pragma once

#include <QSharedPointer>
#include <QSize>
#include <QTemporaryFile>
#include <QUuid>

namespace TempImagesLayer {

struct TempImageFile {
    QSharedPointer<QTemporaryFile> tempFile;
    QSize imageSize;
    QUuid uuid;
};

} // namespace TempImagesLayer

Q_DECLARE_METATYPE(TempImagesLayer::TempImageFile)
