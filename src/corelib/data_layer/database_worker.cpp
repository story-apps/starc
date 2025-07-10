#include "database_worker.h"

#include <data_layer/database.h>

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

namespace DatabaseLayer {

namespace {
/**
 * @brief Название соединения с базой данных
 */
static QString s_connectionName = "thread_local_database";
} // namespace


class DatabaseWorker::Implementation
{
public:
    explicit Implementation(DatabaseWorker* _q);

    void execute(const Query& _query);


    DatabaseWorker* q = nullptr;
};

DatabaseWorker::Implementation::Implementation(DatabaseWorker* _q)
    : q(_q)
{
}

void DatabaseWorker::Implementation::execute(const Query& _query)
{
    QSqlQuery query = Database::query(s_connectionName);
    query.prepare(_query.queryString);
    for (const QVariant& value : std::as_const(_query.bindValues)) {
        query.addBindValue(value);
    }

    if (!query.exec()) {
        DatabaseLayer::Database::setLastError(query.lastError().text());
        qDebug() << query.lastError();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();

        emit q->queryFailed(_query.uuid, query.lastError().text());
        return;
    }

    QVector<QSqlRecord> results;
    while (query.next()) {
        results.append(query.record());
    }

    emit q->queryExecuted(_query.uuid, results);
}


// ****


DatabaseWorker::DatabaseWorker(QObject* parent)
    : QObject{ parent }
    , d(new Implementation(this))
{
    if (QMetaType::type("QVector<QSqlRecord>") == QMetaType::UnknownType) {
        qRegisterMetaType<QVector<QSqlRecord>>("QVector<QSqlRecord>");
    }
    if (QMetaType::type("QVector<DatabaseLayer::Query>") == QMetaType::UnknownType) {
        qRegisterMetaType<QVector<DatabaseLayer::Query>>("QVector<DatabaseLayer::Query>");
    }
}

DatabaseWorker::~DatabaseWorker() = default;

void DatabaseWorker::execute(const QVector<Query>& _queries)
{
    if (_queries.isEmpty()) {
        return;
    } else if (_queries.size() == 1) {
        d->execute(_queries.first());
        emit transactionExecuted();
    } else {
        if (Database::transaction(s_connectionName)) {
            for (const auto& query : _queries) {
                d->execute(query);
            }
            Database::commit(s_connectionName);
            emit transactionExecuted();
        } else {
            const QString error(tr("Transaction was not opened"));
            for (const auto& query : _queries) {
                emit queryFailed(query.uuid, error);
            }
            emit transactionFailed();
        }
    }
}

} // namespace DatabaseLayer
