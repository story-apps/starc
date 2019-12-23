#pragma once

#include "domain_object.h"

#include <QUuid>


namespace Domain
{

/**
 * @brief Типы элементов
 */
enum class DocumentObjectType {
    Undefined,

    //
    // Служебные для обслуживания работы с проектом
    //

    Structure,

    //
    // Собственно данные
    //

    Project,
    Screenplay,
    ScreenplayTitlePage,
    ScreenplayLogline,
    ScreenplaySynopsis,
    ScreenplayOutline,
    ScreenplayText,

/**
//    Text,
//    Image,
//    Link,
//    MindMap,
//    Screenplay,
//    ScreenplayTitlePage,
//    ScreenplayText,
//    Series,
//    Report,
//    Plot,
//    Characters,
//    Character,
//    Locations,
//    Location,

//    Character = 101,
//    Location = 201,
//    Item = 301,
//    World = 401,
//    WorldRace = 402,
//    WorldFlora = 403,
//    WorldAnimal = 404,
//    WorldNaturalResource = 405,
//    WorldClimate = 406,
//    WorldReligionAndBelief = 407,
//    WorldEthicsAndValues = 408,
//    WorldLanguage = 409,
//    WorldClassCasteSystem = 410,
//    WorldMagicType = 411,
//    Story = 501,
//    StoryPlot = 502,
//    StoryScene = 503
**/
};

/**
 * @brief Определим метод для возможности использовать типы объектов в виде ключей в словарях
 */
inline uint qHash(DocumentObjectType _type)
{
    return qHash(static_cast<int>(_type));
}

/**
 * @brief Получить майм-тип по типу объекта
 */
QByteArray mimeTypeFor(DocumentObjectType _type);

/**
 * @brief Получить тип объекта по майм-типу
 */
DocumentObjectType typeFor(const QByteArray& _mime);

/**
 * @brief Получить иконку по типу объекта
 */
QString iconForType(DocumentObjectType _type);

/**
 * @brief Класс данных документа
 */
class DocumentObject : public DomainObject
{
public:
    /**
     * @brief Уникальный идентификатор
     */
    const QUuid& uuid() const;
    void setUuid(const QUuid& _uuid);

    /**
     * @brief Тип документа
     */
    DocumentObjectType type() const;
    void setType(DocumentObjectType _type);

    /**
     * @brief Содержимое документа
     */
    const QByteArray& content() const;
    void setContent(const QByteArray& _content);

private:
    explicit DocumentObject(const Identifier& _id, const QUuid& _uuid, DocumentObjectType _type,
                            const QByteArray& _content);
    friend class ObjectsBuilder;

    /**
     * @brief Уникальный идентификатор
     */
    QUuid m_uuid;

    /**
     * @brief Тип
     */
    DocumentObjectType m_type = DocumentObjectType::Undefined;

    /**
     * @brief Содержимое объекта
     */
    QByteArray m_content;
};

} // namespace Domain
