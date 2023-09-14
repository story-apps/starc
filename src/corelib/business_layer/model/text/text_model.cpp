#include "text_model.h"

#include "text_model_folder_item.h"
#include "text_model_group_item.h"
#include "text_model_splitter_item.h"
#include "text_model_text_item.h"
#include "text_model_xml.h"
#include "text_model_xml_writer.h"

#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/templates/text_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/model_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>
#include <utils/shugar.h>
#include <utils/tools/edit_distance.h>
#include <utils/tools/model_index_path.h>
#include <utils/tools/run_once.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDomDocument>
#include <QMimeData>
#include <QStringListModel>
#include <QXmlStreamReader>

#ifdef QT_DEBUG
#define XML_CHECKS
#endif


namespace BusinessLayer {

class TextModel::Implementation
{
public:
    explicit Implementation(TextModel* _q, TextModelFolderItem* _rootItem);

    /**
     * @brief Построить модель структуры из xml хранящегося в документе
     */
    void buildModel(Domain::DocumentObject* _document);

    /**
     * @brief Сформировать xml из данных модели
     */
    QByteArray toXml(Domain::DocumentObject* _document) const;

    /**
     * @brief Обновить значение хеша документа, если оно не задано
     */
    void updateContentHash(const QByteArray& _xml = {}) const;


    /**
     * @brief Родительский элемент
     */
    TextModel* q = nullptr;

    /**
     * @brief Корневой элемент дерева
     */
    TextModelFolderItem* rootItem = nullptr;

    /**
     * @brief Модель титульной страницы
     */
    SimpleTextModel* titlePageModel = nullptr;

    /**
     * @brief Модель синопсиса
     */
    SimpleTextModel* synopsisModel = nullptr;

    /**
     * @brief Последние скопированные данные модели
     */
    struct {
        QModelIndex from;
        QModelIndex to;
        QMimeData* data = nullptr;
    } lastMime;

    /**
     * @brief MD5-хэш текущего состояния контента
     */
    mutable QByteArray contentHash;
};

TextModel::Implementation::Implementation(TextModel* _q, TextModelFolderItem* _rootItem)
    : q(_q)
    , rootItem(_rootItem)
{
    Q_ASSERT(_rootItem);
}

void TextModel::Implementation::buildModel(Domain::DocumentObject* _document)
{
    if (_document == nullptr) {
        return;
    }

    QXmlStreamReader contentReader(_document->content());
    contentReader.readNextStartElement(); // document
    contentReader.readNextStartElement();
    while (!contentReader.atEnd()) {
        const auto currentTag = contentReader.name().toString();
        if (currentTag == xml::kDocumentTag) {
            break;
        }

        if (textFolderTypeFromString(currentTag) != TextFolderType::Undefined) {
            rootItem->appendItem(q->createFolderItem(contentReader));
        } else if (textGroupTypeFromString(currentTag) != TextGroupType::Undefined) {
            rootItem->appendItem(q->createGroupItem(contentReader));
        } else if (currentTag == xml::kSplitterTag) {
            rootItem->appendItem(q->createSplitterItem(contentReader));
        } else {
            rootItem->appendItem(q->createTextItem(contentReader));
        }
    }
}

QByteArray TextModel::Implementation::toXml(Domain::DocumentObject* _document) const
{
    if (_document == nullptr) {
        return {};
    }

    //
    // Формируем xml модели
    //
    const bool addXMlHeader = true;
    xml::TextModelXmlWriter xml(addXMlHeader);
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(_document->type())
        + "\" version=\"1.0\">\n";
    for (int childIndex = 0; childIndex < rootItem->childCount(); ++childIndex) {
        xml += rootItem->childAt(childIndex)->toXml();
    }
    xml += "</document>";

    //
    // Обновляем хэш, если он был сброшен
    //
    const auto xmlData = xml.data();
    updateContentHash(xmlData);

    return xmlData;
}

void TextModel::Implementation::updateContentHash(const QByteArray& _xml) const
{
    if (!contentHash.isEmpty()) {
        return;
    }

    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    const auto content = !_xml.isEmpty() ? _xml : toXml(q->document());
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(content);
    contentHash = hash.result();
}


// ****


TextModel::TextModel(QObject* _parent, TextModelFolderItem* _rootItem)
    : AbstractModel(
        {
            xml::kDocumentTag,
            toString(TextFolderType::Act),
            toString(TextFolderType::Sequence),
            toString(TextGroupType::Scene),
            toString(TextGroupType::Beat),
            toString(TextGroupType::Page),
            toString(TextGroupType::Panel),
            toString(TextGroupType::Chapter1),
            toString(TextGroupType::Chapter2),
            toString(TextGroupType::Chapter3),
            toString(TextGroupType::Chapter4),
            toString(TextGroupType::Chapter5),
            toString(TextGroupType::Chapter6),
            toString(TextParagraphType::UnformattedText),
            toString(TextParagraphType::InlineNote),
            toString(TextParagraphType::ActHeading),
            toString(TextParagraphType::ActFooter),
            toString(TextParagraphType::SequenceHeading),
            toString(TextParagraphType::SequenceFooter),
            toString(TextParagraphType::PartHeading),
            toString(TextParagraphType::PartFooter),
            toString(TextParagraphType::ChapterHeading),
            toString(TextParagraphType::ChapterFooter),
            toString(TextParagraphType::PageSplitter),
            toString(TextParagraphType::SceneHeading),
            toString(TextParagraphType::SceneCharacters),
            toString(TextParagraphType::BeatHeading),
            toString(TextParagraphType::Action),
            toString(TextParagraphType::Character),
            toString(TextParagraphType::Parenthetical),
            toString(TextParagraphType::Dialogue),
            toString(TextParagraphType::Lyrics),
            toString(TextParagraphType::Transition),
            toString(TextParagraphType::Shot),
            toString(TextParagraphType::Sound),
            toString(TextParagraphType::Music),
            toString(TextParagraphType::Cue),
            toString(TextParagraphType::PageHeading),
            toString(TextParagraphType::PanelHeading),
            toString(TextParagraphType::Description),
            toString(TextParagraphType::ChapterHeading1),
            toString(TextParagraphType::ChapterHeading2),
            toString(TextParagraphType::ChapterHeading3),
            toString(TextParagraphType::ChapterHeading4),
            toString(TextParagraphType::ChapterHeading5),
            toString(TextParagraphType::ChapterHeading6),
            toString(TextParagraphType::Text),
        },
        _parent)
    , d(new Implementation(this, _rootItem))
{
}

TextModel::~TextModel() = default;

TextModelFolderItem* TextModel::createFolderItem(QXmlStreamReader& _contentReader) const
{
    auto item = createFolderItem(textFolderTypeFromString(_contentReader.name().toString()));
    item->readContent(_contentReader);
    return item;
}

TextModelGroupItem* TextModel::createGroupItem(QXmlStreamReader& _contentReader) const
{
    auto item = createGroupItem(textGroupTypeFromString(_contentReader.name().toString()));
    item->readContent(_contentReader);
    return item;
}

TextModelSplitterItem* TextModel::createSplitterItem() const
{
    return new TextModelSplitterItem(this);
}

TextModelSplitterItem* TextModel::createSplitterItem(QXmlStreamReader& _contentReader) const
{
    auto item = createSplitterItem();
    item->readContent(_contentReader);
    return item;
}

TextModelTextItem* TextModel::createTextItem(QXmlStreamReader& _contentReader) const
{
    auto item = createTextItem();
    item->readContent(_contentReader);
    return item;
}

void TextModel::appendItem(TextModelItem* _item, TextModelItem* _parentItem)
{
    appendItems({ _item }, _parentItem);
}

void TextModel::appendItems(const QVector<TextModelItem*>& _items, TextModelItem* _parentItem)
{
    if (_items.isEmpty()) {
        return;
    }

    d->contentHash.clear();

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem;
    }

    const QModelIndex parentIndex = indexForItem(_parentItem);
    const int fromItemRow = _parentItem->childCount();
    const int toItemRow = fromItemRow + _items.size() - 1;
    beginInsertRows(parentIndex, fromItemRow, toItemRow);
    _parentItem->appendItems({ _items.begin(), _items.end() });
    endInsertRows();
    emit afterRowsInserted(parentIndex, fromItemRow, toItemRow);

    updateItem(_parentItem);
}

void TextModel::prependItem(TextModelItem* _item, TextModelItem* _parentItem)
{
    prependItems({ _item }, _parentItem);
}

void TextModel::prependItems(const QVector<TextModelItem*>& _items, TextModelItem* _parentItem)
{
    if (_items.isEmpty()) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem;
    }

    d->contentHash.clear();

    const QModelIndex parentIndex = indexForItem(_parentItem);
    beginInsertRows(parentIndex, 0, _items.size() - 1);
    _parentItem->prependItems({ _items.begin(), _items.end() });
    endInsertRows();
    emit afterRowsInserted(parentIndex, 0, _items.size() - 1);

    updateItem(_parentItem);
}

void TextModel::insertItem(TextModelItem* _item, TextModelItem* _afterSiblingItem)
{
    insertItems({ _item }, _afterSiblingItem);
}

void TextModel::insertItems(const QVector<TextModelItem*>& _items, TextModelItem* _afterSiblingItem)
{
    if (_items.isEmpty()) {
        return;
    }

    if (_afterSiblingItem == nullptr || _afterSiblingItem->parent() == nullptr) {
        return;
    }

    d->contentHash.clear();

    auto parentItem = _afterSiblingItem->parent();
    const QModelIndex parentIndex = indexForItem(parentItem);
    const int fromItemRow = parentItem->rowOfChild(_afterSiblingItem) + 1;
    const int toItemRow = fromItemRow + _items.size() - 1;
    beginInsertRows(parentIndex, fromItemRow, toItemRow);
    parentItem->insertItems(fromItemRow, { _items.begin(), _items.end() });
    endInsertRows();
    emit afterRowsInserted(parentIndex, fromItemRow, toItemRow);

    updateItem(parentItem);
}

