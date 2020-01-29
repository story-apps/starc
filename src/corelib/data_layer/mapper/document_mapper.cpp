#include "document_mapper.h"

#include <data_layer/database.h>

#include <domain/document_object.h>
#include <domain/objects_builder.h>

#include <QSqlRecord>

using Domain::DomainObject;
using Domain::Identifier;
using Domain::DocumentObject;
using Domain::DocumentObjectType;


namespace DataMappingLayer
{

namespace {
    const QString kColumns = " id, uuid, type, content ";
    const QString kTableName = " documents ";
    QString uuidFilter(const QUuid& _uuid) {
        return QString(" WHERE uuid = '%1' ").arg(_uuid.toString());
    }
    QString typeFilter(Domain::DocumentObjectType _type) {
        return QString(" WHERE type = %1 ").arg(static_cast<int>(_type));
    }
}


DocumentObject* DocumentMapper::find(const Identifier& _id)
{
    return dynamic_cast<DocumentObject*>(abstractFind(_id));
}

DocumentObject* DocumentMapper::find(const QUuid& _uuid)
{
    const auto domainObjects = abstractFind(uuidFilter(_uuid));
    if (domainObjects.isEmpty()) {
        return nullptr;
    }

    return dynamic_cast<DocumentObject*>(domainObjects.first());
}

Domain::DocumentObject* DocumentMapper::find(Domain::DocumentObjectType _type)
{
    const auto domainObjects = abstractFind(typeFilter(_type));
    if (domainObjects.isEmpty()) {
        return nullptr;
    }

    return dynamic_cast<DocumentObject*>(domainObjects.first());
}

DocumentObject* DocumentMapper::findStructure()
{
    return find(QUuid());
}

void DocumentMapper::insert(DocumentObject* _object)
{
    abstractInsert(_object);
}

bool DocumentMapper::update(DocumentObject* _object)
{
    return abstractUpdate(_object);
}

void DocumentMapper::remove(DocumentObject* _object)
{
    abstractDelete(_object);
}

QString DocumentMapper::findStatement(const Identifier& _id) const
{
    QString findStatement =
            QString("SELECT " + kColumns +
                    " FROM " + kTableName +
                    " WHERE id = %1 "
                    )
            .arg(_id.value());
    return findStatement;
}

QString DocumentMapper::findAllStatement() const
{
    return "SELECT " + kColumns + " FROM  " + kTableName;
}

QString DocumentMapper::findLastOneStatement() const
{
    return findAllStatement()
            + QString("WHERE id IN (SELECT id FROM %1 ORDER BY id DESC LIMIT %2)").arg(kTableName).arg(1);
}

QString DocumentMapper::insertStatement(DomainObject* _object, QVariantList& _insertValues) const
{
    const QString insertStatement
            = QString("INSERT INTO " + kTableName +
                      " (" + kColumns + ") "
                      " VALUES(?, ?, ?, ?) "
                      );

    const auto documentObject = dynamic_cast<DocumentObject*>(_object);
    _insertValues.clear();
    _insertValues.append(documentObject->id().value());
    _insertValues.append(documentObject->uuid().toString());
    _insertValues.append(static_cast<int>(documentObject->type()));
    _insertValues.append(documentObject->content());

    return insertStatement;
}

QString DocumentMapper::updateStatement(DomainObject* _object, QVariantList& _updateValues) const
{
    const QString updateStatement
            = QString("UPDATE " + kTableName +
                      " SET uuid = ?, "
                      " type = ?, "
                      " content = ? "
                      " WHERE id = ? "
                      );

    const auto documentObject = dynamic_cast<DocumentObject*>(_object);
    _updateValues.clear();
    _updateValues.append(documentObject->uuid().toString());
    _updateValues.append(static_cast<int>(documentObject->type()));
    _updateValues.append(documentObject->content());
    _updateValues.append(documentObject->id().value());

    return updateStatement;
}

QString DocumentMapper::deleteStatement(DomainObject* _object, QVariantList& _deleteValues) const
{
    QString deleteStatement = "DELETE FROM " + kTableName + " WHERE id = ?";

    _deleteValues.clear();
    _deleteValues.append(_object->id().value());

    return deleteStatement;
}

DomainObject* DocumentMapper::doLoad(const Identifier& _id, const QSqlRecord& _record)
{
    const QUuid uuid = _record.value("uuid").toString();
    const auto type = static_cast<DocumentObjectType>(_record.value("type").toInt());
    const auto content = _record.value("content").toByteArray();

    return Domain::ObjectsBuilder::createDocument(_id, uuid, type, content);
}

void DocumentMapper::doLoad(DomainObject* _object, const QSqlRecord& _record)
{
    auto documentObject = dynamic_cast<DocumentObject*>(_object);
    if (documentObject == nullptr) {
        return;
    }

    const QUuid uuid = _record.value("uuid").toString();
    documentObject->setUuid(uuid);

    const DocumentObjectType type = static_cast<DocumentObjectType>(_record.value("type").toInt());
    documentObject->setType(type);

    const auto content = _record.value("content").toByteArray();
    documentObject->setContent(content);
}

} // namespace DataMappingLayer
