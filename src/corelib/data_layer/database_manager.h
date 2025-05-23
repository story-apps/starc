#pragma once

#include <QObject>
#include <QVariant>

#include <corelib_global.h>


class QSqlQuery;

namespace ManagementLayer {

class CORE_LIBRARY_EXPORT DatabaseManager : public QObject
{
    Q_OBJECT

public:
    ~DatabaseManager();

    static DatabaseManager& instance();
    /**
     * @brief Интерфейс для асинхронной работы
     */
    /** @{ */
    void enqueueQuery(const QUuid& _queryUuid, const QString& _query,
                      const QVariantList& _bindValues);
    /** @} */

    /**
     * @brief Интерфейс для синхронной работы
     */
    /** @{ */
    static bool canOpenFile(const QString& _databaseFileName);
    static QString openFileError();
    static bool hasError();
    static QString lastError();
    static void setLastError(const QString& _error);
    static void setCurrentFile(const QString& _databaseFileName);
    static void closeCurrentFile();
    static QString currentFile();
    static QSqlQuery query();
    static void transaction();
    static void commit();
    static void vacuum();
    /** @} */

private:
    explicit DatabaseManager(QObject* _parent = nullptr);

private slots:
    void onQueryExecuted(const QUuid& _queryUuid, const QVector<QVariantList>& _results);
    void onQueryFailed(const QUuid& _queryUuid, const QString& _error);

signals:
    void executeQuery(const QUuid& _queryUuid, const QString& _query,
                      const QVariantList& _bindValues);
    void queryResult(const QUuid& _queryUuid, const QVector<QVariantList>& _results);
    void queryFailed(const QUuid& _queryUuid, const QString& _error);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
