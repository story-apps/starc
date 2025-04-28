#include "database_worker.h"

#include <data_layer/database.h>

#include <QDebug>
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

void DatabaseWorker::executeQuery(const QString& _queryString, const QVariantList& _bindValues)
{
    QSqlQuery query = Database::query(s_connectionName);
    query.prepare(_queryString);
    for (const QVariant& value : std::as_const(_bindValues)) {
        query.addBindValue(value);
    }

    if (!query.exec()) {
        DatabaseLayer::Database::setLastError(query.lastError().text());
        qDebug() << query.lastError();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();

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
