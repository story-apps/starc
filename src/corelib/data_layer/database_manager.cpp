#include "database_manager.h"

#include "database.h"
#include "database_worker.h"

#include <QMutex>
#include <QQueue>
#include <QSqlQuery>
#include <QThread>


namespace ManagementLayer {


class DatabaseManager::Implementation
{
public:
    explicit Implementation();
    ~Implementation();

    void executeNextQuery();

    DatabaseLayer::DatabaseWorker* worker = nullptr;
    QThread thread;
    QQueue<QPair<QString, QVariantList>> queryQueue;
    QMutex requestQueueMutex;
    bool isProcessing = false;
};

DatabaseManager::Implementation::Implementation()
{
}

DatabaseManager::Implementation::~Implementation()
{
    thread.quit();
    thread.wait();
}


// ****


DatabaseManager::~DatabaseManager() = default;

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

void DatabaseManager::enqueueQuery(const QString& _query, const QVariantList& _bindValues)
{
    QMutexLocker locker(&d->requestQueueMutex);
    d->queryQueue.enqueue({ _query, _bindValues });

    if (!d->isProcessing) {
        d->isProcessing = true;
        emit executeQuery(d->queryQueue.head().first, d->queryQueue.head().second);
    }
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
    , d(new Implementation)
{
    if (!d->worker) {
        d->worker = new DatabaseLayer::DatabaseWorker(this);
        d->worker->moveToThread(&d->thread);

        connect(this, &DatabaseManager::executeQuery, d->worker,
                &DatabaseLayer::DatabaseWorker::executeQuery, Qt::QueuedConnection);
        connect(d->worker, &DatabaseLayer::DatabaseWorker::queryExecuted, this,
                &DatabaseManager::onQueryExecuted, Qt::QueuedConnection);
        connect(d->worker, &DatabaseLayer::DatabaseWorker::queryFailed, this,
                &DatabaseManager::onQueryFailed, Qt::QueuedConnection);
        d->thread.start();
    }
}

void DatabaseManager::onQueryExecuted(const QVector<QVariantList>& _results)
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
        emit executeQuery(d->queryQueue.head().first, d->queryQueue.head().second);
    } else {
        d->isProcessing = false;
    }
}

void DatabaseManager::onQueryFailed(const QString& _error)
{
    emit queryFailed(_error);
}


} // namespace ManagementLayer
