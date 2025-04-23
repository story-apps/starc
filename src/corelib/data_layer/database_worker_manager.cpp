#include "database_worker_manager.h"

#include "database_worker.h"

#include <QMutex>
#include <QQueue>
#include <QSqlQuery>
#include <QThread>


namespace ManagementLayer {


class DatabaseWorkerManager::Implementation
{
public:
    explicit Implementation();
    ~Implementation();

    void executeNextQuery();

    DatabaseLayer::DatabaseWorker* worker = nullptr;
    QThread thread;
    QQueue<QString> queryQueue;
    QMutex requestQueueMutex;
    bool isProcessing = false;
};

DatabaseWorkerManager::Implementation::Implementation()
{
}

DatabaseWorkerManager::Implementation::~Implementation()
{
    thread.quit();
    thread.wait();
}


// ****


DatabaseWorkerManager::~DatabaseWorkerManager() = default;

DatabaseWorkerManager& DatabaseWorkerManager::instance()
{
    static DatabaseWorkerManager instance;
    return instance;
}

void DatabaseWorkerManager::enqueueQuery(const QString& _sqlQuery)
{
    QMutexLocker locker(&d->requestQueueMutex);
    d->queryQueue.enqueue(_sqlQuery);

    if (!d->isProcessing) {
        d->isProcessing = true;
        emit executeQuery(d->queryQueue.head());
    }
}

DatabaseWorkerManager::DatabaseWorkerManager(QObject* _parent)
    : QObject{ _parent }
    , d(new Implementation)
{
    if (!d->worker) {
        d->worker = new DatabaseLayer::DatabaseWorker;
        d->worker->moveToThread(&d->thread);

        connect(this, &DatabaseWorkerManager::executeQuery, d->worker,
                &DatabaseLayer::DatabaseWorker::executeQuery, Qt::QueuedConnection);
        connect(d->worker, &DatabaseLayer::DatabaseWorker::queryExecuted, this,
                &DatabaseWorkerManager::onQueryExecuted, Qt::QueuedConnection);
        connect(d->worker, &DatabaseLayer::DatabaseWorker::queryFailed, this,
                &DatabaseWorkerManager::onQueryFailed, Qt::QueuedConnection);
        d->thread.start();
    }
}

void DatabaseWorkerManager::onQueryExecuted(const QVector<QVariantList>& _results)
{
    emit queryResult(_results);

    QMutexLocker locker(&d->requestQueueMutex);
    //
    // Удаляем текущий запрос
    //
    if (!d->queryQueue.isEmpty()) {
        d->queryQueue.dequeue();
    }
    //
    // Отправляем на исполнение следующий или помечаем, что обработка завершена
    //
    if (!d->queryQueue.isEmpty()) {
        emit executeQuery(d->queryQueue.head());
    } else {
        d->isProcessing = false;
    }
}

void DatabaseWorkerManager::onQueryFailed(const QString& _error)
{
    emit queryFailed(_error);
}


} // namespace ManagementLayer
