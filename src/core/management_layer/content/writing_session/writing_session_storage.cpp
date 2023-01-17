#include "writing_session_storage.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/starcloud_api.h>

#include <QDateTime>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QVariant>


namespace DataStorageLayer {

namespace {
/**
 * @brief Название соединения с базой данных
 */
const QLatin1String kConnectionName("session_statistics_database");

/**
 * @brief Путь с файлом локальных сессий
 */
QString sessionsFilePath()
{
    const QString appDataFolderPath
        = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString sessionsFilePath = appDataFolderPath + QDir::separator() + "sessions.db";
    return sessionsFilePath;
}

/**
 * @brief Ключ для хранния информации о последней синхронизации
 */
const QLatin1String kLastSyncDateTimeKey("last_sync_date_time");

} // namespace

WritingSessionStorage::WritingSessionStorage()
{
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
    database.setDatabaseName(sessionsFilePath());
    database.open();

    QSqlQuery query(database);
    //
    // Проверяем создана ли структура базы данных файла сессий
    //
    if (query.exec("SELECT COUNT(*) as size FROM sqlite_master WHERE type = 'table' ")
        && query.next() && query.record().value("size").toInt() > 0) {
        //
        // ... если структура создана, то ничего не делаем
        //
    } else {
        //
        // ... а если структура не создана, то создаём её
        //
        query.exec("CREATE TABLE sessions "
                   "("
                   "uuid TEXT PRIMARY KEY ON CONFLICT REPLACE, "
                   "project_uuid TEXT NOT NULL, "
                   "project_name TEXT NOT NULL, "
                   "device_uuid TEXT NOT NULL, "
                   "device_name TEXT NOT NULL, "
                   "started_at TEXT NOT NULL, "
                   "ends_at TEXT NOT NULL, "
                   "words INTEGER NOT NULL, "
                   "characters INTEGER NOT NULL, "
                   "account_email TEXT NOT NULL "
                   ");");
        query.exec("CREATE TABLE system_variables "
                   "("
                   "variable TEXT PRIMARY KEY ON CONFLICT REPLACE, "
                   "value TEXT NOT NULL "
                   ");");
    }
}

QDateTime WritingSessionStorage::sessionStatisticsLastSyncDateTime() const
{
    QSqlQuery query(QSqlDatabase::database(kConnectionName));
    query.prepare("SELECT value FROM system_variables WHERE variable = ?");
    query.addBindValue(kLastSyncDateTimeKey);
    query.exec();
    query.next();
    return query.record().value(0).toDateTime();
}

void WritingSessionStorage::saveSessionStatisticsLastSyncDateTime(const QDateTime& _dateTime)
{
    QSqlQuery query(QSqlDatabase::database(kConnectionName));
    query.prepare("INSERT INTO system_variables VALUES (?,?)");
    query.addBindValue(kLastSyncDateTimeKey);
    query.addBindValue(_dateTime);
    query.exec();
}

QVector<Domain::SessionStatistics> WritingSessionStorage::sessionStatistics(
    const QDateTime& _fromDateTime) const
{
    //
    // Берём статистику только с пустым имейлом или таким же как текущий
    // Сортируем по времени создания
    //
    QSqlQuery query(QSqlDatabase::database(kConnectionName));
    if (_fromDateTime.isValid()) {
        query.prepare("SELECT * FROM sessions "
                      "WHERE ends_at >= ? AND (account_email = '' OR account_email = ?) "
                      "ORDER BY started_at ASC");
        query.addBindValue(_fromDateTime);
    } else {
        query.prepare("SELECT * FROM sessions "
                      "WHERE account_email = '' OR account_email = ? "
                      "ORDER BY started_at ASC");
    }
    query.addBindValue(DataStorageLayer::StorageFacade::settingsStorage()->accountEmail());
    query.exec();
    QVector<Domain::SessionStatistics> sessionStatistics;
    while (query.next()) {
        const auto session = query.record();
        sessionStatistics.append({
            session.value(0).toUuid(),
            session.value(1).toUuid(),
            session.value(2).toString(),
            session.value(3).toString(),
            session.value(4).toString(),
            session.value(5).toDateTime(),
            session.value(6).toDateTime(),
            session.value(7).toInt(),
            session.value(8).toInt(),
        });
    }
    return sessionStatistics;
}

void WritingSessionStorage::saveSessionStatistics(
    const QVector<Domain::SessionStatistics>& _sessionStatistics)
{
    auto database = QSqlDatabase::database(kConnectionName);
    database.transaction();
    QSqlQuery query(database);
    for (const auto& session : _sessionStatistics) {
        query.prepare("INSERT INTO sessions VALUES(?,?,?,?,?,?,?,?,?,?)");
        query.addBindValue(session.uuid);
        query.addBindValue(session.projectUuid);
        query.addBindValue(session.projectName);
        query.addBindValue(session.deviceUuid);
        query.addBindValue(session.deviceName);
        query.addBindValue(session.startDateTime);
        query.addBindValue(session.endDateTime);
        query.addBindValue(session.words);
        query.addBindValue(session.characters);
        query.addBindValue(DataStorageLayer::StorageFacade::settingsStorage()->accountEmail());
        query.exec();
    }
    database.commit();
}

} // namespace DataStorageLayer
