#pragma once

#include "domain_object.h"

#include <QDateTime>
#include <QUuid>


namespace Domain {

/**
 * @brief Класс изменения документа
 */
class CORE_LIBRARY_EXPORT DocumentChangeObject : public DomainObject
{
public:
    /**
     * @brief Получить идентификатор документа
     */
    const QUuid& documentUuid() const;
    void setDocumentUuid(const QUuid& _uuid);

    /**
     * @brief Получить уникальный идентификатор изменения
     */
    const QUuid& uuid() const;
    void setUuid(const QUuid& _uuid);

    /**
     * @brief Патч отменяющий изменение
     */
    const QByteArray& undoPatch() const;
    void setUndoPatch(const QByteArray& _patch);

    /**
     * @brief Патч применяющий изменение
     */
    const QByteArray& redoPatch() const;
    void setRedoPatch(const QByteArray& _patch);

    /**
     * @brief Дата и время создания изменения
     */
    const QDateTime& dateTime() const;
    void setDateTime(const QDateTime& _dateTime);

    /**
     * @brief Имя пользователя создавшего изменение
     */
    const QString& userName() const;
    void setUserName(const QString& _name);

    /**
     * @brief Email пользователя создавшего изменение
     */
    const QString& userEmail() const;
    void setUserEmail(const QString& _email);

    /**
     * @brief Синхронизировано ли изменение
     */
    bool isSynced() const;
    void setSynced(bool _synced);

private:
    explicit DocumentChangeObject(const Identifier& _id, const QUuid& _documentUuid,
                                  const QUuid& _uuid, const QByteArray& _undoPatch,
                                  const QByteArray& _redoPatch, const QDateTime& _dateTime,
                                  const QString& _userEmail, const QString& _userName,
                                  bool _isSynced);
    friend class ObjectsBuilder;

    QUuid m_documentUuid;
    QUuid m_uuid;
    QByteArray m_undoPatch;
    QByteArray m_redoPatch;
    QDateTime m_dateTime;
    QString m_userName;
    QString m_userEmail;
    bool m_isSynced = false;
};

} // namespace Domain
