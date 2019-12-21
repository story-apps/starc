#pragma once

#include "domain_object.h"

#include <QDateTime>
#include <QUuid>


namespace Domain
{

/**
 * @brief Класс изменения документа
 */
class DocumentChangeObject : public DomainObject
{
public:
    /**
     * @brief Получить идентификатор документа
     */
    const QUuid& documentUuid() const;
    void setDocumentUuid(const QUuid& _uuid);

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
     * @brief Email пользователя создавшего изменение
     */
    const QString& userEmail() const;
    void setUserEmail(const QString& _email);

    /**
     * @brief Имя пользователя создавшего изменение
     */
    const QString& userName() const;
    void setUserName(const QString& _name);

private:
    explicit DocumentChangeObject(const Identifier& _id, const QUuid& _documentUuid,
        const QByteArray& _undoPatch, const QByteArray& _redoPatch, const QDateTime& _dateTime,
        const QString& _userEmail, const QString& _userName);
    friend class ObjectsBuilder;

    QUuid m_documentUuid;
    QByteArray m_undoPatch;
    QByteArray m_redoPatch;
    QDateTime m_dateTime;
    QString m_userEmail;
    QString m_userName;
};

} // namespace Domain

