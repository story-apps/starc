#include "database_worker.h"

#include <data_layer/database.h>

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

namespace DatabaseLayer {

namespace {
/**
 * @brief Название соединения с базой данных
 */
static QString s_connectionName = "thread_local_database";
} // namespace


DatabaseWorker::DatabaseWorker(QObject* parent)
    : QObject{ parent }
{
}

void DatabaseWorker::executeQuery(const QString& _query)
{
    QSqlQuery query = Database::query(s_connectionName);
    query.prepare(_query);
    if (!query.exec()) {
        emit queryFailed(query.lastError().text());
        return;
    }
    QVector<QVariantList> results;
    while (query.next()) {
        QVariantList row;
        for (int i = 0; i < query.record().count(); ++i) {
            row.append(query.value(i));
        }
        results.append(row);
    }

    emit queryExecuted(results);
}

} // namespace DatabaseLayer