void TextModel::takeItem(TextModelItem* _item)
{
    takeItems(_item, _item, _item->parent());
}

void TextModel::takeItems(TextModelItem* _fromItem, TextModelItem* _toItem,
                          TextModelItem* _parentItem)
{
    if (_fromItem == nullptr || _toItem == nullptr || _fromItem->parent() != _toItem->parent()) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem;
    }

    if (!_parentItem->hasChild(_fromItem) || !_parentItem->hasChild(_toItem)) {
        return;
    }

    d->contentHash.clear();

    const QModelIndex parentIndex = indexForItem(_fromItem).parent();
    const int fromItemRow = _parentItem->rowOfChild(_fromItem);
    const int toItemRow = _parentItem->rowOfChild(_toItem);
    Q_ASSERT(fromItemRow <= toItemRow);
    beginRemoveRows(parentIndex, fromItemRow, toItemRow);
    _parentItem->takeItems(fromItemRow, toItemRow);
    endRemoveRows();
    emit afterRowsRemoved(parentIndex, fromItemRow, toItemRow);

    updateItem(_parentItem);
}

void TextModel::removeItem(TextModelItem* _item)
{
    removeItems(_item, _item);
}

void TextModel::removeItems(TextModelItem* _fromItem, TextModelItem* _toItem)
{
    if (_fromItem == nullptr || _fromItem->parent() == nullptr || _toItem == nullptr
        || _toItem->parent() == nullptr || _fromItem->parent() != _toItem->parent()) {
        return;
    }

    d->contentHash.clear();

    auto parentItem = _fromItem->parent();
    const QModelIndex parentIndex = indexForItem(_fromItem).parent();
    const int fromItemRow = parentItem->rowOfChild(_fromItem);
    const int toItemRow = parentItem->rowOfChild(_toItem);
    Q_ASSERT(fromItemRow <= toItemRow);
    beginRemoveRows(parentIndex, fromItemRow, toItemRow);
    parentItem->removeItems(fromItemRow, toItemRow);
    endRemoveRows();
    emit afterRowsRemoved(parentIndex, fromItemRow, toItemRow);

    updateItem(parentItem);
}

void TextModel::moveItem(TextModelItem* _item, TextModelItem* _afterSiblingItem,
                         TextModelItem* _parentItem)
{
    //
    // Попытка переметить тот же самый элемент
    //
    if (_item == _afterSiblingItem) {
        return;
    }

    //
    // Элемент не принадлежит модели
    //
    const auto itemIndex = indexForItem(_item);
    if (!itemIndex.isValid()) {
        return;
    }

    //
    // Определим родителя, если не задан и его индекс
    //
    if (_parentItem == nullptr) {
        _parentItem = _afterSiblingItem != nullptr ? _afterSiblingItem->parent() : d->rootItem;
    }

    //
    // Перемещение в самое начало
    //
    if (_afterSiblingItem == nullptr) {
        //
        // Если элемент и так самый первый
        //
        if (_item->parent() == _parentItem && itemIndex.row() == 0) {
            return;
        }

        beginChangeRows();
        takeItem(_item);
        prependItem(_item, _parentItem);
        endChangeRows();
        return;
    }

    //
    // Перемещение внутри списка
    //
    const auto afterSiblingItemIndex = indexForItem(_afterSiblingItem);
    if (!afterSiblingItemIndex.isValid()) {
        return;
    }
    //
    // ... если элементы и так идут друг за другом
    //
    if (afterSiblingItemIndex.parent() == itemIndex.parent()
        && afterSiblingItemIndex.row() == itemIndex.row() - 1) {
        return;
    }
    //
    // ... собственно перемещение
    //
    beginChangeRows();
    takeItem(_item);
    insertItem(_item, _afterSiblingItem);
    endChangeRows();
}

void TextModel::updateItem(TextModelItem* _item)
{
    updateItemForRoles(_item, {});
}

void TextModel::updateItemForRoles(TextModelItem* _item, const QVector<int>& _roles)
{
    if (_item == nullptr || !_item->isChanged()) {
        return;
    }

    d->contentHash.clear();

    const QModelIndex indexForUpdate = indexForItem(_item);
    emit dataChanged(indexForUpdate, indexForUpdate, _roles);
    _item->setChanged(false);

    if (_item->parent() != nullptr) {
        updateItemForRoles(_item->parent(), _roles);
    }
}

QModelIndex TextModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    if (_row < 0 || _row > rowCount(_parent) || _column < 0 || _column > columnCount(_parent)
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

QModelIndex TextModel::parent(const QModelIndex& _child) const
{
    if (!_child.isValid()) {
        return {};
    }

    auto childItem = itemForIndex(_child);
    auto parentItem = childItem->parent();
    if (parentItem == nullptr || parentItem == d->rootItem) {
        return {};
    }

    auto grandParentItem = parentItem->parent();
    if (grandParentItem == nullptr) {
        return {};
    }

    const auto row = grandParentItem->rowOfChild(parentItem);
    return createIndex(row, 0, parentItem);
}

int TextModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int TextModel::rowCount(const QModelIndex& _parent) const
{
    if (_parent.isValid() && _parent.column() != 0) {
        return 0;
    }

    auto item = itemForIndex(_parent);
    if (item == nullptr) {
        return 0;
    }

    return item->childCount();
}

Qt::ItemFlags TextModel::flags(const QModelIndex& _index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    const auto item = itemForIndex(_index);
    switch (item->type()) {
    case TextModelItemType::Folder:
    case TextModelItemType::Group: {
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        break;
    }

    default:
        break;
    }

    return flags;
}

QVariant TextModel::data(const QModelIndex& _index, int _role) const
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

bool TextModel::canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row,
                                int _column, const QModelIndex& _parent) const
{
    Q_UNUSED(_action);
    Q_UNUSED(_column);

    if (!_data->formats().contains(mimeTypes().constFirst())) {
        return false;
    }

    //
    // Определим родителя, куда будет происходить вставка
    //
    const auto parentItemType
        = _parent.isValid() ? itemForIndex(_parent)->type() : TextModelItemType::Folder;

    //
    // Получим первый из перемещаемых элементов
    //
    QXmlStreamReader contentReader(_data->data(mimeTypes().constFirst()));
    contentReader.readNextStartElement(); // document
    contentReader.readNextStartElement();
    TextModelItemType firstItemType = TextModelItemType::Text;
    do {
        const auto currentTag = contentReader.name().toString();
        if (currentTag == xml::kDocumentTag) {
            break;
        }

        if (textFolderTypeFromString(currentTag) != TextFolderType::Undefined) {
            firstItemType = TextModelItemType::Folder;
        } else if (textGroupTypeFromString(currentTag) != TextGroupType::Undefined) {
            firstItemType = TextModelItemType::Group;
        } else if (currentTag == xml::kSplitterTag) {
            firstItemType = TextModelItemType::Splitter;
        }
    }
    once;

    //
    // Собственно определяем правила перемещения
    //
    switch (firstItemType) {

    case TextModelItemType::Folder: {
        //
        // Папки можно вставить только в папки
        //
        return parentItemType == TextModelItemType::Folder;
    }

    case TextModelItemType::Group: {
        //
        // Группы можно вставить в папки
        //
        if (parentItemType == TextModelItemType::Folder) {
            return true;
        }
        //
        // ... либо в группы более высокого уровня иерархии
        //
        else if (parentItemType == TextModelItemType::Group) {
            const auto parentItem = static_cast<TextModelGroupItem*>(itemForIndex(_parent));
            const auto firstItem = createGroupItem(contentReader);
            return parentItem->level() < firstItem->level();
        }
        //
        // ... а больше никуда нельзя
        //
        else {
            return false;
        }
    }

    default: {
        return false;
    }
    }
}

