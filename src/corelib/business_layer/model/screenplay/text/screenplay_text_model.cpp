#include "screenplay_text_model.h"

#include "screenplay_text_model_folder_item.h"
#include "screenplay_text_model_scene_item.h"
#include "screenplay_text_model_splitter_item.h"
#include "screenplay_text_model_text_item.h"

#include <business_layer/templates/screenplay_template.h>

#include <domain/document_object.h>

#include <QDomDocument>


namespace BusinessLayer
{

namespace {
    const char* kMimeType = "application/x-starc/screenplay/text/item";

    const QString kDocumentTag = QLatin1String("document");
    const QString kFolderTag = QLatin1String("folder");
    const QString kSceneTag = QLatin1String("scene");
    const QString kSplitterTag = QLatin1String("splitter");
}

class ScreenplayTextModel::Implementation
{
public:
    Implementation();

    /**
     * @brief Построить модель структуры из xml хранящегося в документе
     */
    void buildModel(Domain::DocumentObject* _screenplay);

    /**
     * @brief Сформировать xml из данных модели
     */
    QByteArray toXml(Domain::DocumentObject* _screenplay) const;

    /**
     * @brief Обновить номера сцен и реплик
     */
    void updateNumbering();



    /**
     * @brief Корневой элемент дерева
     */
    ScreenplayTextModelFolderItem* rootItem = nullptr;

    /**
     * @brief Модель справочников
     */
    ScreenplayDictionariesModel* dictionariesModel = nullptr;

    /**
     * @brief Модель персонажей
     */
    CharactersModel* charactersModel = nullptr;

    /**
     * @brief Модель локаций
     */
    LocationsModel* locationModel = nullptr;
};

ScreenplayTextModel::Implementation::Implementation()
    : rootItem(new ScreenplayTextModelFolderItem)
{
}

void ScreenplayTextModel::Implementation::buildModel(Domain::DocumentObject* _screenplay)
{
    if (_screenplay == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(_screenplay->content());
    auto documentNode = domDocument.firstChildElement(kDocumentTag);
    auto rootNode = documentNode.firstChildElement();
    while (!rootNode.isNull()) {
        if (rootNode.nodeName() == kFolderTag) {
            rootItem->appendItem(new ScreenplayTextModelFolderItem(rootNode));
        } else if (rootNode.nodeName() == kSceneTag) {
            rootItem->appendItem(new ScreenplayTextModelSceneItem(rootNode));
        } else if (rootNode.nodeName() == kSplitterTag) {
            rootItem->appendItem(new ScreenplayTextModelSplitterItem(rootNode));
        } else {
            rootItem->appendItem(new ScreenplayTextModelTextItem(rootNode));
        }
        rootNode = rootNode.nextSiblingElement();
    }
}

QByteArray ScreenplayTextModel::Implementation::toXml(Domain::DocumentObject* _screenplay) const
{
    if (_screenplay == nullptr) {
        return {};
    }
    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(_screenplay->type()) + "\" version=\"1.0\">\n";
    for (int childIndex = 0; childIndex < rootItem->childCount(); ++childIndex) {
        xml += rootItem->childAt(childIndex)->toXml();
    }
    xml += "</document>";
    return xml;
}

void ScreenplayTextModel::Implementation::updateNumbering()
{
    int sceneNumber = 1;
    int dialogueNumber = 1;
    std::function<void(const ScreenplayTextModelItem*)> updateChildNumbering;
    updateChildNumbering = [&sceneNumber, &dialogueNumber, &updateChildNumbering] (const ScreenplayTextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
                case ScreenplayTextModelItemType::Folder: {
                    updateChildNumbering(childItem);
                    break;
                }

                case ScreenplayTextModelItemType::Scene: {
                    updateChildNumbering(childItem);
                    auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(childItem);
                    sceneItem->setNumber(sceneNumber++);
                    break;
                }

                case ScreenplayTextModelItemType::Text: {
                    auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                    if (textItem->paragraphType() == ScreenplayParagraphType::Character
                        && !textItem->isCorrection()) {
                        textItem->setNumber(dialogueNumber++);
                    }
                    break;
                }

                default: break;
            }
        }
    };
    updateChildNumbering(rootItem);
}


// ****


ScreenplayTextModel::ScreenplayTextModel(QObject* _parent)
    : AbstractModel({}, _parent),
      d(new Implementation)
{
}

ScreenplayTextModel::~ScreenplayTextModel() = default;

void ScreenplayTextModel::appendItem(ScreenplayTextModelItem* _item, ScreenplayTextModelItem* _parentItem)
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
    const int itemRow = _parentItem->childCount();
    beginInsertRows(parentIndex, itemRow, itemRow);
    _parentItem->insertItem(itemRow, _item);
    d->updateNumbering();
    endInsertRows();
}

void ScreenplayTextModel::prependItem(ScreenplayTextModelItem* _item, ScreenplayTextModelItem* _parentItem)
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
    beginInsertRows(parentIndex, 0, 0);
    _parentItem->insertItem(0, _item);
    d->updateNumbering();
    endInsertRows();
}

void ScreenplayTextModel::insertItem(ScreenplayTextModelItem* _item, ScreenplayTextModelItem* _afterSiblingItem)
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
    d->updateNumbering();
    endInsertRows();
}

