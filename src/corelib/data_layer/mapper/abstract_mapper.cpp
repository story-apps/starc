#include "abstract_mapper.h"

#include <data_layer/database_manager.h>
#include <domain/domain_object.h>

#include <QDebug>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>
#include <QVariant>

using DatabaseLayer::DatabaseManager;
using Domain::DomainObject;
using Domain::Identifier;


namespace DataMappingLayer {

/**
 * @brief Типы запросов
 */
enum class QueryType {
    Insert,
    Update,
    Find,
    Delete,
};

class AbstractMapper::Implementation
{
public:
    /**
     * @brief Был ли загружен идентификатор последнего элемента
     */
    bool m_isLastIdentifierLoaded = false;

    /**
     * @brief Загруженные объекты из базы данных
     */
    std::map<Domain::Identifier, Domain::DomainObject*> m_loadedObjectsMap;

    /**
     * @brief Запросы к БД, отправленные на исполнение в отдельном потоке
     */
    QHash<QUuid, QueryType> m_sentQueries;

    /**
     * @brief Объекты, обрабатываемые в отдельном потоке
     */
    QHash<QUuid, Domain::DomainObject*> m_processingObjects;
};


// ****


AbstractMapper::~AbstractMapper() = default;

void AbstractMapper::clear()
{
    d->m_isLastIdentifierLoaded = false;
    for (auto& [key, value] : d->m_loadedObjectsMap) {
        delete value;
        value = nullptr;
    }
    d->m_loadedObjectsMap.clear();
}

DomainObject* AbstractMapper::abstractFind(const Identifier& _id)
{
    const auto objectIter = d->m_loadedObjectsMap.find(_id);
    DomainObject* result = nullptr;
    if (objectIter != d->m_loadedObjectsMap.cend()) {
        result = objectIter->second;
    } else {
        result = loadObjectFromDatabase(_id);
    }
    return result;
}

QVector<Domain::DomainObject*> AbstractMapper::abstractFind(const QString& _filter)
{
    QSqlQuery query = DatabaseManager::query();
    query.prepare(findAllStatement() + _filter);
    query.exec();
    QVector<Domain::DomainObject*> result;
    while (query.next()) {
        QSqlRecord record = query.record();
        DomainObject* domainObject = load(record);
        result.append(domainObject);
    }
    return result;
}

bool AbstractMapper::abstractInsert(DomainObject* _object)
{
    //
    // Установим идентификатор для нового объекта
    //
    _object->setId(findNextIdentifier());

    //
    // Добавим вновь созданный объект в список загруженных объектов
    //
    d->m_loadedObjectsMap.emplace(_object->id(), _object);

    //
    // Получим данные для формирования запроса на их добавление
    //
    QVariantList insertValues;
    QString insertQueryString = insertStatement(_object, insertValues);

    //
    // Сформируем запрос на добавление данных в базу
    //
    QSqlQuery insertQuery = DatabaseManager::query();
    insertQuery.prepare(insertQueryString);
    for (const QVariant& value : std::as_const(insertValues)) {
        insertQuery.addBindValue(value);
    }

    //
    // Добавим данные в базу
    //
    const auto isInsertSuccesful = executeSql(insertQuery);
    if (isInsertSuccesful) {
        _object->markChangesStored();
    }
    return isInsertSuccesful;
}

bool AbstractMapper::abstractUpdate(DomainObject* _object)
{
    //
    // Обновления не было
    //
    if (_object->isChangesStored()) {
        return false;
    }

    //
    // т.к. в d->m_loadedObjectsMap хранится список указателей, то после обновления элементов
    // обновлять элемент непосредственно в списке не нужно
    //

    //
    // Получим данные для формирования запроса на их обновление
    //
    QVariantList updateValues;
    const QString updateQueryString = updateStatement(_object, updateValues);

    //
    // Сформируем запрос на обновление данных в базе
    //
    QSqlQuery updateQuery = DatabaseManager::query();
    updateQuery.prepare(updateQueryString);
    for (const QVariant& value : std::as_const(updateValues)) {
        updateQuery.addBindValue(value);
    }

    //
    // Обновим данные в базе
    //
    const bool isUpdateSuccesful = executeSql(updateQuery);
    if (isUpdateSuccesful) {
        _object->markChangesStored();
    }
    return isUpdateSuccesful;
}

bool AbstractMapper::abstractDelete(DomainObject* _object)
{
    //
    // Получим данные для формирования запроса на их удаление
    //
    QVariantList deleteValues;
    QString deleteQueryString = deleteStatement(_object, deleteValues);

    //
    // Сформируем запрос на удаление данных из базы
    //
    QSqlQuery deleteQuery = DatabaseManager::query();
    deleteQuery.prepare(deleteQueryString);
    for (const QVariant& value : std::as_const(deleteValues)) {
        deleteQuery.addBindValue(value);
    }

    //
    // Удалим данные из базы
    //
    const bool isDeleteSuccesful = executeSql(deleteQuery);
    if (isDeleteSuccesful) {
        //
        // Удалим объекст из списка загруженных
        //
        d->m_loadedObjectsMap.erase(_object->id());
        delete _object;
        _object = nullptr;
    }
    return isDeleteSuccesful;
}

bool AbstractMapper::executeSql(QSqlQuery& _sqlQuery)
{
    const bool isExecutionSuccesful = _sqlQuery.exec();
    if (isExecutionSuccesful) {
        return true;
    }

    //
    // Если запрос завершился с ошибкой, выводим отладочную информацию
    //
    DatabaseManager::setLastError(_sqlQuery.lastError().text());

    qDebug() << _sqlQuery.lastError();
    qDebug() << _sqlQuery.lastQuery();
    qDebug() << _sqlQuery.boundValues();

    return false;
}

QUuid AbstractMapper::abstractInsertAsync(Domain::DomainObject* _object)
{
    //
    // Установим идентификатор для нового объекта
    //
    _object->setId(findNextIdentifier());

    //
    // Добавим вновь созданный объект в список загруженных объектов
    //
    d->m_loadedObjectsMap.emplace(_object->id(), _object);

    //
    // Получим данные для формирования запроса на их добавление
    //
    QVariantList insertValues;
    QString insertQueryString = insertStatement(_object, insertValues);

    //
    // Отправим запрос на исполнение
    //
    const auto queryUuid
        = DatabaseManager::instance().executeQuery(insertQueryString, insertValues);

    //
    // Запомним тип запроса
    //
    d->m_sentQueries.insert(queryUuid, QueryType::Insert);

    return queryUuid;
}

QUuid AbstractMapper::abstractUpdateAsync(Domain::DomainObject* _object)
{
    //
    // Обновления не было
    //
    if (_object->isChangesStored()) {
        return QUuid();
    }

    //
    // т.к. в d->m_loadedObjectsMap хранится список указателей, то после обновления элементов
    // обновлять элемент непосредственно в списке не нужно
    //

    //
    // Получим данные для формирования запроса на их обновление
    //
    QVariantList updateValues;
    const QString updateQueryString = updateStatement(_object, updateValues);

    //
    // Отправим запрос на исполнение
    //
    const auto queryUuid
        = DatabaseManager::instance().executeQuery(updateQueryString, updateValues);

    //
    // Запомним тип запроса
    //
    d->m_sentQueries.insert(queryUuid, QueryType::Update);

    //
    // Запомним объект, который обновляем
    //
    d->m_processingObjects.insert(queryUuid, _object);

    return queryUuid;
}

QUuid AbstractMapper::abstractFindAsync(const QString& _filter)
{
    //
    // Отправим запрос на исполнение
    //
    const auto queryUuid
        = DatabaseManager::instance().executeQuery(findAllStatement() + _filter, {});

    //
    // Запомним тип запроса
    //
    d->m_sentQueries.insert(queryUuid, QueryType::Find);

    return queryUuid;
}

QUuid AbstractMapper::abstractDeleteAsync(Domain::DomainObject* _object)
{
    //
    // Получим данные для формирования запроса на их удаление
    //
    QVariantList deleteValues;
    QString deleteQueryString = deleteStatement(_object, deleteValues);

    //
    // Отправим запрос на исполнение
    //
    const auto queryUuid
        = DatabaseManager::instance().executeQuery(deleteQueryString, deleteValues);

    //
    // Запомним тип запроса
    //
    d->m_sentQueries.insert(queryUuid, QueryType::Delete);

    //
    // Запомним объект, который удаляем
    //
    d->m_processingObjects.insert(queryUuid, _object);

    _object = nullptr;

    return queryUuid;
}

AbstractMapper::AbstractMapper(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(&DatabaseManager::instance(), &DatabaseManager::queryFinished, this,
            [this](const QUuid& _queryUuid, const QVector<QSqlRecord>& _records) {
                const auto query = d->m_sentQueries.find(_queryUuid);
                if (query == d->m_sentQueries.cend()) {
                    return;
                }
                const auto queryType = query.value();
                d->m_sentQueries.erase(query);

                switch (queryType) {
                case QueryType::Find: {
                    QVector<Domain::DomainObject*> objects;
                    for (const auto& record : _records) {
                        objects.append(load(record));
                    }
                    emit objectsFound(_queryUuid, objects);
                    break;
                }
                case QueryType::Update: {
                    const auto updatingObject = d->m_processingObjects.find(_queryUuid);
                    if (updatingObject != d->m_processingObjects.cend()) {
                        updatingObject.value()->markChangesStored();
                        d->m_processingObjects.erase(updatingObject);
                    }
                    break;
                }
                case QueryType::Delete: {
                    const auto deletingObject = d->m_processingObjects.find(_queryUuid);
                    if (deletingObject != d->m_processingObjects.cend()) {
                        d->m_loadedObjectsMap.erase(deletingObject.value()->id());
                        delete deletingObject.value();
                        d->m_processingObjects.erase(deletingObject);
                    }
                    break;
                }
                case QueryType::Insert: {
                    break;
                }
                default: {
                    Q_ASSERT(false);
                }
                }
            });
    connect(&DatabaseManager::instance(), &DatabaseManager::queryFailed, this,
            [this](const QUuid& _queryUuid, const QString& _error) {
                const auto query = d->m_sentQueries.find(_queryUuid);
                //
                // Удалим запрос из списка отправленных
                //
                if (query != d->m_sentQueries.cend()) {
                    const auto processingObject = d->m_processingObjects.find(_queryUuid);
                    //
                    // ... и удалим объект из списка обрабатываемых
                    //
                    if (processingObject != d->m_processingObjects.cend()) {
                        d->m_processingObjects.erase(processingObject);
                    }
                    d->m_sentQueries.erase(query);
                    emit queryFailed(_queryUuid, _error);
                }
            });
}

DomainObject* AbstractMapper::loadObjectFromDatabase(const Identifier& _id)
{
    QSqlQuery query = DatabaseManager::query();
    query.prepare(findStatement(_id));
    query.exec();
    query.next();
    const QSqlRecord record = query.record();
    DomainObject* result = load(record);
    return result;
}

Identifier AbstractMapper::findNextIdentifier()
{
    if (!d->m_isLastIdentifierLoaded) {
        //
        // Если нет ещё последнего индекса по таблице, загрузим его
        //
        QSqlQuery query = DatabaseManager::query();
        query.prepare(findLastOneStatement());
        query.exec();
        query.next();
        const QSqlRecord record = query.record();
        load(record);

        d->m_isLastIdentifierLoaded = true;
    }

    Identifier maxId(0);
    if (!d->m_loadedObjectsMap.empty()) {
        auto iter = d->m_loadedObjectsMap.cend();
        --iter;
        maxId = iter->first;
    }
    return maxId.next();
}

DomainObject* AbstractMapper::load(const QSqlRecord& _record)
{
    const int idValue = _record.value("id").toInt();
    if (idValue == 0) {
        return nullptr;
    }

    Identifier id(idValue);
    DomainObject* result = nullptr;
    //
    // Если объект загружен, используем указатель на него
    //
    const auto iter = d->m_loadedObjectsMap.find(id);
    if (iter != d->m_loadedObjectsMap.cend()) {
        result = iter->second;
        //
        // ... если данные не были изменены, обновляем объект
        //
        if (result->isChangesStored()) {
            doLoad(result, _record);
        }
    }
    //
    // В противном случае создаём новый объект и сохраняем указатель на него
    //
    else {
        result = doLoad(id, _record);
        d->m_loadedObjectsMap.emplace(id, result);
    }
    return result;
}

} // namespace DataMappingLayer
