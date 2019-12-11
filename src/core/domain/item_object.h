#pragma once

#include "domain_object.h"

#include <QColor>

class QDomDocument;


namespace Domain
{

/**
 * @brief Типы элементов
 */
enum class ItemObjectType {
    Undefined = 0,
    Project = 1,

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
};

/**
 * @brief Класс элемента с данными
 */
class ItemObject : public DomainObject
{
public:

    /**
     * @brief Получить родителя
     */
    ItemObject* parent() const;

    /**
     * @brief Установить родителя
     */
    void setParent(ItemObject* _parent);

    /**
     * @brief Получить тип
     */
    ItemObjectType type() const;

    /**
     * @brief Установить тип
     */
    void setType(ItemObjectType _type);

    /**
     * @brief Получить название
     */
    QString name() const;

    /**
     * @brief Установить название
     */
    void setName(const QString& _name);

    /**
     * @brief Получить содержимое элемента
     */
    QDomDocument content() const;
    QString contentXml() const;

    /**
     * @brief Установить содержимое элемента
     */
    void setContent(const QDomDocument& _content);
    void setContentXml(const QString& _content);

    /**
     * @brief Получить изображение
     */
    QPixmap image() const;

    /**
     * @brief Установить изображение
     */
    void setImage(const QPixmap& _image);

    /**
     * @brief Получить цвет элемента
     */
    QColor color() const;

    /**
     * @brief Установить цвет
     */
    void setColor(const QColor& _color);

    /**
     * @brief Получить позицию сортировки
     */
    int sortOrder() const;

    /**
     * @brief Установить позицию сортировки
     */
    void setSortOrder(int _sortOrder);

    /**
     * @brief Удалён ли объект
     */
    bool isDeleted() const;

    /**
     * @brief Пометить объект удалённым/не удалённым
     */
    void setDeleted(bool _deleted);

    /**
     * @brief Получить данные по роли из модели
     */
    virtual QVariant data(int _role) const;

    /**
     * @brief Установить загрузчик изображений
     */
    void setImageWrapper(AbstractImageWrapper* _imageWrapper);

protected:
    ItemObject(const Identifier& _id, ItemObject* _parent, ItemObjectType _type, const QString& _name,
        const QString& _description, const QColor& _color, int _sortOrder, bool _isDeleted,
        AbstractImageWrapper* _imageWrapper);

    friend class ItemsBuilder;

private:
    /**
     * @brief Родительский элемент
     */
    ItemObject* m_parent = nullptr;

    /**
     * @brief Тип
     */
    ItemObjectType m_type = ItemObjectType::Undefined;

    /**
     * @brief Название
     */
    QString m_name;

    /**
     * @brief Содержимое объекта
     * @note Xml-документ, в котором находятся все необходимые данные об объекте
     */
    QString m_content;

    /**
     * @brief Цвет
     */
    QColor m_color;

    /**
     * @brief Порядок сортировки
     */
    int m_sortOrder = 0;

    /**
     * @brief Удалён ли объект
     */
    bool m_isDeleted = false;

    /**
     * @brief Загрузчик фотографий
     */
    AbstractImageWrapper* m_imageWrapper = nullptr;
};


// ****

class ItemObjectsTable : public DomainObjectsItemModel
{
    Q_OBJECT

public:
    explicit ItemObjectsTable(QObject* _parent = nullptr);

public:
    int columnCount(const QModelIndex&) const override;
    QVariant data(const QModelIndex& _index, int _role) const override;
    bool moveRows(const QModelIndex& _sourceParent, int _sourceRow, int _count,
        const QModelIndex& _destinationParent, int _destinationRow) override;
};

} // namespace Domain
