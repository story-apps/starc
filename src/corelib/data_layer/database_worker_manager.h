#pragma once

#include <QObject>
#include <QVariant>

#include <corelib_global.h>

namespace ManagementLayer {

class CORE_LIBRARY_EXPORT DatabaseWorkerManager : public QObject
{
    Q_OBJECT

public:
    ~DatabaseWorkerManager();

    static DatabaseWorkerManager& instance();
    void enqueueQuery(const QString& _sqlQuery);

private:
    explicit DatabaseWorkerManager(QObject* _parent = nullptr);

private slots:
    void onQueryExecuted(const QVector<QVariantList>& _results);
    void onQueryFailed(const QString& _error);

signals:
    void executeQuery(const QString& _sqlQuery);
    void queryResult(const QVector<QVariantList>& _results);
    void queryFailed(const QString& _error);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