bool TextModel::dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column,
                             const QModelIndex& _parent)
{
    Q_UNUSED(_column);

    //
    // _row - индекс, куда вставлять, если в папку, то он равен -1 и если в самый низ списка, то он
    // тоже равен -1
    //

    if (_data == 0 || !canDropMimeData(_data, _action, _row, _column, _parent)) {
        return false;
    }

    switch (_action) {
    case Qt::IgnoreAction: {
        return true;
    }

    case Qt::MoveAction:
    case Qt::CopyAction: {
        //
        // Определим элемент после которого планируется вставить данные
        //
        QModelIndex insertAnchorIndex;
        //
        // ... вкладывается первым
        //
        if (_row == 0) {
            insertAnchorIndex = _parent;
        }
        //
        // ... вкладывается в конец
        //
        else if (_row == -1) {
            if (_parent.isValid()) {
                insertAnchorIndex = _parent;
            } else {
                insertAnchorIndex = index(d->rootItem->childCount() - 1, 0);
            }
        }
        //
        // ... устанавливается после заданного
        //
        else {
            int delta = 1;
            //
            // ... для папок, при вставке в самый конец также нужно учитывать
            //     текстовый блок закрывающий папку
            //
            if (_parent.isValid() && rowCount(_parent) == _row
                && itemForIndex(_parent)->type() == TextModelItemType::Folder) {
                ++delta;
            }
            insertAnchorIndex = index(_row - delta, 0, _parent);
        }
        if (d->lastMime.from == insertAnchorIndex || d->lastMime.to == insertAnchorIndex) {
            return false;
        }
        TextModelItem* insertAnchorItem = itemForIndex(insertAnchorIndex);

        //
        // Начинаем операцию изменения модели
        //
        beginChangeRows();

        //
        // Если это перемещение внутри модели, то удалим старые элементы
        //
        if (d->lastMime.data == _data) {
            for (int row = d->lastMime.to.row(); row >= d->lastMime.from.row(); --row) {
                const auto& itemIndex = index(row, 0, d->lastMime.from.parent());
                auto item = itemForIndex(itemIndex);
                removeItem(item);
            }
            d->lastMime = {};
        }

        //
        // Вставим перемещаемые элементы
        //
        // ... cчитываем данные и последовательно вставляем в модель
        //
        QXmlStreamReader contentReader(_data->data(mimeTypes().constFirst()));
        contentReader.readNextStartElement(); // document
        contentReader.readNextStartElement();
        bool isFirstItemHandled = false;
        TextModelItem* lastItem = insertAnchorItem;
        while (!contentReader.atEnd()) {
            const auto currentTag = contentReader.name().toString();
            if (currentTag == xml::kDocumentTag) {
                break;
            }

            TextModelItem* newItem = nullptr;
            if (textFolderTypeFromString(currentTag) != TextFolderType::Undefined) {
                newItem = createFolderItem(contentReader);
            } else if (textGroupTypeFromString(currentTag) != TextGroupType::Undefined) {
                newItem = createGroupItem(contentReader);
            } else if (currentTag == xml::kSplitterTag) {
                newItem = createSplitterItem(contentReader);
            } else {
                newItem = createTextItem(contentReader);
            }

            if (!isFirstItemHandled) {
                isFirstItemHandled = true;
                //
                // Вставить в начало папки
                //
                if (_row == 0) {
                    //
                    // При вставке в папку, нужно не забыть про открывающий папку блок
                    //
                    if (lastItem->type() == TextModelItemType::Folder && _parent.isValid()) {
                        insertItem(newItem, lastItem->childAt(0));
                    }
                    //
                    // В остальных слачаях, добавляем в начало
                    //
                    else {
                        prependItem(newItem, lastItem);
                    }
                }
                //
                // Вставить в конец папки
                //
                else if (_row == -1) {
                    //
                    // При вставке в папку, нужно не забыть про завершающий папку блок
                    //
                    if (lastItem->type() == TextModelItemType::Folder && _parent.isValid()) {
                        insertItem(newItem, lastItem->childAt(lastItem->childCount() - 2));
                    }
                    //
                    // При вставке в группу, просто добавляем в конец
                    //
                    else if (lastItem->type() == TextModelItemType::Group && _parent.isValid()) {
                        appendItem(newItem, lastItem);
                    }
                    //
                    // В остальных случаях просто вставляем после предыдущего
                    //
                    else {
                        insertItem(newItem, lastItem);
                    }
                }
                //
                // Вставить в середину папки
                //
                else {
                    insertItem(newItem, lastItem);
                }
            } else {
                insertItem(newItem, lastItem);
            }

            lastItem = newItem;
        }

        //
        // Операция изменения завершена
        //
        endChangeRows();

        return true;
    }

    default: {
        return false;
    }
    }
}

QMimeData* TextModel::mimeData(const QModelIndexList& _indexes) const
{
    if (_indexes.isEmpty()) {
        return nullptr;
    }

    //
    // Выделение может быть только последовательным, но нужно учесть ситуацию, когда в выделение
    // попадает родительский элемент и не все его дочерние элементы, поэтому просто использовать
    // последний элемент некорректно, нужно проверить, не входит ли его родитель в выделение
    //

    QVector<QModelIndex> correctedIndexes;
    for (const auto& index : _indexes) {
        if (!_indexes.contains(index.parent())) {
            correctedIndexes.append(index);
        }
    }
    if (correctedIndexes.isEmpty()) {
        return nullptr;
    }

    //
    // Для того, чтобы запретить разрывать папки проверяем выделены ли элементы одного уровня и
    // идущие подряд
    //
    bool itemsHaveSameParent = true;
    bool itemsPlacedContinuously = true;
    QModelIndex lastItemIndex;
    const QModelIndex& genericParent = correctedIndexes.first().parent();
    for (const auto& index : correctedIndexes) {
        if (index != correctedIndexes.constFirst() && index.row() != lastItemIndex.row() + 1) {
            itemsPlacedContinuously = false;
            break;
        }

        if (index.parent() != genericParent) {
            itemsHaveSameParent = false;
            break;
        }

        lastItemIndex = index;
    }
    if (!itemsHaveSameParent || !itemsPlacedContinuously) {
        return nullptr;
    }

    //
    // Если выделены элементы одного уровня, то создаём майм-данные
    //

    std::sort(correctedIndexes.begin(), correctedIndexes.end());
    QModelIndex fromIndex = correctedIndexes.first();
    QModelIndex toIndex = correctedIndexes.last();

    auto mimeData = new QMimeData;
    const bool clearUuid = false;
    mimeData->setData(mimeTypes().constFirst(),
                      mimeFromSelection(fromIndex, 0, toIndex, 1, clearUuid).toUtf8());

    d->lastMime = { fromIndex, toIndex, mimeData };

    return mimeData;
}

Qt::DropActions TextModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions TextModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool TextModel::moveRows(const QModelIndex& _sourceParent, int _sourceRow, int _count,
                         const QModelIndex& _destinationParent, int _destinationRow)
{
    Q_ASSERT(_count == 1);
    TextModelItem* item = itemForIndex(index(_sourceRow, 0, _sourceParent));
    TextModelItem* afterSiblingItem = nullptr;
    if (_destinationRow != 0) {
        afterSiblingItem = itemForIndex(index(_destinationRow - 1, 0, _destinationParent));
    }
    TextModelItem* parentItem = itemForIndex(_destinationParent);
    moveItem(item, afterSiblingItem, parentItem);
    return true;
}

QString TextModel::mimeFromSelection(const QModelIndex& _from, int _fromPosition,
                                     const QModelIndex& _to, int _toPosition, bool _clearUuid) const
{
    if (document() == nullptr) {
        return {};
    }

    if (ModelIndexPath(_to) < ModelIndexPath(_from)
        || (_from == _to && _fromPosition >= _toPosition)) {
        return {};
    }

    const auto fromItem = itemForIndex(_from);
    if (fromItem == nullptr) {
        return {};
    }

    const auto toItem = itemForIndex(_to);
    if (toItem == nullptr) {
        return {};
    }


    const bool addXmlHeader = true;
    xml::TextModelXmlWriter xml(addXmlHeader);
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type())
        + "\" version=\"1.0\">\n";

    auto buildXmlFor = [&xml, fromItem, _fromPosition, toItem, _toPosition,
                        _clearUuid](TextModelItem* _fromItemParent, int _fromItemRow) {
        bool needAddSplitter = false;
        if (fromItem != toItem && fromItem->type() == TextModelItemType::Text
            && toItem->type() == TextModelItemType::Text) {
            const auto fromTextItem = static_cast<TextModelTextItem*>(fromItem);
            const auto toTextItem = static_cast<TextModelTextItem*>(toItem);
            needAddSplitter = fromTextItem->isInFirstColumn().has_value()
                && fromTextItem->isInFirstColumn().value() == true
                && toTextItem->isInFirstColumn().has_value()
                && toTextItem->isInFirstColumn().value() == false;
        }

        auto addSplitterXmlIfNeeded
            = [&xml, fromItem, needAddSplitter](TextModelSplitterItemType _type) {
                  if (needAddSplitter) {
                      TextModelSplitterItem splitterItem(fromItem->model());
                      splitterItem.setSplitterType(_type);
                      xml += splitterItem.toXml();
                  }
              };

        addSplitterXmlIfNeeded(TextModelSplitterItemType::Start);

        for (int childIndex = _fromItemRow; childIndex < _fromItemParent->childCount();
             ++childIndex) {
            const auto childItem = _fromItemParent->childAt(childIndex);

            switch (childItem->type()) {
            case TextModelItemType::Folder: {
                const auto folderItem = static_cast<TextModelFolderItem*>(childItem);
                xml += folderItem->toXml(fromItem, _fromPosition, toItem, _toPosition, _clearUuid);
                break;
            }

            case TextModelItemType::Group: {
                const auto sceneItem = static_cast<TextModelGroupItem*>(childItem);
                xml += sceneItem->toXml(fromItem, _fromPosition, toItem, _toPosition, _clearUuid);
                break;
            }

            case TextModelItemType::Text: {
                const auto textItem = static_cast<TextModelTextItem*>(childItem);

                //
                // Не сохраняем закрывающие блоки неоткрытых папок, всё это делается внутри самих
                // папок
                //
                if (textItem->paragraphType() == TextParagraphType::ActFooter
                    || textItem->paragraphType() == TextParagraphType::SequenceFooter
                    || textItem->paragraphType() == TextParagraphType::PartFooter
                    || textItem->paragraphType() == TextParagraphType::ChapterFooter) {
                    break;
                }

                if (textItem == fromItem && textItem == toItem) {
                    xml += { textItem, _fromPosition, _toPosition - _fromPosition };
                } else if (textItem == fromItem) {
                    xml += { textItem, _fromPosition, textItem->text().length() - _fromPosition };
                } else if (textItem == toItem) {
                    xml += { textItem, 0, _toPosition };
                } else {
                    xml += textItem;
                }
                break;
            }

            default: {
                xml += childItem;
                break;
            }
            }

            const bool recursively = true;
            if (childItem == toItem || childItem->hasChild(toItem, recursively)) {
                addSplitterXmlIfNeeded(TextModelSplitterItemType::End);
                return true;
            }
        }

        addSplitterXmlIfNeeded(TextModelSplitterItemType::End);

        return false;
    };
    auto fromItemParent = fromItem->parent();
    auto fromItemRow = fromItemParent->rowOfChild(fromItem);
    //
    // Если построить нужно начиная с заголовка сцены или папки, и при этом майм нужен не только для
    // блока заголовка, то нужно захватить и саму сцену/папку
    //
    if (fromItem->type() == TextModelItemType::Text && fromItemParent->hasParent()) {
        const auto textItem = static_cast<TextModelTextItem*>(fromItem);
        if (textItem->paragraphType() == TextParagraphType::ActHeading
            || textItem->paragraphType() == TextParagraphType::SequenceHeading
            || textItem->paragraphType() == TextParagraphType::PartHeading
            || textItem->paragraphType() == TextParagraphType::ChapterHeading
            || textItem->paragraphType() == TextParagraphType::SceneHeading
            || textItem->paragraphType() == TextParagraphType::BeatHeading
            || textItem->paragraphType() == TextParagraphType::PageHeading
            || textItem->paragraphType() == TextParagraphType::PanelHeading
            || textItem->paragraphType() == TextParagraphType::ChapterHeading1
            || textItem->paragraphType() == TextParagraphType::ChapterHeading2
            || textItem->paragraphType() == TextParagraphType::ChapterHeading3
            || textItem->paragraphType() == TextParagraphType::ChapterHeading4
            || textItem->paragraphType() == TextParagraphType::ChapterHeading5
            || textItem->paragraphType() == TextParagraphType::ChapterHeading6) {
            auto newFromItem = fromItemParent;
            fromItemParent = fromItemParent->parent();
            fromItemRow = fromItemParent->rowOfChild(newFromItem);
        }
    }
    //
    // Собственно строим xml с данными выделенного интервала
    //
    while (buildXmlFor(fromItemParent, fromItemRow) != true) {
        auto newFromItem = fromItemParent;
        fromItemParent = fromItemParent->parent();
        fromItemRow
            = fromItemParent->rowOfChild(newFromItem) + 1; // +1, т.к. текущий мы уже обработали
    }

    xml += "</document>";
    return xml.data();
}

