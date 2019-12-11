#include "item_object.h"

#include <QDomDocument>
#include <QPixmap>


namespace Domain
{

ItemObject* ItemObject::parent() const
{
    return m_parent;
}

void ItemObject::setParent(ItemObject* _parent)
{
    if (m_parent == _parent) {
        return;
    }

    m_parent = _parent;
    markChangesNotStored();
}

ItemObjectType ItemObject::type() const
{
    return m_type;
}

void ItemObject::setType(ItemObjectType _type)
{
    if (m_type == _type) {
        return;
    }

    m_type = _type;
    markChangesNotStored();
}

QString ItemObject::name() const
{
    return m_name;
}

void ItemObject::setName(const QString& _name)
{
    if (m_name == _name) {
        return;
    }

    m_name = _name;
    markChangesNotStored();
}

QDomDocument ItemObject::content() const
{
    QDomDocument document;
    document.setContent(m_content);
    return document;
}

QString ItemObject::contentXml() const
{
    return m_content;
}

void ItemObject::setContent(const QDomDocument& _content)
{
    setContentXml(_content.toString());
}

void ItemObject::setContentXml(const QString& _content)
{
    if (m_content == _content) {
        return;
    }

    m_content = _content;
    markChangesNotStored();
}

QPixmap ItemObject::image() const
{
    if (m_imageWrapper == nullptr) {
        return {};
    }

    return m_imageWrapper->image(this);
}

void ItemObject::setImage(const QPixmap& _image)
{
    if (m_imageWrapper == nullptr) {
        return;
    }

    m_imageWrapper->setImage(_image, this);
    markChangesNotStored();
}

QColor ItemObject::color() const
{
    return m_color;
}

void ItemObject::setColor(const QColor& _color)
{
    if (m_color != _color) {
        m_color = _color;

        markChangesNotStored();
    }
}

int ItemObject::sortOrder() const
{
    return m_sortOrder;
}

void ItemObject::setSortOrder(int _sortOrder)
{
    if (m_sortOrder == _sortOrder) {
        return;
    }

    m_sortOrder = _sortOrder;
    markChangesNotStored();
}

bool ItemObject::isDeleted() const
{
    return m_isDeleted;
}

void ItemObject::setDeleted(bool _deleted)
{
    if (m_isDeleted == _deleted) {
        return;
    }

    m_isDeleted = _deleted;
    markChangesNotStored();
}

QVariant ItemObject::data(int _role) const
{
    switch (_role) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            return name();
        }

        case Qt::DecorationRole: {
            return image();
        }

        default: {
            return {};
        }
    }
}

void ItemObject::setImageWrapper(AbstractImageWrapper* _imageWrapper)
{
    if (m_imageWrapper != _imageWrapper) {
        m_imageWrapper = _imageWrapper;
    }
}

ItemObject::ItemObject(const Identifier& _id, ItemObject* _parent, ItemObjectType _type,
    const QString& _name, const QString& _description, const QColor& _color, int _sortOrder,
    bool _isDeleted, AbstractImageWrapper* _imageWrapper)
    : DomainObject(_id),
      m_parent(_parent),
      m_type(_type),
      m_name(_name),
      m_content(_description),
      m_color(_color),
      m_sortOrder(_sortOrder),
      m_isDeleted(_isDeleted),
      m_imageWrapper(_imageWrapper)
{
}


// ****


ItemObjectsTable::ItemObjectsTable(QObject* _parent) :
    DomainObjectsItemModel(_parent)
{
}

int ItemObjectsTable::columnCount(const QModelIndex&) const
{
    const int kColumnCount = 1;
    return kColumnCount;
}

QVariant ItemObjectsTable::data(const QModelIndex& _index, int _role) const
{
    const DomainObject *domainObject = domainObjects().value(_index.row());
    const ItemObject* item = dynamic_cast<const ItemObject*>(domainObject);
    return item->data(_role);
}

bool ItemObjectsTable::moveRows(const QModelIndex& _sourceParent, int _sourceRow, int _count,
    const QModelIndex& _destinationParent, int _destinationRow)
{
    //
    // Переносим элементы
    //
    const bool isMoveSucceed =
            DomainObjectsItemModel::moveRows(
                _sourceParent, _sourceRow, _count, _destinationParent, _destinationRow);
    if (!isMoveSucceed) {
        return false;
    }

    //
    // Если перенос удался, обновим порядок сортировки для всех смещаемых элементов
    //
    for (int movedItemRow = std::min(_sourceRow, _destinationRow);
         movedItemRow <= std::max(_sourceRow, _destinationRow);
         ++movedItemRow) {
        DomainObject* movedObject = itemForIndex(index(movedItemRow, 0));
        ItemObject* movedItem = dynamic_cast<ItemObject*>(movedObject);
        if (movedItem != nullptr) {
            movedItem->setSortOrder(movedItemRow);
        }
    }
    return true;
}

} // namespace Domain
