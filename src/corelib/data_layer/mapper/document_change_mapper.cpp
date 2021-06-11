#include "document_change_mapper.h"

#include <data_layer/database.h>
#include <domain/document_change_object.h>
#include <domain/objects_builder.h>

#include <QSqlRecord>

using Domain::DocumentChangeObject;
using Domain::DomainObject;
using Domain::Identifier;


namespace DataMappingLayer {

namespace {
const QString kColumns
    = " id, fk_document_uuid, uuid, undo_patch, redo_patch, date_time, user_name, user_email ";
const QString kTableName = " documents_changes ";
const QString kDateTimeFormat = "yyyy-MM-dd hh:mm:ss:zzz";
QString uuidFilter(const QUuid& _uuid)
{
    return QString(" WHERE uuid = '%1' ").arg(_uuid.toString());
}
QString documentFilter(const QUuid& _documentUuid, int _changeIndex)
{
    //
    // Игнорируем самое первое изменнеие документа, т.к. это добавление стандартной разметки
    // элемента в пустой документ
    //
    return QString(" WHERE fk_document_uuid = '%1'"
                   " AND id not in (SELECT id FROM %3 WHERE fk_document_uuid = '%1' ORDER BY id "
                   "ASC LIMIT 0, 1) "
                   " ORDER BY id DESC LIMIT %2, 1")
        .arg(_documentUuid.toString())
        .arg(_changeIndex)
        .arg(kTableName);
}
} // namespace

DocumentChangeObject* DocumentChangeMapper::find(const Domain::Identifier& _id)
{
    return dynamic_cast<DocumentChangeObject*>(abstractFind(_id));
}

DocumentChangeObject* DocumentChangeMapper::find(const QUuid& _uuid)
{
    const auto domainObjects = abstractFind(uuidFilter(_uuid));
    if (domainObjects.isEmpty()) {
        return nullptr;
    }

    return dynamic_cast<DocumentChangeObject*>(domainObjects.first());
}

Domain::DocumentChangeObject* DocumentChangeMapper::find(const QUuid& _documentUuid,
                                                         int _changeIndex)
{
    const auto domainObjects = abstractFind(documentFilter(_documentUuid, _changeIndex));
    if (domainObjects.isEmpty()) {
        return nullptr;
    }

    return dynamic_cast<DocumentChangeObject*>(domainObjects.first());
}

void DocumentChangeMapper::insert(DocumentChangeObject* _object)
{
    abstractInsert(_object);
}

bool DocumentChangeMapper::update(DocumentChangeObject* _object)
{
    return abstractUpdate(_object);
}

void DocumentChangeMapper::remove(DocumentChangeObject* _object)
{
    abstractDelete(_object);
}

QString DocumentChangeMapper::findStatement(const Domain::Identifier& _id) const
{
    QString findStatement
        = QString("SELECT " + kColumns + " FROM " + kTableName + " WHERE id = %1 ")
              .arg(_id.value());
    return findStatement;
}

QString DocumentChangeMapper::findAllStatement() const
{
    return "SELECT " + kColumns + " FROM  " + kTableName;
}

QString DocumentChangeMapper::findLastOneStatement() const
{
    return findAllStatement()
        + QString("WHERE id IN (SELECT id FROM %1 ORDER BY id DESC LIMIT %2)")
              .arg(kTableName)
              .arg(1);
}

QString DocumentChangeMapper::insertStatement(Domain::DomainObject* _object,
                                              QVariantList& _insertValues) const
{
    const QString insertStatement = QString("INSERT INTO " + kTableName + " (" + kColumns
                                            + ") "
                                              " VALUES(?, ?, ?, ?, ?, ?, ?, ?) ");

    const auto documentChangeObject = dynamic_cast<DocumentChangeObject*>(_object);
    _insertValues.clear();
    _insertValues.append(documentChangeObject->id().value());
    _insertValues.append(documentChangeObject->documentUuid().toString());
    _insertValues.append(documentChangeObject->uuid().toString());
    _insertValues.append(qCompress(documentChangeObject->undoPatch()));
    _insertValues.append(qCompress(documentChangeObject->redoPatch()));
    _insertValues.append(documentChangeObject->dateTime().toString(kDateTimeFormat));
    _insertValues.append(documentChangeObject->userName());
    _insertValues.append(documentChangeObject->userEmail());

    return insertStatement;
}

QString DocumentChangeMapper::updateStatement(Domain::DomainObject* _object,
                                              QVariantList& _updateValues) const
{
    const QString updateStatement = QString("UPDATE " + kTableName
                                            + " SET fk_document_uuid = ?, "
                                              " uuid = ?, "
                                              " undo_patch = ?, "
                                              " redo_patch = ?, "
                                              " date_time = ?, "
                                              " user_name = ?, "
                                              " user_email = ? "
                                              " WHERE id = ? ");

    const auto documentChangeObject = dynamic_cast<DocumentChangeObject*>(_object);
    _updateValues.clear();
    _updateValues.append(documentChangeObject->documentUuid().toString());
    _updateValues.append(documentChangeObject->uuid().toString());
    _updateValues.append(qCompress(documentChangeObject->undoPatch()));
    _updateValues.append(qCompress(documentChangeObject->redoPatch()));
    _updateValues.append(documentChangeObject->dateTime().toString(kDateTimeFormat));
    _updateValues.append(documentChangeObject->userName());
    _updateValues.append(documentChangeObject->userEmail());
    _updateValues.append(documentChangeObject->id().value());

    return updateStatement;
}

QString DocumentChangeMapper::deleteStatement(Domain::DomainObject* _object,
                                              QVariantList& _deleteValues) const
{
    QString deleteStatement = "DELETE FROM " + kTableName + " WHERE id = ?";

    _deleteValues.clear();
    _deleteValues.append(_object->id().value());

    return deleteStatement;
}

Domain::DomainObject* DocumentChangeMapper::doLoad(const Domain::Identifier& _id,
                                                   const QSqlRecord& _record)
{
    const QUuid documentUuid = _record.value("fk_document_uuid").toString();
    const QUuid uuid = _record.value("uuid").toString();
    const auto undoPatch = qUncompress(_record.value("undo_patch").toByteArray());
    const auto redoPatch = qUncompress(_record.value("redo_patch").toByteArray());
    const auto dateTime
        = QDateTime::fromString(_record.value("date_time").toString(), kDateTimeFormat);
    const auto userName = _record.value("user_name").toString();
    const auto userEmail = _record.value("user_email").toString();

    return Domain::ObjectsBuilder::createDocumentChange(_id, documentUuid, uuid, undoPatch,
                                                        redoPatch, dateTime, userName, userEmail);
}

void DocumentChangeMapper::doLoad(Domain::DomainObject* _object, const QSqlRecord& _record)
{
    auto documentChangeObject = dynamic_cast<DocumentChangeObject*>(_object);
    if (documentChangeObject == nullptr) {
        return;
    }

    const QUuid documentUuid = _record.value("fk_document_uuid").toString();
    documentChangeObject->setDocumentUuid(documentUuid);

    const QUuid uuid = _record.value("uuid").toString();
    documentChangeObject->setUuid(uuid);

    const auto undoPatch = qUncompress(_record.value("undo_patch").toByteArray());
    documentChangeObject->setUndoPatch(undoPatch);

    const auto redoPatch = qUncompress(_record.value("redo_patch").toByteArray());
    documentChangeObject->setRedoPatch(redoPatch);

    const auto dateTime
        = QDateTime::fromString(_record.value("date_time").toString(), kDateTimeFormat);
    documentChangeObject->setDateTime(dateTime);

    const auto userName = _record.value("user_name").toString();
    documentChangeObject->setUserName(userName);

    const auto userEmail = _record.value("user_email").toString();
    documentChangeObject->setUserEmail(userEmail);
}

} // namespace DataMappingLayer
