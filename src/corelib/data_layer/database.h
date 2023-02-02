#pragma once

#include <QtGlobal>

#include <corelib_global.h>

class QString;
class QSqlQuery;
class QSqlDatabase;


namespace DatabaseLayer {

class CORE_LIBRARY_EXPORT Database
{
public:
    /**
     * @brief Можно ли открыть заданный файл
     */
    static bool canOpenFile(const QString& _databaseFileName);

    /**
     * @brief Текст ошибки открытия последнего загружаемого файла
     */
    static QString openFileError();

    /**
     * @brief Текст последней ошибки базы данных
     * @note Если он пуст, то и ошибки нет
     */
    /** @{ */
    static bool hasError();
    static QString lastError();
    static void setLastError(const QString& _error);
    /** @} */

    /**
     * @brief Установить текущий файл базы данных
     */
    static void setCurrentFile(const QString& _databaseFileName);

    /**
     * @brief Закрыть текущее соединение с базой данных
     */
    static void closeCurrentFile();

    /**
     * @brief Получить имя текущей базы данных
     */
    static QString currentFile();

    /**
     * @brief Получить объект для выполнения запросов в БД
     */
    static QSqlQuery query();

    /**
     * @brief Запустить транзакцию, если ещё не запущена
     */
    static void transaction();

    /**
     * @brief Зафиксировать транзакцию, если она была запущена
     */
    static void commit();

    /**
     * @brief Сжать базу данных
     */
    static void vacuum();

    /**
     * @brief Состояния базы данных
     */
    enum State {
        //! Новая база данных
        EmptyFlag,
        //! Схема создана
        SchemeFlag,
        //! Индексы созданы
        IndexesFlag,
        //! Справочники созданы
        EnumsFlag,
        //! Старая версия
        OldVersionFlag
    };
    Q_DECLARE_FLAGS(States, State)

private:
    /**
     * @brief Получить объект текущей базы данных
     */
    static QSqlDatabase instanse();

    /**
     * @brief Открыть соединение с базой данных
     */
    static void open(QSqlDatabase& _database, const QString& _connectionName,
                     const QString& _databaseName);
    static Database::States checkState(QSqlDatabase& _database);
    static void createTables(QSqlDatabase& _database);
    static void createIndexes(QSqlDatabase& _database);
    static void createEnums(QSqlDatabase& _database);

    static void updateDatabase(QSqlDatabase& _database);
    static void updateDatabaseTo_0_0_10(QSqlDatabase& _database);
    static void updateDatabaseTo_0_1_3(QSqlDatabase& _database);
    static void updateDatabaseTo_0_2_4(QSqlDatabase& _database);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Database::States)

} // namespace DatabaseLayer