int TextModel::insertFromMime(const QModelIndex& _index, int _positionInBlock,
                              const QString& _mimeData)
{
    constexpr auto invalidLength = -1;
    if (!_index.isValid()) {
        return invalidLength;
    }

    if (_mimeData.isEmpty()) {
        return invalidLength;
    }

    //
    // Начинаем операцию изменения модели
    //
    beginChangeRows();

    //
    // Определим элемент, внутрь, или после которого будем вставлять данные
    //
    auto item = itemForIndex(_index);
    if (item->type() != TextModelItemType::Text) {
        Log::warning(
            "Trying to insert from mime to position with no text item. Aborting insertion.");
        return invalidLength;
    }

    //
    // Посмотрим на содержимое майм данных и проверим сколько там текстовых блоков
    //
    QString correctedMimeData = _mimeData;
    const auto mimeInfo = ModelHelper::isMimeHasJustOneBlock(correctedMimeData);
    const auto isMimeContainsJustOneBlock = mimeInfo.first;
    const auto isMimeContainsFolderOrSequence = mimeInfo.second;
    //
    // ... если текст блока, в который идёт вставка не пуст, а в майм данных есть группа и всего
    //     один текстовый элемент в ней
    //
    auto textItem = static_cast<TextModelTextItem*>(item);
    if (!textItem->text().isEmpty() && isMimeContainsJustOneBlock
        && isMimeContainsFolderOrSequence) {
        //
        // ... то удалим группирующий элемент, чтобы вставлять только текст
        //
        QDomDocument mimeDocument;
        mimeDocument.setContent(correctedMimeData);
        auto document = mimeDocument.firstChildElement(xml::kDocumentTag);
        // document -> folder/group -> content tag -> text block
        const auto itemNode = document.firstChild();
        auto contentNode = itemNode.firstChild();
        while (contentNode.nodeName() != xml::kContentTag) {
            contentNode = contentNode.nextSibling();
        }
        const auto textNode = contentNode.firstChild();
        document.removeChild(itemNode);
        document.appendChild(textNode);
        correctedMimeData = mimeDocument.toString();
    }

    //
    // Определим необходимость удаления элемента в который идёт вставка после удаления
    //
    TextModelItem* itemToDelete = nullptr;
    if (textItem->text().isEmpty()
        && (!isMimeContainsJustOneBlock || isMimeContainsFolderOrSequence)) {
        itemToDelete = textItem;
    }

    //
    // Подготовимся к вставке данных
    //
    QString sourceBlockEndContent;
    QVector<TextModelItem*> itemsToPlaceAfterMime;
    auto extractEndContentFromBlock
        = [this, _index, _positionInBlock, &textItem, &sourceBlockEndContent] {
              //
              // Извлекаем контент только если текстовый блок не пуст
              //
              if (textItem->text().isEmpty()) {
                  return;
              }

              //
              // Разделим текущий блок на два, даже если курсор в самом начале блока
              //
              const bool clearUuid = false;
              sourceBlockEndContent = mimeFromSelection(_index, _positionInBlock, _index,
                                                        textItem->text().length(), clearUuid);
              textItem->removeText(_positionInBlock);
              updateItem(textItem);
          };
    std::optional<int> mimeLength;
    auto increaseMimeLength = [&mimeLength](int _length = 0) {
        if (mimeLength.has_value()) {
            ++mimeLength.value();
        }
        mimeLength = mimeLength.value_or(0) + _length;
    };
    //
    // ... если вставляются несколько блоков
    //
    if (!isMimeContainsJustOneBlock) {
        //
        // ... если вставка идёт в заголовке папки/группы/части/главы/сцены/панели/страницы/и т.п.
        //
        if (textItem->paragraphType() == TextParagraphType::ActHeading
            || textItem->paragraphType() == TextParagraphType::SequenceHeading
            || textItem->paragraphType() == TextParagraphType::PartHeading
            || textItem->paragraphType() == TextParagraphType::ChapterHeading
            || textItem->paragraphType() == TextParagraphType::SceneHeading
            || textItem->paragraphType() == TextParagraphType::BeatHeading
            || textItem->paragraphType() == TextParagraphType::PageHeading
            || textItem->paragraphType() == TextParagraphType::PanelHeading
            || textItem->paragraphType() == TextParagraphType::ChapterHeading
            || textItem->paragraphType() == TextParagraphType::ChapterHeading1
            || textItem->paragraphType() == TextParagraphType::ChapterHeading2
            || textItem->paragraphType() == TextParagraphType::ChapterHeading3
            || textItem->paragraphType() == TextParagraphType::ChapterHeading4
            || textItem->paragraphType() == TextParagraphType::ChapterHeading5
            || textItem->paragraphType() == TextParagraphType::ChapterHeading6) {
            //
            // ... то вставляемые данные будем добавлять после текущего элемента
            //

            itemToDelete = nullptr;
            increaseMimeLength(textItem->text().length() - _positionInBlock);
        }
        //
        // ... если вставка идёт в завершении папки/группы/части/главы
        //
        else if (textItem->paragraphType() == TextParagraphType::ActFooter
                 || textItem->paragraphType() == TextParagraphType::SequenceFooter
                 || textItem->paragraphType() == TextParagraphType::PartFooter
                 || textItem->paragraphType() == TextParagraphType::ChapterFooter) {
            //
            // ... то вставляемые данные будем добавлять после родителя текущего элемента
            //
            item = item->parent();

            itemToDelete = nullptr;
            increaseMimeLength(textItem->text().length() - _positionInBlock);
        }
        //
        // ... если вставка идёт в обычный текстовый блок
        //
        else {
            //
            // ... разделим текущий блок на части, чтобы вставить данные между ними
            //
            extractEndContentFromBlock();

            if (_positionInBlock > 0) {
                increaseMimeLength();
            }
        }

    }
    //
    // ... если вставляется один блок
    //
    else {
        //
        // ... разделим текущий блок на части, чтобы потом их сшить
        //
        extractEndContentFromBlock();
    }

    //
    // Считываем данные и последовательно вставляем в модель
    //
    bool isFirstTextItemHandled = false;
    bool isSplitterStartWasCreated = false;
    TextModelItem* lastItem = item;
    TextModelItem* insertAfterItem = lastItem;
    QVector<TextModelItem*> itemsToInsert;
    auto insertCollectedItems = [this, &lastItem, &insertAfterItem, &itemsToInsert] {
        if (itemsToInsert.isEmpty()) {
            return;
        }

        insertItems(itemsToInsert, insertAfterItem);
        lastItem = itemsToInsert.constLast();
        itemsToInsert.clear();
    };
    QXmlStreamReader contentReader(correctedMimeData);
    contentReader.readNextStartElement(); // document
    contentReader.readNextStartElement();
    struct ItemContainerType {
        TextFolderType folder = TextFolderType::Undefined;
        TextGroupType group = TextGroupType::Undefined;

        bool isValid() const
        {
            return folder != TextFolderType::Undefined || group != TextGroupType::Undefined;
        }
        bool isFolder() const
        {
            return folder != TextFolderType::Undefined;
        }
        bool isGroup() const
        {
            return group != TextGroupType::Undefined;
        }
    };
    while (!contentReader.atEnd()) {
        const auto currentTag = contentReader.name().toString();

        //
        // Если дошли до конца
        //
        if (currentTag == xml::kDocumentTag) {
            //
            // ... поместим в модель все собранные элементы
            //
            insertCollectedItems();
            break;
        }

        //
        // При вставке папки или группы, если предыдущий текстовый элемент был в группе,
        // то вставлять их будем не после текстового элемента, а после группы
        //
        const auto currentItemContainerType
            = ItemContainerType{ textFolderTypeFromString(currentTag),
                                 textGroupTypeFromString(currentTag) };
        if (currentItemContainerType.isValid()
            && (lastItem->type() == TextModelItemType::Text
                || lastItem->type() == TextModelItemType::Splitter)) {
            //
            // ... вставим в модель всё, что было собрано до этого момента
            //
            insertCollectedItems();


            //
            // ... если у предыдущего элемента есть родитель
            //     и этот родитель не является корнем
            //     и этот родитель является папкой или группой (более низкого уровня),
            //     то вставим новую группу/папку после родителя текстового элемента
            //
            forever
            {
                const auto lastItemParent = lastItem != nullptr ? lastItem->parent() : nullptr;
                if (lastItemParent != nullptr && lastItemParent != d->rootItem) {
                    //
                    // ... в папку будем вставлять после текущего текстового элемента, т.к. вложение
                    //     в этом случае не имеет ограничений
                    //
                    if (lastItemParent->type() == TextModelItemType::Folder) {
                        //
                        // ... и при этом вырезаем из него все блоки, идущие до конца группы/папки
                        //
                        const int targetChildCountDelta = 1;
                        int movedItemIndex = lastItemParent->rowOfChild(lastItem) + 1;
                        while (lastItemParent->childCount()
                               > movedItemIndex + targetChildCountDelta) {
                            itemsToPlaceAfterMime.append(lastItemParent->childAt(movedItemIndex));
                            ++movedItemIndex;
                        }
                        //
                        // ... вставлять будем после текущего элемента
                        //
                        insertAfterItem = lastItem;
                    }
                    //
                    // ... в группу вставляется папка - идёт до группы уровня 0 и вставляем после
                    //     неё, либо до момента, когда родителем группы будет папка
                    //
                    else if (lastItemParent->type() == TextModelItemType::Group
                             && currentItemContainerType.isFolder()) {
                        //
                        // ... вставка идёт после группы не самого верхнего уровня, переходим к
                        //     более верхнему уровню иерархии в поиске места для вставки, при этом
                        //     текстовые и остальные блоки до конца группирующего элемента должны
                        //     быть вырезаны для последующей вставки после вставляемых данных
                        //
                        if (textGroupTypeLevel(
                                static_cast<TextGroupType>(lastItemParent->subtype()))
                            > 0) {
                            int movedItemIndex = lastItemParent->rowOfChild(lastItem) + 1;
                            while (lastItemParent->childCount() > movedItemIndex) {
                                itemsToPlaceAfterMime.append(
                                    lastItemParent->childAt(movedItemIndex));
                                ++movedItemIndex;
                            }

                            lastItem = lastItemParent;
                            continue;
                        }

                        //
                        // ... и при этом вырезаем из него все блоки, идущие до конца группы/папки
                        //
                        int movedItemIndex = lastItemParent->rowOfChild(lastItem) + 1;
                        while (lastItemParent->childCount() > movedItemIndex) {
                            itemsToPlaceAfterMime.append(lastItemParent->childAt(movedItemIndex));
                            ++movedItemIndex;
                        }
                        //
                        // Собственно берём родителя вместо самого элемента
                        //
                        lastItem = lastItemParent;
                        insertAfterItem = lastItemParent;
                    }
                    //
                    // ... в группу вставляется группа - идёт до группы того же уровня
                    //
                    else if (lastItemParent->type() == TextModelItemType::Group
                             && currentItemContainerType.isGroup()) {
                        //
                        // ... если уровень вставляемой группы выше уровня текущей, то переносим
                        // оставшиеся элементы и переходим к родителю
                        //
                        if (textGroupTypeLevel(currentItemContainerType.group) < textGroupTypeLevel(
                                static_cast<TextGroupType>(lastItemParent->subtype()))) {
                            int movedItemIndex = lastItemParent->rowOfChild(lastItem) + 1;
                            while (lastItemParent->childCount() > movedItemIndex) {
                                itemsToPlaceAfterMime.append(
                                    lastItemParent->childAt(movedItemIndex));
                                ++movedItemIndex;
                            }
                            lastItem = lastItemParent;
                            continue;
                        }

                        //
                        // ... и при этом вырезаем из него все блоки, идущие до конца группы/папки
                        //
                        int movedItemIndex = lastItemParent->rowOfChild(lastItem) + 1;
                        while (lastItemParent->childCount() > movedItemIndex) {
                            itemsToPlaceAfterMime.append(lastItemParent->childAt(movedItemIndex));
                            ++movedItemIndex;
                        }
                        //
                        // Собственно берём родителя вместо самого элемента
                        //
                        lastItem = lastItemParent;
                        insertAfterItem = lastItemParent;
                    }
                    //
                    // ... в противном случае, вставлять будем после последнего вставленного
                    // элемента
                    //
                    else {
                        insertAfterItem = lastItem;
                    }
                }

                break;
            }
        }


        //
        // Вставляется папка
        //
        TextModelItem* newItem = nullptr;
        if (currentItemContainerType.isFolder()) {
            newItem = createFolderItem(contentReader);
        }
        //
        // Вставляется сцена
        //
        else if (currentItemContainerType.isGroup()) {
            auto newGroupItem = createGroupItem(contentReader);
            increaseMimeLength(newGroupItem->length());

            //
            // Если группа вставляется после группы, учитываем уровень группы, и при необходимости
            // поднимаемся ещё на уровень выше
            //
            if (insertAfterItem && insertAfterItem->type() == TextModelItemType::Group) {
                auto groupItem = static_cast<TextModelGroupItem*>(insertAfterItem);
                while (groupItem != nullptr && groupItem->level() > newGroupItem->level()
                       && groupItem->parent() != d->rootItem) {
                    insertAfterItem = insertAfterItem->parent();
                    if (insertAfterItem != nullptr
                        && insertAfterItem->type() != TextModelItemType::Group) {
                        break;
                    }

                    groupItem = static_cast<TextModelGroupItem*>(insertAfterItem);
                }
            }

            newItem = newGroupItem;
        }
        //
        // Вставляется разделитель
        //
        else if (currentTag == xml::kSplitterTag) {
            const auto previousItemType
                = insertAfterItem != nullptr ? insertAfterItem->type() : TextModelItemType::Folder;
            switch (previousItemType) {
            case TextModelItemType::Text: {
                //
                // Т.к. вставка разделителя таблицы аффектит текст, то считаем,
                // что первый текстовый блок был обработан
                //
                if (!isFirstTextItemHandled) {
                    isFirstTextItemHandled = true;
                }

                QScopedPointer<TextModelSplitterItem> newSplitterItem(
                    createSplitterItem(contentReader));
                const auto textItem = static_cast<TextModelTextItem*>(insertAfterItem);

                //
                // Если предыдущий элемент уже в таблице, то не нужно ничего создавать тут
                //
                if (textItem->isInFirstColumn().has_value()) {
                    break;
                }
                //
                // Если создаётся открывающий элемент, запоним этот нюанс
                //
                if (newSplitterItem->splitterType() == TextModelSplitterItemType::Start) {
                    isSplitterStartWasCreated = true;
                }
                //
                // Если создаётся закрывающий элемент, то позволяем этому свершиться,
                // только если был создал открывающий, иначе пропускаем его
                //
                else {
                    if (!isSplitterStartWasCreated) {
                        break;
                    }
                    //
                    // ... сбрасываем статус создания открывающего элемента для следующего прохода
                    //
                    isSplitterStartWasCreated = false;
                }

                //
                // Если всё прошло успешно, добавляем созданный разделитель
                //
                increaseMimeLength(1);
                newItem = createSplitterItem();
                newItem->copyFrom(newSplitterItem.data());
                break;
            }

            default: {
                Q_ASSERT(false);
                break;
            }
            }
        }
        //
        // Вставляется текстовый элемент
        //
        else {
            auto newTextItem = createTextItem(contentReader);
            increaseMimeLength(newTextItem->text().length());
            //
            // Смотрим на положение в таблице предыдущего элемента
            //
            if (insertAfterItem != nullptr && insertAfterItem->type() == TextModelItemType::Text) {
                const auto textItem = static_cast<TextModelTextItem*>(insertAfterItem);
                //
                // ... если он в таблице, то помещаем добавляемый элемент в ту же колонку
                //
                if (textItem->isInFirstColumn().has_value()) {
                    newTextItem->setInFirstColumn(textItem->isInFirstColumn());
                }
                //
                // ... если не в таблице, и при вставке не было добавлено таблицы,
                //     то удаляем информацию о таблице в новом текстовом элементе
                //
                else if (!isSplitterStartWasCreated) {
                    newTextItem->setInFirstColumn({});
                }
            }
            //
            // Если вставляется текстовый элемент внутрь уже существующего элемента
            //
            if (!isFirstTextItemHandled) {
                isFirstTextItemHandled = true;
                //
                // ... при этом это вставка только одного элемента, то просто объединим их
                //
                if (isMimeContainsJustOneBlock && item->type() == TextModelItemType::Text
                    && !itemsToPlaceAfterMime.contains(item)) {
                    auto textItem = static_cast<TextModelTextItem*>(item);
                    textItem->mergeWith(newTextItem);
                    updateItem(textItem);
                    delete newTextItem;
                    //
                    // ... и исключаем исходный блок из переноса, если он был туда помещён
                    //
                    itemsToPlaceAfterMime.removeAll(textItem);
                }
                //
                // ... иначе вставляем текстовый элемент в модель
                //
                else {
                    newItem = newTextItem;
                }
            }
            //
            // В противном случае вставляем текстовый элемент в модель
            //
            else {
                newItem = newTextItem;
            }
        }

        if (newItem != nullptr) {
            itemsToInsert.append(newItem);
            lastItem = newItem;
        }
    }
    insertCollectedItems();

    //
    // Если есть оторванный от первого блока текст
    //
    if (!sourceBlockEndContent.isEmpty()) {
        contentReader.clear();
        contentReader.addData(sourceBlockEndContent);
        contentReader.readNextStartElement(); // document
        const auto hasContent = contentReader.readNextStartElement(); // potential text node
        if (textFolderTypeFromString(contentReader.name().toString()) != TextFolderType::Undefined
            || textGroupTypeFromString(contentReader.name().toString())
                != TextGroupType::Undefined) {
            while (contentReader.name() != xml::kContentTag) {
                contentReader.readNextStartElement(); // content
            }
            contentReader.readNextStartElement(); // text node
        }

        if (hasContent) {
            auto newTextItem = createTextItem(contentReader);
            if (isMimeContainsJustOneBlock) {
                auto textItem = static_cast<TextModelTextItem*>(lastItem);
                textItem->mergeWith(newTextItem);
                updateItem(textItem);
                delete newTextItem;
            } else {
                insertItem(newTextItem, lastItem);
                lastItem = newTextItem;
            }
        }
    }

    //
    // Если есть оторванные текстовые блоки
    //
    if (!itemsToPlaceAfterMime.isEmpty()) {
        //
        // Извлечём блоки из родителя
        //
        for (auto item : itemsToPlaceAfterMime) {
            if (item->hasParent()) {
                auto itemParent = item->parent();
                takeItem(item);

                //
                // Удалим родителя, если у него больше не осталось детей
                // NOTE: актуально для случая, когда в сцене был один абзац заголовка
                //
                if (itemParent->childCount() == 0) {
                    removeItem(itemParent);
                }
            }
        }

        //
        // Просто вставляем их внутрь или после последнего элемента
        //
        for (auto item : itemsToPlaceAfterMime) {
            //
            // Удаляем пустые элементы модели
            //
            switch (item->type()) {
            case TextModelItemType::Group: {
                auto groupItem = static_cast<TextModelGroupItem*>(item);
                if (groupItem->isEmpty()) {
                    delete groupItem;
                    groupItem = nullptr;
                    continue;
                }
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<TextModelTextItem*>(item);
                if (textItem->text().isEmpty()) {
                    delete textItem;
                    textItem = nullptr;
                    continue;
                }
                break;
            }

            default: {
                break;
            }
            }

            if (lastItem->type() == TextModelItemType::Group) {
                //
                // ... корректируем уровень групп при необходимости
                //
                if (item->type() == TextModelItemType::Group) {
                    auto lastItemGroup = static_cast<TextModelGroupItem*>(lastItem);
                    auto itemGroup = static_cast<TextModelGroupItem*>(item);
                    if (lastItemGroup->level() < itemGroup->level()) {
                        appendItem(item, lastItem);
                    } else if (lastItemGroup->level() == itemGroup->level()) {
                        insertItem(item, lastItem);
                    } else {
                        Q_ASSERT(false);
                        insertItem(item, lastItem);
                    }
                } else {
                    appendItem(item, lastItem);
                }
            } else {
                insertItem(item, lastItem);
            }
            lastItem = item;
        }
    }

    //
    // Если необходимо, то удаляем пустой блок в которым стоял курсор при вставке
    //
    if (itemToDelete != nullptr) {
        if (itemToDelete->parent() != nullptr
            && itemToDelete->parent()->type() == TextModelItemType::Group
            && itemToDelete->parent()->childCount() == 1) {
            itemToDelete = itemToDelete->parent();
        }
        removeItem(itemToDelete);
    }

    //
    // Завершаем изменение
    //
    endChangeRows();

    return mimeLength.value_or(0);
}

