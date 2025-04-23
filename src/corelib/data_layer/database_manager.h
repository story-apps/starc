#pragma once

#include "query.h"

#include <QObject>
#include <QVariantList>
#include <QtSql/QSqlRecord>

#include <corelib_global.h>


class QSqlQuery;

namespace DatabaseLayer {

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
    /**
     * @brief Выполнить запрос в отдельном потоке
     */
    QUuid executeQuery(const QString& _query, const QVariantList& _bindValues);

    /**
     * @brief Начать собирать запросы для транзакции в отдельном потоке
     */
    void beginTransaction();

    /**
     * @brief Выполнить транзакцию в отдельном потоке
     */
    void executeTransaction();
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
    static bool transaction();
    static void commit();
    static void rollback();
    static void vacuum();
    /** @} */

signals:
    void queriesRequested(const QVector<Query>& _queries);
    void queryFinished(const QUuid& _queryUuid, const QVector<QSqlRecord>& _results);
    void queryFailed(const QUuid& _queryUuid, const QString& _error);

private:
    explicit DatabaseManager(QObject* _parent = nullptr);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace DatabaseLayer
