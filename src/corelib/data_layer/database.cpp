#include "database.h"

#include <QApplication>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QVariant>

namespace DatabaseLayer {

namespace {
/**
 * @brief Название соединения с базой данных
 */
static QString s_connectionName = "local_database";

/**
 * @brief Плагин используемый для работы с базой
 */
static QString s_sqlDriver = "QSQLITE";

/**
 * @brief Имя файла базы данных, если не задан, то база будет размещаться во временной памяти
 */
static QString s_databaseName = ":memory:";

/**
 * @brief Текст ошибки открытия последнего загружаемого файла
 */
static QString s_openFileError;

/**
 * @brief Текст последней ошибки
 */
static QString s_lastError;

/**
 * @brief Счётчик открытых транзакций
 */
static int s_openedTransactions = 0;

/**
 * @brief Получить ключ хранения номера версии приложения
 */
static QString applicationVersionKey()
{
    return "application-version";
}
} // namespace

bool Database::canOpenFile(const QString& _databaseFileName)
{
    bool canOpen = true;

    {
        QSqlDatabase database = QSqlDatabase::addDatabase(s_sqlDriver, "tmp_database");
        database.setDatabaseName(_databaseFileName);
        database.open();

        QSqlQuery q_checker(database);

        //
        // 1. Если файл был создан в более поздней версии приложения, его нельзя открывать
        //
        if (q_checker.exec(
                "SELECT value FROM system_variables WHERE variable = 'application-version' ")
            && q_checker.next()
            && q_checker.value("value").toString().split(" ").first()
                > QApplication::applicationVersion()) {
            canOpen = false;
            s_openFileError = QApplication::translate("DatabaseLayer::Database",
                                                      "Project was modified in a newer version. "
                                                      "Update to the latest version to open it.");
        }
    }

    QSqlDatabase::removeDatabase("tmp_database");

    return canOpen;
}

QString Database::openFileError()
{
    return s_openFileError;
}

bool Database::hasError()
{
    return !s_lastError.isEmpty();
}

QString Database::lastError()
{
    return s_lastError;
}

void Database::setLastError(const QString& _error)
{
    if (s_lastError != _error) {
        s_lastError = _error;
    }
}

void Database::setCurrentFile(const QString& _databaseFileName)
{
    //
    // Если использовалась база данных, то удалим старое соединение
    //
    closeCurrentFile();

    //
    // Установим текущее имя базы данных
    //
    if (s_databaseName != _databaseFileName) {
        s_databaseName = _databaseFileName;
        s_connectionName = "local_database [" + s_databaseName + "]";
    }

    //
    // Откроем базу данных, или создадим новую
    //
    instanse();
}

void Database::closeCurrentFile()
{
    if (QSqlDatabase::contains(s_connectionName)) {
        QSqlDatabase::removeDatabase(s_connectionName);
    }
}

QString Database::currentFile()
{
    return instanse().databaseName();
}

QSqlQuery Database::query()
{
    return QSqlQuery(instanse());
}

void Database::transaction()
{
    //
    // Для первого запроса открываем транзакцию
    //
    if (s_openedTransactions == 0) {
        instanse().transaction();
    }

    //
    // Увеличиваем счётчик открытых транзакций
    //
    ++s_openedTransactions;
}

void Database::commit()
{
    //
    // Уменьшаем счётчик транзакций
    //
    --s_openedTransactions;

    //
    // При закрытии корневой транзакции фиксируем изменения в базе данных
    //
    if (s_openedTransactions == 0) {
        instanse().commit();
    }
}

void Database::rollback()
{
    //
    // Уменьшаем счётчик транзакций
    //
    --s_openedTransactions;

    //
    // При закрытии корневой транзакции отменяем изменения в базе данных
    //
    if (s_openedTransactions == 0) {
        instanse().rollback();
    }
}

void Database::vacuum()
{
    auto query = Database::query();
    query.exec("VACUUM");
}

// ****

QSqlDatabase Database::instanse()
{
    QSqlDatabase database;

    if (!QSqlDatabase::contains(s_connectionName)) {
        open(database, s_connectionName, s_databaseName);
    } else {
        database = QSqlDatabase::database(s_connectionName);
    }

    return database;
}

void Database::open(QSqlDatabase& _database, const QString& _connectionName,
                    const QString& _databaseName)
{
    s_lastError.clear();

    _database = QSqlDatabase::addDatabase(s_sqlDriver, _connectionName);
    _database.setDatabaseName(_databaseName);
    _database.open();

    Database::States states = checkState(_database);

    if (!states.testFlag(SchemeFlag))
        createTables(_database);
    if (!states.testFlag(IndexesFlag))
        createIndexes(_database);
    if (!states.testFlag(EnumsFlag))
        createEnums(_database);
    if (states.testFlag(OldVersionFlag))
        updateDatabase(_database);
}

// Проверка состояния базы данных
// например:
// - БД отсутствует
// - БД пуста
// - БД имеет старую версию
// - БД имеет последнюю версию
// - и т.д.
Database::States Database::checkState(QSqlDatabase& _database)
{
    QSqlQuery q_checker(_database);
    Database::States states = Database::EmptyFlag;

    //
    // Созданы ли таблицы
    //
    if (q_checker.exec("SELECT COUNT(*) as size FROM sqlite_master WHERE type = 'table' ")
        && q_checker.next() && q_checker.record().value("size").toInt()) {
        //
        // Все остальные проверки имеют смысл, только если проходит данная проверка
        //
        states = states | Database::SchemeFlag;

        //
        // Созданы ли индексы
        //
        if (q_checker.exec("SELECT COUNT(*) as size FROM sqlite_master WHERE type = 'index' ")
            && q_checker.next() && q_checker.record().value("size").toInt()) {
            states = states | Database::IndexesFlag;
        }

        //
        // Проверка версии
        //
        if (q_checker.exec(
                QString("SELECT value as version FROM system_variables WHERE variable = '%1' ")
                    .arg(applicationVersionKey()))) {
            //
            // Если версия не задана, или она не соответствует текущей, то надо обновить файл
            //
            if (!q_checker.next()
                || q_checker.record().value("version").toString()
                    != QApplication::applicationVersion()) {
                states = states | Database::OldVersionFlag;
            }
        }
    }

    return states;
}

void Database::createTables(QSqlDatabase& _database)
{
    QSqlQuery query(_database);
    _database.transaction();

    //
    // Таблица системных переменных
    //
    query.exec("CREATE TABLE system_variables "
               "("
               "variable TEXT PRIMARY KEY ON CONFLICT REPLACE, "
               "value TEXT NOT NULL "
               ");");

    //
    // Таблица с документами
    //
    query.exec("CREATE TABLE documents "
               "("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "uuid TEXT UNIQUE NOT NULL, "
               "type INTEGER NOT NULL DEFAULT(0), "
               "content BLOB DEFAULT(NULL), "
               "synced_at TEXT DEFAULT(NULL) "
               ")");

    //
    // Таблица с изменениями документов
    //
    query.exec("CREATE TABLE documents_changes "
               "("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "fk_document_uuid TEXT NOT NULL, "
               "uuid TEXT UNIQUE NOT NULL, "
               "undo_patch BLOB NOT NULL, " // отмена изменения
               "redo_patch BLOB NOT NULL, " // повтор изменения
               "date_time TEXT NOT NULL, " // yyyy.mm.dd.hh.mm.ss.zzz
               "user_name TEXT NOT NULL, "
               "user_email TEXT DEFAULT(NULL), "
               "is_synced INTEGER NOT NULL DEFAULT(0) "
               ")");

    _database.commit();
}

void Database::createIndexes(QSqlDatabase& _database)
{
    QSqlQuery query(_database);
    _database.transaction();

    //
    // Таблица с изменениями документов
    //
    query.exec("CREATE INDEX documents_changes_fk_document_uuid_idx "
               "ON documents_changes (fk_document_uuid)");
    query.exec("CREATE INDEX documents_changes_date_time_idx "
               "ON documents_changes (date_time)");

    _database.commit();
}

void Database::createEnums(QSqlDatabase& _database)
{
    Q_UNUSED(_database)
}

void Database::updateDatabase(QSqlDatabase& _database)
{
    QSqlQuery query(_database);

    //
    // Определим версию базы данных
    //
    query.prepare("SELECT value as version FROM system_variables WHERE variable = ? ");
    query.addBindValue(applicationVersionKey());
    query.exec();
    query.next();
    auto databaseVersion = query.record().value("version").toString();
    //
    // Извлекаем информацию о dev-версии
    //
    bool isDevVersion = false;
    if (const auto devIndex = databaseVersion.indexOf(" dev "); devIndex > 0) {
        isDevVersion = true;
        databaseVersion = databaseVersion.left(devIndex);
    }
    const QStringList& versionParts = databaseVersion.split(".");
    int versionMajor = versionParts.value(0, "0").toInt();
    int versionMinor = versionParts.value(1, "0").toInt();
    int versionBuild = versionParts.value(2, "1").toInt();
    //
    // Корректируем номер версии если это dev-сборка
    //
    if (isDevVersion) {
        //
        // x.x.3 dev -> x.x.2
        //
        if (versionBuild > 0) {
            --versionBuild;
        }
        //
        // x.x.0 dev ->
        //
        else {
            versionBuild = 999;
            //
            // x.3.0 dev -> x.2.999
            //
            if (versionMinor > 0) {
                --versionMinor;
            }
            //
            // x.0.0 dev ->
            //
            else {
                versionMinor = 999;
                //
                // 1.0.0 dev -> 0.999.999
                //
                if (versionMajor > 0) {
                    --versionMajor;
                }
                //
                // 0.0.0 dev - такого не может быть!
                //
                else {
                    Q_ASSERT(false);
                }
            }
        }
    }

    //
    // Вызываются необходимые процедуры обновления БД в зависимости от её версии
    //
    // 0.X.X
    //
    if (versionMajor <= 0) {
        //
        // 0.0.X
        //
        if (versionMajor < 0 || versionMinor <= 0) {
            //
            // 0.0.9
            //
            if (versionMajor < 0 || versionMinor < 0 || versionBuild <= 9) {
                updateDatabaseTo_0_0_10(_database);
            }
        }
        //
        // 0.1.X
        //
        if (versionMajor < 0 || versionMinor <= 1) {
            //
            // 0.1.2
            //
            if (versionMajor < 0 || versionMinor < 1 || versionBuild <= 2) {
                updateDatabaseTo_0_1_3(_database);
            }
        }
        //
        // 0.2.x
        //
        if (versionMajor < 0 || versionMinor <= 2) {
            //
            // 0.2.3
            //
            if (versionMajor < 0 || versionMinor < 2 || versionBuild <= 3) {
                updateDatabaseTo_0_2_4(_database);
            }
        }
        //
        // 0.6.x
        //
        if (versionMajor < 0 || versionMinor <= 6) {
            //
            // 0.6.1
            //
            if (versionMajor < 0 || versionMinor < 6 || versionBuild <= 1) {
                updateDatabaseTo_0_6_2(_database);
            }
        }
    }

    //
    // Сохраняем информацию об обновлении версии
    //
    query.exec(
        QString("INSERT INTO system_variables VALUES ('application-updated-to-version-%1', '%2')")
            .arg(QApplication::applicationVersion())
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate)));

    //
    // Обновляется версия программы
    //
    query.exec(QString("INSERT INTO system_variables VALUES ('%1', '%2')")
                   .arg(applicationVersionKey())
                   .arg(QApplication::applicationVersion()));
}