TextModelItem* TextModel::itemForIndex(const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return d->rootItem;
    }

    auto item = static_cast<TextModelItem*>(_index.internalPointer());
    if (item == nullptr) {
        return d->rootItem;
    }

    return item;
}

QModelIndex TextModel::indexForItem(TextModelItem* _item) const
{
    if (_item == nullptr) {
        return {};
    }

    int row = 0;
    QModelIndex parent;
    if (_item->hasParent() && _item->parent()->hasParent()) {
        row = _item->parent()->rowOfChild(_item);
        parent = indexForItem(_item->parent());
    } else {
        row = d->rootItem->rowOfChild(_item);
    }

    return index(row, 0, parent);
}

void TextModel::setTitlePageModel(SimpleTextModel* _model)
{
    d->titlePageModel = _model;
}

SimpleTextModel* TextModel::titlePageModel() const
{
    ModelHelper::initTitlePageModel(d->titlePageModel);

    return d->titlePageModel;
}

void TextModel::setSynopsisModel(SimpleTextModel* _model)
{
    d->synopsisModel = _model;
}

SimpleTextModel* TextModel::synopsisModel() const
{
    return d->synopsisModel;
}

QByteArray TextModel::contentHash() const
{
    d->updateContentHash();
    return d->contentHash;
}

