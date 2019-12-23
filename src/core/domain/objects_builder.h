#pragma once

class QByteArray;
class QDateTime;
class QString;
class QUuid;


namespace Domain
{

class DocumentChangeObject;
class DocumentObject;
enum class DocumentObjectType;
class Identifier;

/**
 * @brief Фабрика для создания элементов данных
 */
class ObjectsBuilder
{
public:
    /**
     * @brief Создать элемент
     */
    static DocumentObject* createDocument(const Identifier& _id, const QUuid& _uuid,
        DocumentObjectType _type, const QByteArray& _content);

    /**
     * @brief Создать изменение
     */
    static DocumentChangeObject* createDocumentChange(const Identifier& _id,
        const QUuid& _documentUuid, const QUuid& _uuid, const QByteArray& _undoPatch,
        const QByteArray& _redoPatch, const QDateTime& _dateTime, const QString& _userName,
        const QString& _userEmail);
};

} // namespace Domain
