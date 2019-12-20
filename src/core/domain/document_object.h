#pragma once

#include "domain_object.h"

#include <QUuid>


namespace Domain
{

/**
 * @brief Типы элементов
 */
enum class DocumentObjectType {
    Undefined = 0,

    //
    // Служебные для обслуживания работы с проектом
    //

    Structure = 1,

    //
    // Собственно данные
    //

    Project = 101,

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
 * @brief Класс данных документа
 */
class DocumentObject : public DomainObject
{
public:
    /**
     * @brief Дефолтная структура проекта
     */
    static const QByteArray kDefaultStructureContent;

public:
    /**
     * @brief Уникальный идентификатор
     */
    QUuid uuid() const;
    void setUuid(const QUuid& _uuid);

    /**
     * @brief Тип документа
     */
    DocumentObjectType type() const;
    void setType(DocumentObjectType _type);

    /**
     * @brief Содержимое документа
     */
    QByteArray content() const;
    void setContent(const QByteArray& _content);

private:
    DocumentObject(const Identifier& _id, const QUuid& _uuid, DocumentObjectType _type, const QByteArray& _content);
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
