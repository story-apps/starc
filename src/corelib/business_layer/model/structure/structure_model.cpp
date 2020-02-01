#include "structure_model.h"

#include "structure_model_item.h"

#include <domain/document_object.h>

#include <utils/helpers/text_helper.h>

#include <QColor>
#include <QDataStream>
#include <QDomDocument>
#include <QMimeData>


namespace BusinessLayer
{

namespace {
    const char* kMimeType = "application/x-starc/document";
}

class StructureModel::Implementation
{
public:
    Implementation();

    /**
     * @brief Построить модель структуры из xml хранящегося в документе
     */
    void buildModel(Domain::DocumentObject* _structure);

    /**
     * @brief Сформировать xml из данных модели
     */
    QByteArray toXml(Domain::DocumentObject* _structure) const;


    /**
     * @brief Корневой элемент дерева
     */
    StructureModelItem* rootItem = nullptr;

    /**
     * @brief Последние положенные в майм элементы
     */
    mutable QVector<StructureModelItem*> m_lastMimeItems;
};

StructureModel::Implementation::Implementation()
    : rootItem(new StructureModelItem({}, Domain::DocumentObjectType::Undefined, {}, {}))
{
}

void StructureModel::Implementation::buildModel(Domain::DocumentObject* _structure)
{
    if (_structure == nullptr) {
        return;
    }

    std::function<void(const QDomElement&, StructureModelItem*)> buildItem;
    buildItem = [&buildItem] (const QDomElement& _node, StructureModelItem* _parent) {
        auto item = new StructureModelItem(_node.attribute("uuid"),
                                           Domain::typeFor(_node.attribute("type").toUtf8()),
                                           TextHelper::fromHtmlEscaped(_node.attribute("name")),
                                           _node.attribute("color"));
        _parent->appendItem(item);

        auto child = _node.firstChildElement();
        while (!child.isNull()) {
            buildItem(child, item);
            child = child.nextSiblingElement();
        }
    };

    QDomDocument domDocument;
    domDocument.setContent(_structure->content());
    auto documentNode = domDocument.firstChildElement("document");
    auto itemNode = documentNode.firstChildElement();
    while (!itemNode.isNull()) {
        buildItem(itemNode, rootItem);
        itemNode = itemNode.nextSiblingElement();
    }
}

QByteArray StructureModel::Implementation::toXml(Domain::DocumentObject* _structure) const
{
    if (_structure == nullptr) {
        return {};
    }

    //
    // TODO: Продумать, как оптимизировать формирование xml
    //

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(_structure->type()) + "\" version=\"1.0\">\n";
    std::function<void(StructureModelItem*)> writeItemXml;
    writeItemXml = [&xml, &writeItemXml] (StructureModelItem* _item) {
        xml += "<item ";
        xml += " uuid=\"" + _item->uuid().toString() + "\" ";
        xml += " type=\"" + Domain::mimeTypeFor(_item->type()) + "\" ";
        xml += " name=\"" + TextHelper::toHtmlEscaped(_item->name()) + "\" ";
        xml += " color=\"" + _item->color().name() + "\" ";
        if (!_item->hasChildren()) {
            xml += "/>\n";
            return;
        }

        xml += ">\n";
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            writeItemXml(_item->childAt(childIndex));
        }
        xml += "</item>\n";
    };
    for (int childIndex = 0; childIndex < rootItem->childCount(); ++childIndex) {
        writeItemXml(rootItem->childAt(childIndex));
    }
    xml += "</document>";
    return xml;
}


// ****


StructureModel::StructureModel(QObject* _parent)
    : AbstractModel({}, _parent),
      d(new Implementation)
{
    connect(this, &StructureModel::rowsInserted, this, &StructureModel::updateDocumentContent);
    connect(this, &StructureModel::rowsRemoved, this, &StructureModel::updateDocumentContent);
    connect(this, &StructureModel::rowsMoved, this, &StructureModel::updateDocumentContent);
    connect(this, &StructureModel::dataChanged, this, &StructureModel::updateDocumentContent);
}

StructureModel::~StructureModel() = default;

