#pragma once

#include "query.h"

#include <QObject>
#include <QSqlRecord>


namespace DatabaseLayer {
class DatabaseWorker : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseWorker(QObject* _parent = nullptr);
    ~DatabaseWorker();

    /**
     * @brief Выполнить очередь запросов к БД
     * @note Если запросов больше одного, они выполнятся в транзакции
     */
    void execute(const QVector<Query>& _queries);

signals:
    void transactionExecuted();
    void transactionFailed();
    void queryExecuted(const QUuid& _queryUuid, const QVector<QSqlRecord>& _results);
    void queryFailed(const QUuid& _queryUuid, const QString& _error);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace DatabaseLayer