void ScreenplayTextModel::takeItem(ScreenplayTextModelItem* _item, ScreenplayTextModelItem* _parentItem)
{
    if (_item == nullptr) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem;
    }

    if (!_parentItem->hasChild(_item)) {
        return;
    }

    //
    // Извлекаем элемент
    //
    const QModelIndex parentItemIndex = indexForItem(_item).parent();
    const int itemRowIndex = _parentItem->rowOfChild(_item);
    beginRemoveRows(parentItemIndex, itemRowIndex, itemRowIndex);
    _parentItem->takeItem(_item);
    d->updateNumbering();
    endRemoveRows();
}

void ScreenplayTextModel::removeItem(ScreenplayTextModelItem* _item)
{
    if (_item == nullptr
        || _item->parent() == nullptr) {
        return;
    }

    //
    // Удаляем элемент
    //
    auto itemParent = _item->parent();
    const QModelIndex itemParentIndex = indexForItem(_item).parent();
    const int itemRowIndex = itemParent->rowOfChild(_item);
    beginRemoveRows(itemParentIndex, itemRowIndex, itemRowIndex);
    itemParent->removeItem(_item);
    d->updateNumbering();
    endRemoveRows();
}

void ScreenplayTextModel::updateItem(ScreenplayTextModelItem* _item)
{
    if (_item == nullptr
        || !_item->isChanged()) {
        return;
    }

    const QModelIndex indexForUpdate = indexForItem(_item);
    emit dataChanged(indexForUpdate, indexForUpdate);
    _item->setChanged(false);

    if (_item->parent() != nullptr) {
        updateItem(_item->parent());
    }
}

QModelIndex ScreenplayTextModel::index(int _row, int _column, const QModelIndex& _parent) const
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

QModelIndex ScreenplayTextModel::parent(const QModelIndex& _child) const
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

int ScreenplayTextModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int ScreenplayTextModel::rowCount(const QModelIndex& _parent) const
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

Qt::ItemFlags ScreenplayTextModel::flags(const QModelIndex& _index) const
{
    //
    // TODO:
    //
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ScreenplayTextModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    auto item = itemForIndex(_index);
    if (item == nullptr) {
        return {};
    }

    return item->data(_role);
}

bool ScreenplayTextModel::canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column, const QModelIndex& _parent) const
{
    return false;
}

bool ScreenplayTextModel::dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column, const QModelIndex& _parent)
{
    return false;
}

QMimeData* ScreenplayTextModel::mimeData(const QModelIndexList& _indexes) const
{
    return nullptr;
}

QStringList ScreenplayTextModel::mimeTypes() const
{
    return { kMimeType };
}

Qt::DropActions ScreenplayTextModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions ScreenplayTextModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

ScreenplayTextModelItem* ScreenplayTextModel::itemForIndex(const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return d->rootItem;
    }

    auto item = static_cast<ScreenplayTextModelItem*>(_index.internalPointer());
    if (item == nullptr) {
        return d->rootItem;
    }

    return item;
}

QModelIndex ScreenplayTextModel::indexForItem(ScreenplayTextModelItem* _item) const
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

void ScreenplayTextModel::setDictionariesModel(ScreenplayDictionariesModel* _model)
{
    d->dictionariesModel = _model;
}

ScreenplayDictionariesModel* ScreenplayTextModel::dictionariesModel() const
{
    return d->dictionariesModel;
}

void ScreenplayTextModel::setCharactersModel(CharactersModel* _model)
{
    d->charactersModel = _model;
}

CharactersModel* ScreenplayTextModel::charactersModel() const
{
    return d->charactersModel;
}

void ScreenplayTextModel::setLocationsModel(LocationsModel* _model)
{
    d->locationModel = _model;
}

LocationsModel* ScreenplayTextModel::locationsModel() const
{
    return d->locationModel;
}

std::chrono::milliseconds ScreenplayTextModel::duration() const
{
    return d->rootItem->duration();
}

void ScreenplayTextModel::recalculateDuration()
{
    std::function<void(const ScreenplayTextModelItem*)> updateChildDuration;
    updateChildDuration = [this, &updateChildDuration] (const ScreenplayTextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
                case ScreenplayTextModelItemType::Folder: {
                    updateChildDuration(childItem);
                    break;
                }

                case ScreenplayTextModelItemType::Scene: {
                    updateChildDuration(childItem);
                    break;
                }

                case ScreenplayTextModelItemType::Text: {
                    auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                    textItem->updateDuration();
                    updateItem(textItem);
                    break;
                }

                default: break;
            }
        }
    };
    updateChildDuration(d->rootItem);
}

void ScreenplayTextModel::initDocument()
{
    //
    // Если документ пустой, создаём первоначальную структуру
    //
    if (document()->content().isEmpty()) {
        auto sceneHeading = new ScreenplayTextModelTextItem;
        sceneHeading->setParagraphType(ScreenplayParagraphType::SceneHeading);
        auto scene = new ScreenplayTextModelSceneItem;
        scene->appendItem(sceneHeading);
        appendItem(scene);
    }
    //
    // А если данные есть, то загрузим их из документа
    //
    else {
        beginResetModel();
        d->buildModel(document());
        endResetModel();
    }

    d->updateNumbering();
    recalculateDuration();
}

void ScreenplayTextModel::clearDocument()
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

QByteArray ScreenplayTextModel::toXml() const
{
    return d->toXml(document());
}

} // namespace BusinessLayer
