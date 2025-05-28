#include "database_manager.h"

#include "database.h"
#include "database_worker.h"

#include <QMutex>
#include <QQueue>
#include <QSqlQuery>
#include <QThread>
#include <QUuid>


namespace DatabaseLayer {


class DatabaseManager::Implementation
{
public:
    explicit Implementation(DatabaseManager* _q);
    ~Implementation();

    void executeNextQuery();


    DatabaseManager* q = nullptr;

    DatabaseLayer::DatabaseWorker* worker = nullptr;
    QThread thread;
    struct QueryInQueue {
        QUuid uuid;
        QString query;
        QVariantList bindValues;
    };
    QQueue<QueryInQueue> queryQueue;
    QMutex queryQueueMutex;
    bool isProcessing = false;
};

DatabaseManager::Implementation::Implementation(DatabaseManager* _q)
    : q(_q)
{
    worker = new DatabaseLayer::DatabaseWorker();
    worker->moveToThread(&thread);

    connect(q, &DatabaseManager::queryRequested, worker,
            &DatabaseLayer::DatabaseWorker::executeQuery, Qt::QueuedConnection);
    connect(
        worker, &DatabaseLayer::DatabaseWorker::queryExecuted, q,
        [this](const QUuid& _queryUuid, const QVector<QSqlRecord>& _results) {
            QMutexLocker locker(&queryQueueMutex);
            //
            // Удаляем текущий запрос
            //
            if (!queryQueue.isEmpty()) {
                queryQueue.dequeue();
            }

            //
            // Уведомляем о том, что он исполнен
            //
            emit q->queryFinished(_queryUuid, _results);

            //
            // Отправляем на исполнение следующий запрос или помечаем, что обработка завершена
            //
            if (!queryQueue.isEmpty()) {
                emit q->queryRequested(queryQueue.head().uuid, queryQueue.head().query,
                                       queryQueue.head().bindValues);
            } else {
                isProcessing = false;
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &DatabaseLayer::DatabaseWorker::queryFailed, q,
        [this](const QUuid& _queryUuid, const QString& _error) {
            QMutexLocker locker(&queryQueueMutex);
            //
            // Удаляем текущий запрос
            //
            if (!queryQueue.isEmpty()) {
                queryQueue.dequeue();
            }

            //
            // Уведомляем об ошибке
            //
            emit q->queryFailed(_queryUuid, _error);

            //
            // Отправляем на исполнение следующий запрос или помечаем, что обработка завершена
            //
            if (!queryQueue.isEmpty()) {
                emit q->queryRequested(queryQueue.head().uuid, queryQueue.head().query,
                                       queryQueue.head().bindValues);
            } else {
                isProcessing = false;
            }
        },
        Qt::QueuedConnection);
    connect(&thread, &QThread::finished, worker, &QObject::deleteLater);
}

DatabaseManager::Implementation::~Implementation()
{
    if (thread.isRunning()) {
        thread.quit();
        thread.wait();
    }
}


// ****


DatabaseManager::~DatabaseManager() = default;

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

QUuid DatabaseManager::enqueueQuery(const QString& _query, const QVariantList& _bindValues)
{
    if (!d->thread.isRunning()) {
        d->thread.start();
    }

    QMutexLocker locker(&d->queryQueueMutex);

    QUuid queryUuid = QUuid::createUuid();
    d->queryQueue.enqueue({ queryUuid, _query, _bindValues });

    if (!d->isProcessing) {
        d->isProcessing = true;
        emit queryRequested(d->queryQueue.head().uuid, d->queryQueue.head().query,
                            d->queryQueue.head().bindValues);
    }

    return queryUuid;
}

bool DatabaseManager::canOpenFile(const QString& _databaseFileName)
{
    return DatabaseLayer::Database::canOpenFile(_databaseFileName);
}

QString DatabaseManager::openFileError()
{
    return DatabaseLayer::Database::openFileError();
}

bool DatabaseManager::hasError()
{
    return DatabaseLayer::Database::hasError();
}

QString DatabaseManager::lastError()
{
    return DatabaseLayer::Database::lastError();
}

void DatabaseManager::setLastError(const QString& _error)
{
    DatabaseLayer::Database::setLastError(_error);
}

void DatabaseManager::setCurrentFile(const QString& _databaseFileName)
{
    DatabaseLayer::Database::setCurrentFile(_databaseFileName);
}

void DatabaseManager::closeCurrentFile()
{
    DatabaseLayer::Database::closeCurrentFile();
}

QString DatabaseManager::currentFile()
{
    return DatabaseLayer::Database::currentFile();
}

QSqlQuery DatabaseManager::query()
{
    return DatabaseLayer::Database::query();
}

void DatabaseManager::transaction()
{
    DatabaseLayer::Database::transaction();
}

void DatabaseManager::commit()
{
    DatabaseLayer::Database::commit();
}

void DatabaseManager::vacuum()
{
    DatabaseLayer::Database::vacuum();
}

DatabaseManager::DatabaseManager(QObject* _parent)
    : QObject{ _parent }
    , d(new Implementation(this))
{
}

} // namespace DatabaseLayer
