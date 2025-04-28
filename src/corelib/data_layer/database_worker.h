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
    void executeQuery(const QString& _queryString, const QVariantList& _bindValues);

signals:
    void queryExecuted(const QVector<QVariantList>& _results);
    void queryFailed(const QString& _error);
};

} // namespace DatabaseLayer