void TextModel::initDocument()
{
    //
    // Если документ пустой, создаём первоначальную структуру
    //
    if (document()->content().isEmpty()) {
        initEmptyDocument();
    }
    //
    // А если данные есть, то загрузим их из документа
    //
    else {
        beginResetModelTransaction();
        d->buildModel(document());
        endResetModelTransaction();
    }

    //
    // Исполним всё, что необходимо после инициализации
    //
    finalizeInitialization();
}

void TextModel::clearDocument()
{
    if (!d->rootItem->hasChildren()) {
        return;
    }

    beginResetModelTransaction();
    while (d->rootItem->childCount() > 0) {
        d->rootItem->removeItem(d->rootItem->childAt(0));
    }
    endResetModelTransaction();
}

QByteArray TextModel::toXml() const
{
    return d->toXml(document());
}

ChangeCursor TextModel::applyPatch(const QByteArray& _patch)
{
    Q_ASSERT(document());

#ifdef XML_CHECKS
    const auto newContent = dmpController().applyPatch(toXml(), _patch);
    qDebug(QString("Before applying patch xml is\n\n%1\n\n").arg(toXml().constData()).toUtf8());
    qDebug(QString("Patch is\n\n%1\n\n")
               .arg(QByteArray::fromPercentEncoding(_patch).constData())
               .toUtf8());
#endif

    //
    // Определить область изменения в xml
    //
    auto changes = dmpController().changedXml(toXml(), _patch);
    if (changes.first.xml.isEmpty() && changes.second.xml.isEmpty()) {
        Log::warning("Patch don't lead to any changes");
        return {};
    }

    changes.first.xml = xml::prepareXml(changes.first.xml);
    changes.second.xml = xml::prepareXml(changes.second.xml);

#ifdef XML_CHECKS
    qDebug(QString("Xml data changes first item\n\n%1\n\n")
               .arg(changes.first.xml.constData())
               .toUtf8());
    qDebug(QString("Xml data changes second item\n\n%1\n\n")
               .arg(changes.second.xml.constData())
               .toUtf8());
#endif

    //
    // Считываем элементы из обоих изменений для дальнейшего определения необходимых изменений
    //
    auto readItems = [this](const QString& _xml) -> QVector<TextModelItem*> {
        QXmlStreamReader _reader(_xml);
        xml::readNextElement(_reader); // document
        xml::readNextElement(_reader);

        //
        // Если попался пустой документ
        //
        if (_reader.name() == xml::kDocumentTag) {
            return {};
        }

        QVector<TextModelItem*> items;
        while (!_reader.atEnd()) {
            const auto currentTag = _reader.name().toString();
            TextModelItem* item = nullptr;
            if (textFolderTypeFromString(currentTag) != TextFolderType::Undefined) {
                item = createFolderItem(_reader);
            } else if (textGroupTypeFromString(currentTag) != TextGroupType::Undefined) {
                item = createGroupItem(_reader);
            } else if (currentTag == xml::kSplitterTag) {
                item = createSplitterItem(_reader);
            } else {
                item = createTextItem(_reader);
            }
            items.append(item);

            //
            // Считываем контент до конца
            //
            if (_reader.name() == xml::kDocumentTag) {
                _reader.readNext();
            }
        }

        return items;
    };
    const auto oldItems = readItems(changes.first.xml);
    const auto newItems = readItems(changes.second.xml);

    //
    // Раскладываем элементы в плоские списки для сравнения
    //
    std::function<QVector<TextModelItem*>(const QVector<TextModelItem*>&)> makeItemsPlain;
    makeItemsPlain = [&makeItemsPlain](const QVector<TextModelItem*>& _items) {
        QVector<TextModelItem*> itemsPlain;
        for (auto item : _items) {
            itemsPlain.append(item);
            for (int row = 0; row < item->childCount(); ++row) {
                itemsPlain.append(makeItemsPlain({ item->childAt(row) }));
            }
        }
        return itemsPlain;
    };
    auto oldItemsPlain = makeItemsPlain(oldItems);
    auto newItemsPlain = makeItemsPlain(newItems);

    //
    // Если изменение касается таблицы, но в списке элементов нет закрывающего таблицу элемента,
    // добавим его вручную, чтобы корректно отработал алгоритм корректировки текста
    //
    bool needToAddSplitterEnd = false;
    for (const auto& item : oldItemsPlain) {
        if (item->type() != TextModelItemType::Splitter) {
            continue;
        }

        const auto splitterItem = static_cast<TextModelSplitterItem*>(item);
        needToAddSplitterEnd = splitterItem->splitterType() == TextModelSplitterItemType::Start;
    }
    if (needToAddSplitterEnd) {
        auto splitterEndItem = [this] {
            auto item = createSplitterItem();
            item->setSplitterType(TextModelSplitterItemType::End);
            return item;
        };
        oldItemsPlain.append(splitterEndItem());
        newItemsPlain.append(splitterEndItem());
    }

    //
    // Если элеметов очень много, то обсчитывать все изменения будет очень дорого,
    // поэтому применяем грубую силу - просто накатываем патч и обновляем модель целиком
    //
    const auto operationsLimit = 300;
    if (oldItemsPlain.size() * newItemsPlain.size() / 2 > operationsLimit) {
        Log::trace("Apply patch operations to much, avoid step by step procesing.");

        //
        // Сперва загружаем содержимое из нового XML в модель через документ
        //
        const auto oldContent = document()->content();
        const auto newContent = dmpController().applyPatch(toXml(), _patch);
        clearDocument();
        document()->setContent(newContent);
        initDocument();
        //
        // ... но затем возвращаем предыдущее состояние в документ, чтобы модель могла сформировать
        //     патч осуществлённого измененеия сравним собственное состояние с состоянием документа
        //
        document()->setContent(oldContent);
        return {};
    }

    //
    // Идём по структуре документа до момента достижения начала изменения
    //
    auto length = [this] {
        QByteArray xml = "<?xml version=\"1.0\"?>\n";
        xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type())
            + "\" version=\"1.0\">\n";
        return xml.length();
    }();
    std::function<TextModelItem*(TextModelItem*)> findStartItem;
    findStartItem
        = [this, changes, &length, &findStartItem](TextModelItem* _item) -> TextModelItem* {
        if (changes.first.from == 0) {
            return _item->childAt(0);
        }

        TextModelItem* lastBrokenItem = nullptr;
        QScopedPointer<TextModelTextItem> lastBrokenItemCopy;
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            //
            // Определим дочерний элемент
            //
            auto child = _item->childAt(childIndex);
            if (child->type() == TextModelItemType::Text) {
                auto textItem = static_cast<TextModelTextItem*>(child);
                if (textItem->isCorrection()) {
                    continue;
                }
                if (textItem->isBreakCorrectionStart()) {
                    lastBrokenItem = textItem;
                    lastBrokenItemCopy.reset(new TextModelTextItem(this));
                    lastBrokenItemCopy->copyFrom(lastBrokenItem);
                    continue;
                }
                if (!lastBrokenItemCopy.isNull()) {
                    lastBrokenItemCopy->setText(lastBrokenItemCopy->text() + " ");
                    lastBrokenItemCopy->mergeWith(textItem);
                }
            }
            //
            // Определим длину дочернего элемента
            //
            const auto childLength = lastBrokenItemCopy.isNull()
                ? QString(child->toXml()).length()
                : QString(lastBrokenItemCopy->toXml()).length();

            //
            // В этом элементе начинается изменение
            //
            if (changes.first.from >= length && changes.first.from < length + childLength) {
                //
                // Если есть дети, то уточняем поиск
                //
                int headerLength = 0;
                if (child->type() == TextModelItemType::Folder) {
                    auto folder = static_cast<TextModelFolderItem*>(child);
                    headerLength = QString(folder->xmlHeader()).length();
                } else if (child->type() == TextModelItemType::Group) {
                    auto scene = static_cast<TextModelGroupItem*>(child);
                    headerLength = QString(scene->xmlHeader()).length();
                }

                if (child->hasChildren() && changes.first.from >= length + headerLength) {
                    length += headerLength;
                    return findStartItem(child);
                }
                //
                // В противном случае завершаем поиск
                //
                else {
                    if (lastBrokenItem != nullptr) {
                        return lastBrokenItem;
                    } else {
                        return child;
                    }
                }
            }

            length += childLength;

            if (lastBrokenItem != nullptr) {
                lastBrokenItem = nullptr;
                lastBrokenItemCopy.reset();
            }
        }

        return nullptr;
    };
    auto modelItem = findStartItem(d->rootItem);

    //
    // Если были вставлены сцены или папки при балансировке xml, опустим их
    //
    while (oldItemsPlain.size() > 1 && oldItemsPlain.constFirst()->type() != modelItem->type()) {
        oldItemsPlain.removeFirst();
    }
    while (newItemsPlain.size() > 1 && newItemsPlain.constFirst()->type() != modelItem->type()
           && changes.second.from > 0) {
        newItemsPlain.removeFirst();
    }

    //
    // Подгрузим информацию о родительских элементах, если они были вставлены при балансировке
    //
    if (!oldItemsPlain.isEmpty() && oldItemsPlain.constFirst()->isEqual(modelItem)) {
        auto oldItemParent = oldItemsPlain.first()->parent();
        auto modelItemParent = modelItem->parent();
        while (oldItemParent != nullptr) {
            oldItemParent->copyFrom(modelItemParent);
            oldItemParent = oldItemParent->parent();
            modelItemParent = modelItemParent->parent();
        }
    }
    if (!newItemsPlain.isEmpty() && newItemsPlain.constFirst()->isEqual(modelItem)) {
        auto newItemParent = newItemsPlain.first()->parent();
        auto modelItemParent = modelItem->parent();
        while (newItemParent != nullptr) {
            newItemParent->copyFrom(modelItemParent);
            newItemParent = newItemParent->parent();
            modelItemParent = modelItemParent->parent();
        }
    }

    //
    // Определим необходимые операции для применения изменения
    //
    const auto operations = edit_distance::editDistance(oldItemsPlain, newItemsPlain);
    //
    std::function<TextModelItem*(TextModelItem*, bool)> findNextItemWithChildren;
    findNextItemWithChildren = [&findNextItemWithChildren](
                                   TextModelItem* _item, bool _searchInChildren) -> TextModelItem* {
        if (_item == nullptr) {
            return nullptr;
        }

        if (_searchInChildren) {
            //
            // Если есть дети, идём в дочерний элемент
            //
            if (_item->hasChildren()) {
                return _item->childAt(0);
            }
        }

        //
        // Если детей нет, идём в следующий
        //

        if (!_item->hasParent()) {
            return nullptr;
        }
        auto parent = _item->parent();

        auto itemIndex = parent->rowOfChild(_item);
        if (itemIndex < 0 || itemIndex >= parent->childCount()) {
            return nullptr;
        }

        //
        // Не последний в родителе, берём следующий с этого же уровня
        //
        if (itemIndex < parent->childCount() - 1) {
            return parent->childAt(itemIndex + 1);
        }
        //
        // Последний в родителе, берём следующий с предыдущего уровня
        //
        else {
            return findNextItemWithChildren(parent, false);
        }
    };
    auto findNextItem = [&findNextItemWithChildren](TextModelItem* _item) {
        auto nextItem = findNextItemWithChildren(_item, true);
        //
        // Пропускаем текстовые декорации, т.к. они не сохраняются в модель
        //
        while (nextItem != nullptr && nextItem->type() == TextModelItemType::Text
               && static_cast<TextModelTextItem*>(nextItem)->isCorrection()) {
            nextItem = findNextItemWithChildren(nextItem, true);
        }
        return nextItem;
    };
    //
    // И применяем их
    //
    beginChangeRows();
    TextModelItem* previousModelItem = nullptr;
    //
    // В некоторых ситуациях мы не знаем сразу, куда будут извлечены элементы из удаляемого
    // элемента, или когда элемент вставляется посреди и отрезает часть вложенных элементов, поэтому
    // упаковываем их в список для размещения в правильном месте в следующем проходе
    //
    QVector<TextModelItem*> movedSiblingItems;
    auto updateItemPlacement = [this, &modelItem, &previousModelItem, newItemsPlain,
                                &movedSiblingItems](TextModelItem* _newItem, TextModelItem* _item) {
        //
        // Определим предыдущий элемент из списка новых, в дальнейшем будем опираться
        // на его расположение относительно текущего нового
        //
        const auto newItemIndex = newItemsPlain.indexOf(_newItem);
        TextModelItem* previousNewItem
            = newItemIndex > 0 ? newItemsPlain.at(newItemIndex - 1) : nullptr;
        //
        // У элемента нет родителя, то это вставка нового элемента
        //
        if (!_item->hasParent()) {
            //
            // И это первый из вставляемых
            //
            if (previousNewItem == nullptr) {
                const int modelItemIndex
                    = modelItem ? modelItem->parent()->rowOfChild(modelItem) : 0;
                //
                // Если нужно вставить перед первым элементом, то это случай вставки в начало
                // документа
                //
                if (modelItemIndex == 0) {
                    prependItem(_item);
                }
                //
                // Иначе вставим перед элементом модели
                //
                else {
                    insertItem(_item, modelItem->parent()->childAt(modelItemIndex - 1));
                }
            }
            //
            // А если он не первый из вставляемых
            //
            else {
                Q_ASSERT(previousNewItem->isEqual(previousModelItem));
                //
                // Если у текущего нового и предыдущего нет родителя, то они на одном уровне,
                // вставим после предыдущего
                //
                if ((!_newItem->hasParent() && !previousNewItem->hasParent())
                    || (_newItem->hasParent() && previousNewItem->hasParent()
                        && _newItem->parent() == previousNewItem->parent())) {
                    insertItem(_item, previousModelItem);
                }
                //
                // Если предыдущий новый является родителем текущего
                //
                else if (_newItem->parent() == previousNewItem) {
                    prependItem(_item, previousModelItem);
                }
                //
                // Если у предыдущего есть родитель, то нужно определить смещение
                //
                else {
                    auto previousNewItemParent = previousNewItem->parent()->parent();
                    auto insertAfterItem
                        = previousModelItem
                              ->parent(); // этот на один уровень опаздывает за предыдущим
                    while (previousNewItemParent != _newItem->parent()) {
                        previousNewItemParent = previousNewItemParent->parent();
                        insertAfterItem = insertAfterItem->parent();
                    }

                    insertItem(_item, insertAfterItem);

                    //
                    // И вытаскиваем все последующие элементы на уровень нового, если есть откуда
                    // вытянуть конечно же
                    //
                    if (modelItem != nullptr && modelItem->isChildOf(insertAfterItem)) {
                        auto modelItemParent = modelItem->parent();
                        const int modelItemIndex = modelItemParent->rowOfChild(modelItem);
                        while (modelItemParent->childCount() > modelItemIndex) {
                            auto childItem
                                = modelItemParent->childAt(modelItemParent->childCount() - 1);
                            takeItem(childItem);
                            insertItem(childItem, _item);
                            const auto siblingChildIndex = movedSiblingItems.indexOf(childItem);
                            if (siblingChildIndex == -1) {
                                movedSiblingItems.prepend(childItem);
                            } else if (siblingChildIndex > 0) {
                                movedSiblingItems.move(siblingChildIndex, 0);
                            }
                        }
                    }
                }
            }
        }
        //
        // А если у элемента есть родитель, то это обновление существующего в модели
        //
        else {
            Q_ASSERT(_item->isEqual(modelItem));

            //
            // Первый из обновлённых элементов просто пропускаем
            //
            if (previousNewItem == nullptr) {
                return false;
            }

            //
            // А если это не первый из обновляемых элементов
            //
            Q_ASSERT(previousNewItem->isEqual(previousModelItem));

            //
            // Если должен находиться на том же уровне, что и предыдущий
            //
            if ((!_newItem->hasParent() && !previousNewItem->hasParent())
                || (_newItem->hasParent() && previousNewItem->hasParent()
                    && _newItem->parent() == previousNewItem->parent())) {
                //
                // ... и находится, то ничего не делаем
                //
                if (_item->parent()->isEqual(previousModelItem->parent())) {
                    return false;
                }

                //
                // ... а если не находится, то корректируем
                //
                takeItem(_item);
                insertItem(_item, previousModelItem);
            }
            //
            // Если предыдущий должен быть родителем текущего
            //
            else if (_newItem->parent() == previousNewItem) {
                //
                // ... и является, то ничего не делаем
                //
                if (_item->parent() == previousModelItem) {
                    return false;
                }

                //
                // ... а если родитель, другой, то просто перемещаем элемент внутрь предыдушего
                //
                takeItem(_item);
                appendItem(_item, previousModelItem);
            }
            //
            // Если должен находиться на разных уровнях
            //
            else {
                auto previousNewItemParent = previousNewItem->parent()->parent();
                auto insertAfterItem
                    = previousModelItem->parent(); // этот на один уровень опаздывает за предыдущим
                while (previousNewItemParent != _newItem->parent()) {
                    previousNewItemParent = previousNewItemParent->parent();
                    insertAfterItem = insertAfterItem->parent();
                }

                //
                // ... и находится по месту, то ничего не делаем
                //
                if (_item->parent()->isEqual(insertAfterItem->parent())) {
                    return false;
                }

                //
                // ... а если не там где должен быть, то корректируем структуру
                //

                auto itemParent = _item->parent();
                const int itemIndex = itemParent->rowOfChild(_item);

                takeItem(_item);
                insertItem(_item, insertAfterItem);

                //
                // И вытаскиваем все последующие текстовые элементы в модели на уровень вставки
                //
                while (itemParent->childCount() > itemIndex) {
                    auto childItem = itemParent->childAt(itemParent->childCount() - 1);
                    if (childItem->type() != TextModelItemType::Text) {
                        break;
                    }

                    takeItem(childItem);
                    insertItem(childItem, _item);
                    const auto siblingChildIndex = movedSiblingItems.indexOf(childItem);
                    if (siblingChildIndex == -1) {
                        movedSiblingItems.prepend(childItem);
                    } else if (siblingChildIndex > 0) {
                        movedSiblingItems.move(siblingChildIndex, 0);
                    }
                }
            }
        }

        //
        // Если у нас в буфере есть перенесённые элементы и текущий является их предводителем
        //
        if (!movedSiblingItems.isEmpty() && movedSiblingItems.constFirst() == _item) {
            //
            // Удалим сам якорный элемент
            //
            movedSiblingItems.removeFirst();
            //
            // То перенесём их в след за предводителем
            //
            for (auto siblingItem : reversed(movedSiblingItems)) {
                takeItem(siblingItem);
                insertItem(siblingItem, _item);
            }
            //
            // и очистим список для будущих свершений
            //
            movedSiblingItems.clear();
        }

        return true;
    };

    TextModelItem* lastChangedItem = nullptr;
    int lastChangedItemPosition = -1;
    for (const auto& operation : operations) {
        //
        // Если текущий элемент модели разбит на несколько абзацев, нужно его склеить
        //
        if (modelItem != nullptr && modelItem->type() == TextModelItemType::Text) {
            auto textItem = static_cast<TextModelTextItem*>(modelItem);
            if (textItem->isBreakCorrectionStart()) {
                auto nextItem = findNextItemWithChildren(textItem, false);
                while (nextItem != nullptr && nextItem->type() == TextModelItemType::Text) {
                    auto nextTextItem = static_cast<TextModelTextItem*>(nextItem);
                    if (nextTextItem->isCorrection()) {
                        auto itemToRemove = nextItem;
                        nextItem = findNextItemWithChildren(nextItem, false);
                        removeItem(itemToRemove);
                        continue;
                    }

                    textItem->setText(textItem->text() + " ");
                    textItem->mergeWith(nextTextItem);
                    textItem->setBreakCorrectionStart(false);
                    updateItem(textItem);
                    removeItem(nextItem);
                    break;
                }
            }
        }

        //
        // Собственно применяем операции
        //
        auto newItem = operation.value;
        switch (operation.type) {
        case edit_distance::OperationType::Skip: {
            //
            // Корректируем позицию
            //
            updateItemPlacement(newItem, modelItem);
            //
            // ... и просто переходим к следующему элементу
            //
            previousModelItem = modelItem;
            modelItem = findNextItem(modelItem);
            break;
        }

        case edit_distance::OperationType::Remove: {
            //
            // Выносим детей на предыдущий уровень
            //
            if (modelItem->hasChildren()) {
                QVector<BusinessLayer::TextModelItem*> childItems;
                for (int index = 0; index < modelItem->childCount(); ++index) {
                    auto childItem = modelItem->childAt(index);
                    childItems.append(childItem);
                }

                takeItems(childItems.constFirst(), childItems.constLast(), modelItem);
                insertItems(childItems, modelItem);

                movedSiblingItems.append(childItems);
            }
            //
            // ... и удаляем сам элемент
            //
            auto nextItem = findNextItem(modelItem);
            //
            // ... в кейсе когда удаляется группирующий элемент и его содержимое, содержимое
            // находится во временном списке, для перемещения за пределы удаляемой группы, но если
            // этот элемент таки нужно удалить, удалим его и из временного списка тоже
            //
            if (const auto index = movedSiblingItems.indexOf(modelItem); index != -1) {
                movedSiblingItems.removeAt(index);
            }
            removeItem(modelItem);
            modelItem = nextItem;
            break;
        }

        case edit_distance::OperationType::Insert: {
            //
            // Создаём новый элемент
            //
            TextModelItem* itemToInsert = nullptr;
            switch (newItem->type()) {
            case TextModelItemType::Folder: {
                itemToInsert
                    = createFolderItem(static_cast<TextModelFolderItem*>(newItem)->folderType());
                break;
            }

            case TextModelItemType::Group: {
                itemToInsert
                    = createGroupItem(static_cast<TextModelGroupItem*>(newItem)->groupType());
                break;
            }

            case TextModelItemType::Text: {
                itemToInsert = createTextItem();
                break;
            }

            case TextModelItemType::Splitter: {
                auto splitterItem = createSplitterItem();
                splitterItem->setSplitterType(
                    static_cast<TextModelSplitterItem*>(newItem)->splitterType());
                itemToInsert = splitterItem;
                break;
            }
            }
            itemToInsert->copyFrom(newItem);

            //
            // ... и вставляем в нужного родителя
            //
            updateItemPlacement(newItem, itemToInsert);

            previousModelItem = itemToInsert;
            break;
        }

        case edit_distance::OperationType::Replace: {
            //
            // Обновляем элемент
            //
            Q_ASSERT(modelItem->type() == newItem->type());
            if (!modelItem->isEqual(newItem)) {
                if (modelItem->type() == TextModelItemType::Text) {
                    const auto modelTextItem = static_cast<TextModelTextItem*>(modelItem);
                    const auto newTextItem = static_cast<TextModelTextItem*>(newItem);
                    lastChangedItemPosition = dmpController().changeEndPosition(
                        modelTextItem->text(), newTextItem->text());
                }
                modelItem->copyFrom(newItem);
            }

            auto nextItem = findNextItem(modelItem);

            //
            // Если элемент был перемещён, скорректируем его позицию
            //
            const auto isPlacementChanged = updateItemPlacement(newItem, modelItem);
            //
            // Если обновляемый элемент находился в списке элементов на перемещение, удалим его
            // оттуда, т.к. он уже был обработан и расположен в правильном месте
            //
            if (const auto index = movedSiblingItems.indexOf(modelItem); index != -1) {
                movedSiblingItems.removeAt(index);
            }
            //
            // В противном случае просто обновим его в модели
            //
            if (!isPlacementChanged) {
                updateItem(modelItem);
            }

            previousModelItem = modelItem;
            modelItem = nextItem;
            break;
        }
        }

        //
        // Сохраняем последнего изменённого, если было обработано изменение
        //
        if (operation.type != edit_distance::OperationType::Skip) {
            lastChangedItem = previousModelItem;
            if (operation.type != edit_distance::OperationType::Replace) {
                lastChangedItemPosition = -1;
            }
        }
    }

    //
    // Если у нас в буфере есть перенесённые элементы и текущий является их предводителем
    //
    if (!movedSiblingItems.isEmpty() && previousModelItem != nullptr
        && movedSiblingItems.constFirst()->parent() != previousModelItem->parent()) {
        for (auto siblingItem : reversed(movedSiblingItems)) {
            takeItem(siblingItem);
            insertItem(siblingItem, previousModelItem);
        }
    }

    qDeleteAll(oldItems);
    qDeleteAll(newItems);

    endChangeRows();

#ifdef XML_CHECKS
    //
    // Делаем проверку на соответствие обновлённой модели прямому наложению патча
    //
    if (newContent != toXml()) {
        qDebug(QString("New content should be\n\n%1\n\n").arg(newContent.constData()).toUtf8());
        qDebug(QString("New content is\n\n%1\n\n").arg(toXml().constData()).toUtf8());
    }
    Q_ASSERT(newContent == toXml());
#endif

    return { lastChangedItem, lastChangedItemPosition };
}

} // namespace BusinessLayer
