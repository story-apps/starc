#include "temp_images_manager.h"

#include "temp_images_worker.h"

#include <QMutex>
#include <QQueue>
#include <QThread>


namespace ManagementLayer {

class TempImagesManager::Implementation
{
public:
    explicit Implementation() = default;
    ~Implementation();

    TempImagesLayer::TempImagesWorker* worker = nullptr;
    QThread thread;
};

TempImagesManager::Implementation::~Implementation()
{
    thread.quit();
    thread.wait();
}


// ****


TempImagesManager::TempImagesManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    d->worker = new TempImagesLayer::TempImagesWorker();
    d->worker->moveToThread(&d->thread);

    connect(this, &TempImagesManager::requestedToStore, d->worker,
            &TempImagesLayer::TempImagesWorker::storeToTempFiles, Qt::QueuedConnection);
    connect(d->worker, &TempImagesLayer::TempImagesWorker::filesStored, this,
            &TempImagesManager::filesStored, Qt::QueuedConnection);

    connect(this, &TempImagesManager::requestedToLoad, d->worker,
            &TempImagesLayer::TempImagesWorker::loadFromTempFiles, Qt::QueuedConnection);
    connect(d->worker, &TempImagesLayer::TempImagesWorker::imagesLoaded, this,
            &TempImagesManager::imagesLoaded, Qt::QueuedConnection);

    connect(&d->thread, &QThread::finished, d->worker, &QObject::deleteLater);

    d->thread.start();
}

TempImagesManager::~TempImagesManager() = default;

QUuid TempImagesManager::storeAsync(const QByteArray& _zipArchive)
{
    const auto queryUuid = QUuid::createUuid();
    emit requestedToStore(queryUuid, _zipArchive);
    return queryUuid;
}

QUuid TempImagesManager::loadAsync(const QVector<TempImagesLayer::TempImageFile>& _tempFiles)
{
    const auto queryUuid = QUuid::createUuid();
    emit requestedToLoad(queryUuid, _tempFiles);
    return queryUuid;
}

} // namespace ManagementLayer
