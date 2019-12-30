#pragma once

#include <domain/identifier.h>

#include <QMap>

namespace Domain {
    class DomainObject;
}

class QSqlRecord;
class QSqlQuery;


namespace DataMappingLayer
{

/**
 * @brief The AbstractMapper class
 */
class AbstractMapper
{
public:
    virtual ~AbstractMapper() = default;

    /**
     * @brief Очистить все загруженные ранее данные
     */
    void clear();

protected:
    virtual QString findStatement(const Domain::Identifier& _id) const = 0;
    virtual QString findAllStatement() const = 0;
    virtual QString insertStatement(Domain::DomainObject* _object, QVariantList& _values) const = 0;
    virtual QString updateStatement(Domain::DomainObject* _object, QVariantList& _values) const = 0;
    virtual QString deleteStatement(Domain::DomainObject* _object, QVariantList& _values) const = 0;

protected:
    /**
     * @brief Создать объект с заданным идентификатором из sql-записи
     */
    virtual Domain::DomainObject* doLoad(const Domain::Identifier& _id, const QSqlRecord& _record) = 0;

    /**
     * @brief Обновить параметры заданного объекта из sql-записи
     */
    virtual void doLoad(Domain::DomainObject* _object, const QSqlRecord& _record) = 0;

protected:
    Domain::DomainObject * abstractFind(const Domain::Identifier& _id);
    QVector<Domain::DomainObject*> abstractFind(const QString& _filter);
    void abstractInsert(Domain::DomainObject* _object);
    bool abstractUpdate(Domain::DomainObject* _object);
    void abstractDelete(Domain::DomainObject* _object);

    /**
     * @brief Выполнить запрос
     */
    bool executeSql(QSqlQuery& _sqlQuery);

protected:
    /**
     * @brief Скрываем конструктор от публичного доступа
     */
    AbstractMapper() = default;

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
    /**
     * @brief Загруженные объекты из базы данных
     */
    QMap<Domain::Identifier, Domain::DomainObject*> m_loadedObjectsMap;
};

} // namespace DataMappingLayer

