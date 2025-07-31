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
    virtual ~AbstractMapper();

    /**
     * @brief Очистить все загруженные ранее данные
     */
    void clear();

signals:
    void objectsFound(const QUuid& _queryUuid, QVector<Domain::DomainObject*> _objects);
    void queryFailed(const QUuid& _queryUuid, const QString& _error);

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

    /**
     * @brief Обновить параметры заданного объекта из sql-записи
     */
    virtual void doLoad(Domain::DomainObject* _object, const QSqlRecord& _record) = 0;

protected:
    /**
     * @brief Методы для синхронной работы
     */
    /** @{ */
    Domain::DomainObject* abstractFind(const Domain::Identifier& _id);
    QVector<Domain::DomainObject*> abstractFind(const QString& _filter);
    bool abstractInsert(Domain::DomainObject* _object);
    bool abstractUpdate(Domain::DomainObject* _object);
    bool abstractDelete(Domain::DomainObject* _object);

    /**
     * @brief Выполнить запрос
     */
    bool executeSql(QSqlQuery& _sqlQuery);
    /** @} */

    /**
     * @brief Методы для асинхронной работы
     * @return Гуид запроса
     */
    /** @{ */
    QUuid abstractInsertAsync(Domain::DomainObject* _object);
    QUuid abstractUpdateAsync(Domain::DomainObject* _object);
    QUuid abstractFindAsync(const QString& _filter);
    QUuid abstractDeleteAsync(Domain::DomainObject* _object);
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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace DataMappingLayer
