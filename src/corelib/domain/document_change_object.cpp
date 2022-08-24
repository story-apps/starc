#include "document_change_object.h"


namespace Domain {

const QUuid& DocumentChangeObject::documentUuid() const
{
    return m_documentUuid;
}

void DocumentChangeObject::setDocumentUuid(const QUuid& _uuid)
{
    if (m_documentUuid == _uuid) {
        return;
    }

    m_documentUuid = _uuid;
    markChangesNotStored();
}

const QUuid& DocumentChangeObject::uuid() const
{
    return m_uuid;
}

void DocumentChangeObject::setUuid(const QUuid& _uuid)
{
    if (m_uuid == _uuid) {
        m_uuid = _uuid;
    }

    m_uuid = _uuid;
    markChangesNotStored();
}

const QByteArray& DocumentChangeObject::undoPatch() const
{
    return m_undoPatch;
}

void DocumentChangeObject::setUndoPatch(const QByteArray& _patch)
{
    //
    // NOTE: Тут специально нет проверки, т.к. данные могут быть очень большими
    //

    m_undoPatch = _patch;
    markChangesNotStored();
}

const QByteArray& DocumentChangeObject::redoPatch() const
{
    return m_redoPatch;
}

void DocumentChangeObject::setRedoPatch(const QByteArray& _patch)
{
    //
    // NOTE: Тут специально нет проверки, т.к. данные могут быть очень большими
    //

    m_redoPatch = _patch;
    markChangesNotStored();
}

const QDateTime& DocumentChangeObject::dateTime() const
{
    return m_dateTime;
}

void DocumentChangeObject::setDateTime(const QDateTime& _dateTime)
{
    if (m_dateTime == _dateTime) {
        return;
    }

    m_dateTime = _dateTime;
    markChangesNotStored();
}

const QString& DocumentChangeObject::userName() const
{
    return m_userName;
}

void DocumentChangeObject::setUserName(const QString& _name)
{
    if (m_userName == _name) {
        return;
    }

    m_userName = _name;
    markChangesNotStored();
}

const QString& DocumentChangeObject::userEmail() const
{
    return m_userEmail;
}

void DocumentChangeObject::setUserEmail(const QString& _email)
{
    if (m_userEmail == _email) {
        return;
    }

    m_userEmail = _email;
    markChangesNotStored();
}

bool DocumentChangeObject::isSynced() const
{
    return m_isSynced;
}

void DocumentChangeObject::setSynced(bool _synced)
{
    if (m_isSynced == _synced) {
        return;
    }

    m_isSynced = _synced;
    markChangesNotStored();
}

DocumentChangeObject::DocumentChangeObject(const Identifier& _id, const QUuid& _documentUuid,
                                           const QUuid& _uuid, const QByteArray& _undoPatch,
                                           const QByteArray& _redoPatch, const QDateTime& _dateTime,
                                           const QString& _userName, const QString& _userEmail,
                                           bool _isSynced)
    : DomainObject(_id)
    , m_documentUuid(_documentUuid)
    , m_uuid(_uuid)
    , m_undoPatch(_undoPatch)
    , m_redoPatch(_redoPatch)
    , m_dateTime(_dateTime)
    , m_userName(_userName)
    , m_userEmail(_userEmail)
    , m_isSynced(_isSynced)
{
}

} // namespace Domain
