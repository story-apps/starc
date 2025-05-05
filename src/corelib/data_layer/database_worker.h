#pragma once

#include <QObject>
#include <QVariant>


namespace DatabaseLayer {

class DatabaseWorker : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseWorker(QObject* _parent = nullptr);

public slots:
    void executeQuery(const QUuid& _queryUuid, const QString& _queryString,
                      const QVariantList& _bindValues);

signals:
    void queryExecuted(const QUuid& _queryUuid, const QVector<QVariantList>& _results);
    void queryFailed(const QUuid& _queryUuid, const QString& _error);
};

} // namespace DatabaseLayer
