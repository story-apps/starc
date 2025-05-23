#pragma once

#include <domain/identifier.h>

#include <QUuid>

namespace Domain {
class DomainObject;
}

class QSqlRecord;
class QSqlQuery;


namespace DataMappingLayer {

/**
 * @brief The AbstractMapper class
 */
class AbstractMapper : public QObject
{
    Q_OBJECT

public:
    virtual ~AbstractMapper() = default;

    /**
     * @brief Очистить все загруженные ранее данные
     */
    void clear();

protected:
    virtual QString findStatement(const Domain::Identifier& _id) const = 0;
    virtual QString findAllStatement() const = 0;
    virtual QString findLastOneStatement() const = 0;
    virtual QString insertStatement(Domain::DomainObject* _object, QVariantList& _values) const = 0;
    virtual QString updateStatement(Domain::DomainObject* _object, QVariantList& _values) const = 0;
    virtual QString deleteStatement(Domain::DomainObject* _object, QVariantList& _values) const = 0;

protected:
    /**
     * @brief Создать объект с заданным идентификатором из sql-записи
     */
    virtual Domain::DomainObject* doLoad(const Domain::Identifier& _id, const QSqlRecord& _record)
        = 0;
    virtual Domain::DomainObject* doLoad(const Domain::Identifier& _id, const QVariantList& _record)
        = 0;

    /**
     * @brief Обновить параметры заданного объекта из sql-записи
     */
    virtual void doLoad(Domain::DomainObject* _object, const QSqlRecord& _record) = 0;
    virtual void doLoad(Domain::DomainObject* _object, const QVariantList& _record) = 0;

protected:
    /**
     * @brief Методы для синхронной работы
     */
    /** @{ */
    Domain::DomainObject* abstractFind(const Domain::Identifier& _id);
    QVector<Domain::DomainObject*> abstractFind(const QString& _filter);
    void abstractInsert(Domain::DomainObject* _object);
    bool abstractUpdate(Domain::DomainObject* _object);
    void abstractDelete(Domain::DomainObject* _object);
    /**
     * @brief Выполнить запрос
     */
    bool executeSql(QSqlQuery& _sqlQuery);
    /** @} */

    /**
     * @brief Методы для асинхронной работы
     */
    /** @{ */
    void abstractInsertAsync(const QUuid& _queryUuid, Domain::DomainObject* _object);
    void abstractUpdateAsync(const QUuid& _queryUuid, Domain::DomainObject* _object);
    void abstractFindAsync(const QUuid& _queryUuid, const QString& _filter);
    void abstractDeleteAsync(const QUuid& _queryUuid, Domain::DomainObject* _object);

public:
    Q_SIGNAL void objectsFound(const QUuid& _queryUuid, QVector<Domain::DomainObject*> _objects);
    Q_SIGNAL void queryFailed(const QUuid& _queryUuid, const QString& _error);
    /** @} */

protected:
    /**
     * @brief Скрываем конструктор от публичного доступа
     */
    AbstractMapper(QObject* _parent = nullptr);

private:
    /**
     * @brief Загрузить объект из БД по его идентификатору
     */
    Domain::DomainObject* loadObjectFromDatabase(const Domain::Identifier& _id);

    /**
     * @brief Получить идентификатор для нового объекта на сохранение в БД
     */
    Domain::Identifier findNextIdentifier();

    /**
     * @brief Загрузить или обновить объект из записи в БД
     */
    Domain::DomainObject* load(const QSqlRecord& _record);
    Domain::DomainObject* load(const QVariantList& _record);

private:
    /**
     * @brief Был ли загружен идентификатор последнего элемента
     */
    bool m_isLastIdentifierLoaded = false;

    /**
     * @brief Загруженные объекты из базы данных
     */
    std::map<Domain::Identifier, Domain::DomainObject*> m_loadedObjectsMap;

    /**
     * @brief Типы запросов
     */
    enum class QueryType {
        Insert,
        Update,
        Find,
        Delete,
    };

    /**
     * @brief Запросы к БД, отправленные на исполнение в отдельном потоке
     */
    std::map<QUuid, QueryType> m_sentQueries;

    /**
     * @brief Объекты, обрабатываемые в отдельном потоке
     */
    std::map<QUuid, Domain::DomainObject*> m_processingObjects;
};

} // namespace DataMappingLayer
