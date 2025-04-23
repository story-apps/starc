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

    void nextTransaction();


    DatabaseManager* q = nullptr;

    DatabaseWorker* worker = nullptr;
    QThread thread;
    QQueue<QVector<Query>> queries;
    QVector<Query> transactionBuffer;

    bool isProcessing = false;
    bool isTransaction = false;
};

void DatabaseManager::Implementation::nextTransaction()
{
    //
    // Удаляем текущий запрос из очереди
    //
    Q_ASSERT(!queries.isEmpty());
    if (!queries.isEmpty()) {
        queries.dequeue();
    }
    //
    // Отправляем на исполнение следующий запрос или помечаем, что обработка завершена
    //
    if (!queries.isEmpty()) {
        emit q->queriesRequested(queries.head());
    } else {
        isProcessing = false;
    }
}

DatabaseManager::Implementation::Implementation(DatabaseManager* _q)
    : q(_q)
{
    worker = new DatabaseWorker();
    worker->moveToThread(&thread);

    connect(q, &DatabaseManager::queriesRequested, worker, &DatabaseWorker::execute,
            Qt::QueuedConnection);
    //
    connect(worker, &DatabaseWorker::queryExecuted, q, &DatabaseManager::queryFinished,
            Qt::QueuedConnection);
    connect(worker, &DatabaseWorker::queryFailed, q, &DatabaseManager::queryFailed,
            Qt::QueuedConnection);
    connect(
        worker, &DatabaseWorker::transactionExecuted, q, [this]() { nextTransaction(); },
        Qt::QueuedConnection);
    connect(
        worker, &DatabaseWorker::transactionFailed, q, [this]() { nextTransaction(); },
        Qt::QueuedConnection);
    //
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

QUuid DatabaseManager::executeQuery(const QString& _query, const QVariantList& _bindValues)
{

    const QUuid queryUuid = QUuid::createUuid();
    const Query query{ queryUuid, _query, _bindValues };

    if (d->isTransaction) {
        d->transactionBuffer.append(query);
        return queryUuid;
    }

    d->queries.enqueue({ query });

    if (!d->isProcessing) {
        if (!d->thread.isRunning()) {
            d->thread.start();
        }
        d->isProcessing = true;
        emit queriesRequested(d->queries.head());
    }

    return queryUuid;
}

void DatabaseManager::beginTransaction()
{
    d->isTransaction = true;
}

void DatabaseManager::executeTransaction()
{
    if (!d->isTransaction) {
        return;
    }
    d->queries.enqueue(d->transactionBuffer);
    d->transactionBuffer.clear();

    if (!d->isProcessing) {
        if (!d->thread.isRunning()) {
            d->thread.start();
        }
        d->isProcessing = true;
        emit queriesRequested(d->queries.head());
    }

    d->isTransaction = false;
}

bool DatabaseManager::canOpenFile(const QString& _databaseFileName)
{
    return Database::canOpenFile(_databaseFileName);
}

QString DatabaseManager::openFileError()
{
    return Database::openFileError();
}

bool DatabaseManager::hasError()
{
    return Database::hasError();
}

QString DatabaseManager::lastError()
{
    return Database::lastError();
}

void DatabaseManager::setLastError(const QString& _error)
{
    Database::setLastError(_error);
}

void DatabaseManager::setCurrentFile(const QString& _databaseFileName)
{
    Database::setCurrentFile(_databaseFileName);
}

void DatabaseManager::closeCurrentFile()
{
    Database::closeCurrentFile();
}

QString DatabaseManager::currentFile()
{
    return Database::currentFile();
}

QSqlQuery DatabaseManager::query()
{
    return Database::query();
}

bool DatabaseManager::transaction()
{
    return Database::transaction();
}

void DatabaseManager::commit()
{
    Database::commit();
}

void DatabaseManager::rollback()
{
    Database::rollback();
}

void DatabaseManager::vacuum()
{
    Database::vacuum();
}

DatabaseManager::DatabaseManager(QObject* _parent)
    : QObject{ _parent }
    , d(new Implementation(this))
{
}

} // namespace DatabaseLayer