void Database::updateDatabaseTo_0_0_10(QSqlDatabase& _database)
{
    //
    // В структуре хранились кривые цвета элементов проекта, поэтому удаляем их
    //

    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        //
        // Извлекаем структуру проекта
        //
        q_updater.exec("SELECT content FROM documents WHERE type = 1");
        QString structure;
        if (q_updater.next()) {
            structure = q_updater.record().value("content").toString();
        }
        //
        // ... удаляем цвета
        //
        structure = structure.remove("#000000");
        //
        // ... обновим данные
        //
        q_updater.prepare("UPDATE documents SET content = ? WHERE type = 1");
        q_updater.addBindValue(structure);
        q_updater.exec();
    }

    _database.commit();
}

void Database::updateDatabaseTo_0_1_3(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    //
    // Изменить в комиксах блоки page_name & panel_name на page_heading & panel_heading
    //

    {
        //
        // Извлекаем документы комиксов
        //
        std::map<QString, QString> comicBooks;
        q_updater.exec("SELECT id, content FROM documents WHERE type = 10203");
        while (q_updater.next()) {
            comicBooks[q_updater.record().value("id").toString()]
                = q_updater.record()
                      .value("content")
                      .toString()
                      .replace("page_name", "page_heading")
                      .replace("panel_name", "panel_heading");
        }

        //
        // Обновим данные
        //
        for (const auto& [id, content] : comicBooks) {
            q_updater.prepare("UPDATE documents SET content = ? WHERE id = ?");
            q_updater.addBindValue(content);
            q_updater.addBindValue(id);
            q_updater.exec();
        }
    }

    //
    // Изменить в сценариях элементы
    // folder_header -> sequence_heading
    // folder_footer -> sequence_footer
    // folder -> sequence
    //

    {
        //
        // Извлекаем документы комиксов
        //
        std::map<QString, QString> comicBooks;
        q_updater.exec("SELECT id, content FROM documents WHERE type = 10104");
        while (q_updater.next()) {
            comicBooks[q_updater.record().value("id").toString()]
                = q_updater.record()
                      .value("content")
                      .toString()
                      .replace("folder_header", "sequence_heading")
                      .replace("folder_footer", "sequence_footer")
                      .replace("folder", "sequence");
        }

        //
        // Обновим данные
        //
        for (const auto& [id, content] : comicBooks) {
            q_updater.prepare("UPDATE documents SET content = ? WHERE id = ?");
            q_updater.addBindValue(content);
            q_updater.addBindValue(id);
            q_updater.exec();
        }
    }

    _database.commit();
}

void Database::updateDatabaseTo_0_2_4(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    q_updater.exec("ALTER TABLE documents_changes ADD is_synced INTEGER NOT NULL DEFAULT(0)");

    _database.commit();
}

void Database::updateDatabaseTo_0_6_2(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    q_updater.exec("ALTER TABLE documents ADD synced_at TEXT DEFAULT(NULL)");

    _database.commit();
}

} // namespace DatabaseLayer