void StructureModel::addDocument(Domain::DocumentObjectType _type, const QString& _name, const QModelIndex& _parent)
{
    using namespace Domain;

    auto createItem = [] (DocumentObjectType _type, const QString& _name) {
        auto uuid = QUuid::createUuid();
        return new StructureModelItem(uuid, _type, _name, {});
    };

    auto parentItem = itemForIndex(_parent);

    switch (_type) {
        case DocumentObjectType::Project: {
            appendItem(createItem(_type, tr("Project")), parentItem);
            break;
        }

        case DocumentObjectType::RecycleBin: {
            appendItem(createItem(_type, tr("Recicle bin")), parentItem);
            break;
        }

        case DocumentObjectType::Screenplay: {
            auto screenplayItem = createItem(DocumentObjectType::Screenplay, !_name.isEmpty() ? _name : tr("Screenplay"));
            appendItem(screenplayItem, parentItem);
            appendItem(createItem(DocumentObjectType::ScreenplayTitlePage, tr("Title page")), screenplayItem);
            appendItem(createItem(DocumentObjectType::ScreenplaySynopsis, tr("Synopsis")), screenplayItem);
            appendItem(createItem(DocumentObjectType::ScreenplayOutline, tr("Outline")), screenplayItem);
            appendItem(createItem(DocumentObjectType::ScreenplayText, tr("Screenplay")), screenplayItem);
            break;
        }

        default: {
            Q_ASSERT(false);
            break;
        }
    }
}

void StructureModel::prependItem(StructureModelItem* _item, StructureModelItem* _parentItem)
{
    if (_item == nullptr) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem;
    }

    if (_parentItem->hasChild(_item)) {
        return;
    }

    const QModelIndex parentIndex = indexForItem(_parentItem);
    const int itemRowIndex = 0; // т.к. в самое начало
    beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
    _parentItem->prependItem(_item);
    endInsertRows();
}

void StructureModel::appendItem(StructureModelItem* _item, StructureModelItem* _parentItem)
{
    if (_item == nullptr) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem;
    }

    if (_parentItem->hasChild(_item)) {
        return;
    }

    const QModelIndex parentIndex = indexForItem(_parentItem);
    const int itemRowIndex = _parentItem->childCount();
    beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
    _parentItem->insertItem(itemRowIndex, _item);
    endInsertRows();

    emit documentAdded(_item->uuid(), _item->type(), _item->name());
}

void StructureModel::insertItem(StructureModelItem* _item, StructureModelItem* _afterSiblingItem)
{
    if (_item == nullptr
        || _afterSiblingItem == nullptr
        || _afterSiblingItem->parent() == nullptr) {
        return;
    }

    auto parent = _afterSiblingItem->parent();

    if (parent->hasChild(_item)) {
        return;
    }

    const QModelIndex parentIndex = indexForItem(parent);
    const int itemRowIndex = parent->rowOfChild(_afterSiblingItem) + 1;
    beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
    parent->insertItem(itemRowIndex, _item);
    endInsertRows();
}

void StructureModel::moveItem(StructureModelItem* _item, StructureModelItem* _parentItem)
{
    if (_item == nullptr
        || _parentItem == nullptr) {
        return;
    }

    auto sourceParent = _item->parent();
    if (sourceParent == nullptr) {
        return;
    }

    const auto itemIndex = indexForItem(_item);
    const auto sourceParentIndex = indexForItem(sourceParent);
    const auto destinationIndex = indexForItem(_parentItem);
    beginMoveRows(sourceParentIndex, itemIndex.row(), itemIndex.row(), destinationIndex, _parentItem->childCount());
    sourceParent->takeItem(_item);
    _parentItem->appendItem(_item);
    endMoveRows();
}

void StructureModel::removeItem(StructureModelItem* _item)
{
    if (_item == nullptr
        || _item->parent() == nullptr) {
        return;
    }

    auto itemParent = _item->parent();
    const QModelIndex itemParentIndex = indexForItem(_item).parent();
    const int itemRowIndex = itemParent->rowOfChild(_item);
    beginRemoveRows(itemParentIndex, itemRowIndex, itemRowIndex);
    itemParent->removeItem(_item);
    endRemoveRows();
}

void StructureModel::updateItem(StructureModelItem* _item)
{
    if (_item == nullptr
        || _item->parent() == nullptr) {
        return;
    }

    const QModelIndex indexForUpdate = indexForItem(_item);
    emit dataChanged(indexForUpdate, indexForUpdate);
}

QModelIndex StructureModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    if (_row < 0
        || _row > rowCount(_parent)
        || _column < 0
        || _column > columnCount(_parent)
        || (_parent.isValid() && (_parent.column() != 0))) {
        return {};
    }

    auto parentItem = itemForIndex(_parent);
    Q_ASSERT(parentItem);

    auto indexItem = parentItem->childAt(_row);
    if (indexItem == nullptr) {
        return {};
    }

    return createIndex(_row, _column, indexItem);
}

QModelIndex StructureModel::parent(const QModelIndex& _child) const
{
    if (!_child.isValid()) {
        return {};
    }

    auto childItem = itemForIndex(_child);
    auto parentItem = childItem->parent();
    if (parentItem == nullptr
        || parentItem == d->rootItem) {
        return {};
    }

    auto grandParentItem = parentItem->parent();
    if (grandParentItem == nullptr) {
        return {};
    }

    const auto row = grandParentItem->rowOfChild(parentItem);
    return createIndex(row, 0, parentItem);
}

int StructureModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int StructureModel::rowCount(const QModelIndex& _parent) const
{
    if (_parent.isValid()
        && _parent.column() != 0) {
        return 0;
    }

    auto item = itemForIndex(_parent);
    if (item == nullptr) {
        return 0;
    }

    return item->childCount();
}

Qt::ItemFlags StructureModel::flags(const QModelIndex& _index) const
{
    const auto item = itemForIndex(_index);
    switch (item->type()) {
        case Domain::DocumentObjectType::Project:
        case Domain::DocumentObjectType::RecycleBin: {
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
        }

        case Domain::DocumentObjectType::Screenplay: {
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        }

        case Domain::DocumentObjectType::ScreenplayTitlePage:
        case Domain::DocumentObjectType::ScreenplaySynopsis:
        case Domain::DocumentObjectType::ScreenplayOutline:
        case Domain::DocumentObjectType::ScreenplayText: {
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        }

        default: {
            return Qt::ItemIsDropEnabled;
        }
    }
}

QVariant StructureModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    auto item = itemForIndex(_index);
    if (item == nullptr) {
        return {};
    }

    switch (_role) {
        case Qt::DisplayRole: {
            return item->name();
        }

        case Qt::DecorationRole: {
            return Domain::iconForType(item->type());
        }

        case Qt::BackgroundRole: {
            return item->color();
        }

        default: {
            return {};
        }
    }
}

bool StructureModel::canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row,
    int _column, const QModelIndex& _parent) const
{
    Q_UNUSED(_action)
    Q_UNUSED(_row)

    if (!_data->hasFormat(kMimeType)) {
        return false;
    }

    if (_column > 0) {
        return false;
    }

    //
    // Обработка конкретных случаев что куда можно бросать
    //
    const auto dropTarget = itemForIndex(_parent);
    switch (dropTarget->type()) {
        case Domain::DocumentObjectType::Screenplay: {
            return false;
        }

        //
        // Во всех остальных случаях можно
        //
        default: {
            return true;
        }
    }
}

bool StructureModel::dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row,
    int _column, const QModelIndex& _parent)
{
    if (!canDropMimeData(_data, _action, _row, _column, _parent)) {
        return false;
    }

    if (_action == Qt::IgnoreAction) {
        return true;
    }

    //
    // Проверяем, что перемещаются данные из модели
    //
    QByteArray encodedData = _data->data(kMimeType);
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    int row = 0;
    while (!stream.atEnd()) {
        QUuid itemUuid;
        stream >> itemUuid;
        if (itemUuid != d->m_lastMimeItems[row]->uuid()) {
            //
            // ... если это какие-то внешние данные, то ничего не делаем
            //
            return false;
        }

        ++row;
    }

    //
    // Если с данными всё окей, то перемещаем все элементы по очереди
    //

    auto parentItem = itemForIndex(_parent);

    //
    // Добавляем после всех элементов выбранного
    //
    if (_row == -1
        || _row == parentItem->childCount()) {
        while (!d->m_lastMimeItems.isEmpty()) {
            auto item = d->m_lastMimeItems.takeFirst();
            auto itemIndex = indexForItem(item);

            if (item->parent() == parentItem
                && itemIndex.row() == (parentItem->childCount() - 1)) {
                continue;
            }

            emit beginMoveRows(itemIndex.parent(), itemIndex.row(), itemIndex.row(),
                               _parent, parentItem->childCount());
            item->parent()->takeItem(item);
            parentItem->appendItem(item);
            emit endMoveRows();
        }
    }
    //
    // Вставляем между элементами
    //
    else {
        const QModelIndex insertBeforeItemIndex = index(_row, _column, _parent);
        auto insertBeforeItem = itemForIndex(insertBeforeItemIndex);

        if (d->m_lastMimeItems.contains(insertBeforeItem)) {
            return false;
        }

        while (!d->m_lastMimeItems.isEmpty()) {
            auto item = d->m_lastMimeItems.takeFirst();
            auto itemIndex = indexForItem(item);

            //
            // Нет смысла перемещать элемент на то же самое место
            //
            if (itemIndex.parent() == insertBeforeItemIndex.parent()
                && (itemIndex.row() == insertBeforeItemIndex.row()
                    || itemIndex.row() == insertBeforeItemIndex.row() - 1)) {
                continue;
            }

            emit beginMoveRows(itemIndex.parent(), itemIndex.row(), itemIndex.row(),
                               insertBeforeItemIndex.parent(), _row);
            item->parent()->takeItem(item);
            parentItem->insertItem(parentItem->rowOfChild(insertBeforeItem), item);
            emit endMoveRows();
        }
    }

    return true;
}

