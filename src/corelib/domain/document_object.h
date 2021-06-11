#pragma once

#include "domain_object.h"

#include <QPixmap>
#include <QUuid>

namespace Domain {

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
    // Сырые данные
    //

    ImageData = 101,

    //
    // Собственно данные
    //

    Project = 10000,
    RecycleBin = 10001,
    //
    Screenplay = 10100,
    ScreenplayTitlePage = 10101,
    ScreenplaySynopsis = 10102,
    ScreenplayTreatment = 10103,
    ScreenplayText = 10104,
    ScreenplayDictionaries = 10105,
    ScreenplayStatistics = 10106,
    ScreenplaySeries = 10110,
    //
    ComicBook = 10200,
    //
    Novel = 10300,
    //
    GameScript = 10400,
    //
    Plots = 20000,
    Plot = 20001,
    //
    Characters = 30000,
    Character = 30001,
    //
    Locations = 40000,
    Location = 40001,
    //
    Worlds = 50000,
    World = 50001,
    //
    Folder = 100001,
    Text = 100002,
    MindMap = 100003,
    Image = 100004,
    Link = 100005,

    /**
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
    **/
};

/**
 * @brief Определим метод для возможности использовать типы объектов в виде ключей в словарях
 */
inline uint qHash(DocumentObjectType _type)
{
    return ::qHash(static_cast<int>(_type));
}

/**
 * @brief Получить майм-тип по типу объекта
 */
QByteArray CORE_LIBRARY_EXPORT mimeTypeFor(DocumentObjectType _type);

/**
 * @brief Получить тип объекта по майм-типу
 */
DocumentObjectType CORE_LIBRARY_EXPORT typeFor(const QByteArray& _mime);

/**
 * @brief Получить иконку по типу объекта
 */
QString CORE_LIBRARY_EXPORT iconForType(DocumentObjectType _type);

/**
 * @brief Изображение связанное с документом
 */
struct DocumentImage {
    QUuid uuid;
    QPixmap image;
};

/**
 * @brief Класс данных документа
 */
class CORE_LIBRARY_EXPORT DocumentObject : public DomainObject
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
