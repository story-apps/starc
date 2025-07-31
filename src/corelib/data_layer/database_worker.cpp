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
    /**
     * @brief Выполнить запрос
     */
    bool execute(const Query& _query);

    /**
     * @brief Список выполненных запросов и результатов их выполнения
     */
    QVector<QPair<QUuid, QVector<QSqlRecord>>> queryUuidsToResults;
};

bool DatabaseWorker::Implementation::execute(const Query& _query)
{
    auto query = Database::query(s_connectionName);
    query.prepare(_query.queryString);
    for (const QVariant& value : std::as_const(_query.bindValues)) {
        query.addBindValue(value);
    }

    if (!query.exec()) {
        DatabaseLayer::Database::setLastError(query.lastError().text());
        qDebug() << query.lastError();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        return false;
    }

    QVector<QSqlRecord> results;
    while (query.next()) {
        results.append(query.record());
    }

    queryUuidsToResults.append({ _query.uuid, results });
    return true;
}


// ****


DatabaseWorker::DatabaseWorker(QObject* parent)
    : QObject{ parent }
    , d(new Implementation)
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
    d->queryUuidsToResults.clear();

    if (_queries.isEmpty()) {
        return;
    }
    //
    // Одиночный запрос исполняем сразу
    //
    else if (_queries.size() == 1) {
        if (d->execute(_queries.first())) {
            const auto queryUuid = d->queryUuidsToResults.first().first;
            const auto queryResult = d->queryUuidsToResults.first().second;
            emit queryExecuted(queryUuid, queryResult);
            emit transactionExecuted();
        }
    }
    //
    // Если пришло несколько запросов, открываем транзакцию
    //
    else {
        if (Database::transaction(s_connectionName)) {
            auto isExecuted = true;
            //
            // Выполняем запросы
            //
            for (const auto& query : _queries) {
                isExecuted = d->execute(query);
                if (!isExecuted) {
                    break;
                }
            }
            //
            // Если все ок, коммитим и отправляем сигналы об исполнении каждого запроса и транзакции
            //
            if (isExecuted) {
                Database::commit(s_connectionName);
                for (const auto& uuidToResult : d->queryUuidsToResults) {
                    const auto queryUuid = uuidToResult.first;
                    const auto queryResult = uuidToResult.second;
                    emit queryExecuted(queryUuid, queryResult);
                }
                emit transactionExecuted();
            }
            //
            // Если какой-то запрос не был выполнен, откатываем и отправляем ошибку для каждого
            // запроса и для транзакции
            //
            else {
                Database::rollback(s_connectionName);
                const QString error(tr("Transaction was not executed"));
                for (const auto& query : _queries) {
                    emit queryFailed(query.uuid, error);
                }
                emit transactionFailed();
            }
        }
        //
        // Если транзакция не была открыта, отправляем ошибку для каждого запроса и для транзакции
        //
        else {
            const QString error(tr("Transaction was not opened"));
            for (const auto& query : _queries) {
                emit queryFailed(query.uuid, error);
            }
            emit transactionFailed();
        }
    }
}

} // namespace DatabaseLayer