QMimeData* StructureModel::mimeData(const QModelIndexList& _indexes) const
{
    d->m_lastMimeItems.clear();

    if (_indexes.isEmpty()) {
        return nullptr;
    }

    //
    // Формируем список элементов для перемещения
    //
    for (const QModelIndex& index : _indexes) {
        if (index.isValid()) {
            d->m_lastMimeItems << itemForIndex(index);
        }
    }
    //
    // ... и упорядочиваем его
    //
    std::sort(d->m_lastMimeItems.begin(), d->m_lastMimeItems.end(),
              [] (StructureModelItem* _lhs, StructureModelItem* _rhs) {
        //
        // Для элементов находящихся на одном уровне сравниваем их позиции
        //
        if (_lhs->parent() == _rhs->parent()) {
            return _lhs->parent()->rowOfChild(_lhs) < _rhs->parent()->rowOfChild(_rhs);
        }

        //
        // Для разноуровневых элементов определяем путь до верха и сравниваем пути
        //
        auto buildPath = [] (StructureModelItem* _item) {
            QString path;
            auto child = _item;
            auto parent = child->parent();
            while (parent != nullptr) {
                path.prepend(QString("0000%1").arg(parent->rowOfChild(child)).right(5));
                child = parent;
                parent = child->parent();
            }
            return path;
        };
        return buildPath(_lhs) < buildPath(_rhs);
    });

    //
    // Помещаем индексы перемещаемых элементов в майм
    //
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    for (const auto& item : d->m_lastMimeItems) {
        stream << item->uuid();
    }

    QMimeData *mimeData = new QMimeData();
    mimeData->setData(kMimeType, encodedData);
    return mimeData;
}

QStringList StructureModel::mimeTypes() const
{
    return { kMimeType };
}

Qt::DropActions StructureModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions StructureModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

StructureModelItem* StructureModel::itemForIndex(const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return d->rootItem;
    }

    auto item = static_cast<StructureModelItem*>(_index.internalPointer());
    if (item == nullptr) {
        return d->rootItem;
    }

    return item;
}

void StructureModel::moveItemToRecycleBin(StructureModelItem* _item)
{
    if (_item == nullptr) {
        return;
    }

    //
    // Идём снизу, т.к. обычно корзина находится внизу
    //
    StructureModelItem* recycleBin = nullptr;
    for (int itemIndex = d->rootItem->childCount() - 1; itemIndex >= 0; --itemIndex) {
        auto item = d->rootItem->childAt(itemIndex);
        if (item->type() == Domain::DocumentObjectType::RecycleBin) {
            recycleBin = item;
            break;
        }
    }

    //
    // Собственно перемещаем элемент в корзину
    //
    moveItem(_item, recycleBin);
}

void StructureModel::setItemName(const QModelIndex& _index, const QString& _name)
{
    auto item = itemForIndex(_index);
    if (item == d->rootItem) {
        return;
    }

    item->setName(_name);
    emit dataChanged(_index, _index);
}

void StructureModel::initDocument()
{
    //
    // Если документ пустой, создаём первоначальную структуру
    //
    if (document()->content().isEmpty()) {
        //
        // TODO: сделать зависимо от предустановленного типа проекта
        //

        addDocument(Domain::DocumentObjectType::Project);
        addDocument(Domain::DocumentObjectType::Screenplay);
        addDocument(Domain::DocumentObjectType::RecycleBin);
    }
    //
    // А если данные есть, то загрузим их из документа
    //
    else {
        beginResetModel();
        d->buildModel(document());
        endResetModel();
    }
}

void StructureModel::clearDocument()
{
    if (!d->rootItem->hasChildren()) {
        return;
    }

    emit beginRemoveRows({}, 0, d->rootItem->childCount() - 1);
    while (d->rootItem->childCount() > 0) {
        d->rootItem->removeItem(d->rootItem->childAt(0));
    }
    emit endRemoveRows();
}

QByteArray StructureModel::toXml() const
{
    return d->toXml(document());
}

QModelIndex StructureModel::indexForItem(StructureModelItem* _item) const
{
    if (_item == nullptr) {
        return {};
    }

    int row = 0;
    QModelIndex parent;
    if (_item->hasParent()
        && _item->parent()->hasParent()) {
        row = _item->parent()->rowOfChild(_item);
        parent = indexForItem(_item->parent());
    } else {
        row = d->rootItem->rowOfChild(_item);
    }

    return index(row, 0, parent);
}

} // namespace BusinessLayer
