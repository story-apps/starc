#include "text_model.h"

#include "text_model_folder_item.h"
#include "text_model_group_item.h"
#include "text_model_splitter_item.h"
#include "text_model_text_item.h"
#include "text_model_xml.h"
#include "text_model_xml_writer.h"

#include <business_layer/templates/text_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>
#include <utils/shugar.h>
#include <utils/tools/edit_distance.h>
#include <utils/tools/model_index_path.h>

#include <QDateTime>
#include <QMimeData>
#include <QRegularExpression>
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
     * @brief Последние скопированные данные модели
     */
    struct {
        QModelIndex from;
        QModelIndex to;
        QMimeData* data = nullptr;
    } lastMime;
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

    const bool addXMlHeader = true;
    xml::TextModelXmlWriter xml(addXMlHeader);
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(_document->type())
        + "\" version=\"1.0\">\n";
    for (int childIndex = 0; childIndex < rootItem->childCount(); ++childIndex) {
        xml += rootItem->childAt(childIndex);
    }
    xml += "</document>";
    return xml.data();
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
            toString(TextGroupType::Chapter),
            toString(TextParagraphType::UnformattedText),
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
            toString(TextParagraphType::InlineNote),
            toString(TextParagraphType::ActHeading),
            toString(TextParagraphType::ActFooter),
            toString(TextParagraphType::SequenceHeading),
            toString(TextParagraphType::SequenceFooter),
            toString(TextParagraphType::PageSplitter),
        },
        _parent)
    , d(new Implementation(this, _rootItem))
{
}

TextModel::~TextModel() = default;

TextModelFolderItem* TextModel::createFolderItem(QXmlStreamReader& _contentReader) const
{
    auto item = createFolderItem();
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

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem;
    }

    const QModelIndex parentIndex = indexForItem(_parentItem);
    const int fromItemRow = _parentItem->childCount();
    const int toItemRow = fromItemRow + _items.size() - 1;
    beginInsertRows(parentIndex, fromItemRow, toItemRow);
    _parentItem->appendItems({ _items.begin(), _items.end() });
    endInsertRows();

    updateItem(_parentItem);
}

void TextModel::prependItem(TextModelItem* _item, TextModelItem* _parentItem)
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
    _parentItem->prependItem(_item);
    endInsertRows();

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

    auto parentItem = _afterSiblingItem->parent();
    const QModelIndex parentIndex = indexForItem(parentItem);
    const int fromItemRow = parentItem->rowOfChild(_afterSiblingItem) + 1;
    const int toItemRow = fromItemRow + _items.size() - 1;
    beginInsertRows(parentIndex, fromItemRow, toItemRow);
    parentItem->insertItems(fromItemRow, { _items.begin(), _items.end() });
    endInsertRows();

    updateItem(parentItem);
}

void TextModel::takeItem(TextModelItem* _item, TextModelItem* _parentItem)
{
    takeItems(_item, _item, _parentItem);
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

    const QModelIndex parentItemIndex = indexForItem(_fromItem).parent();
    const int fromItemRow = _parentItem->rowOfChild(_fromItem);
    const int toItemRow = _parentItem->rowOfChild(_toItem);
    Q_ASSERT(fromItemRow <= toItemRow);
    beginRemoveRows(parentItemIndex, fromItemRow, toItemRow);
    _parentItem->takeItems(fromItemRow, toItemRow);
    endRemoveRows();

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

    auto parentItem = _fromItem->parent();
    const QModelIndex itemParentIndex = indexForItem(_fromItem).parent();
    const int fromItemRow = parentItem->rowOfChild(_fromItem);
    const int toItemRow = parentItem->rowOfChild(_toItem);
    Q_ASSERT(fromItemRow <= toItemRow);
    beginRemoveRows(itemParentIndex, fromItemRow, toItemRow);
    parentItem->removeItems(fromItemRow, toItemRow);
    endRemoveRows();

    updateItem(parentItem);
}

void TextModel::updateItem(TextModelItem* _item)
{
    if (_item == nullptr || !_item->isChanged()) {
        return;
    }

    const QModelIndex indexForUpdate = indexForItem(_item);
    emit dataChanged(indexForUpdate, indexForUpdate);
    _item->setChanged(false);

    if (_item->parent() != nullptr) {
        updateItem(_item->parent());
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
        emit rowsAboutToBeChanged();

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
        emit rowsChanged();

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
    // Для того, чтобы запретить разрывать папки проверяем выделены ли элементы одного уровня
    //
    bool itemsHaveSameParent = true;
    const QModelIndex& genericParent = correctedIndexes.first().parent();
    for (const auto& index : correctedIndexes) {
        if (index.parent() != genericParent) {
            itemsHaveSameParent = false;
            break;
        }
    }
    if (!itemsHaveSameParent) {
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


    const bool addXMlHeader = true;
    xml::TextModelXmlWriter xml(addXMlHeader);
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type())
        + "\" version=\"1.0\">\n";

    auto buildXmlFor = [&xml, fromItem, _fromPosition, toItem, _toPosition,
                        _clearUuid](TextModelItem* _fromItemParent, int _fromItemRow) {
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
                if (textItem->paragraphType() == TextParagraphType::SequenceFooter) {
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
                return true;
            }
        }

        return false;
    };
    auto fromItemParent = fromItem->parent();
    auto fromItemRow = fromItemParent->rowOfChild(fromItem);
    //
    // Если построить нужно начиная с заголовка сцены или папки, то нужно захватить и саму
    // сцену/папку
    //
    if (fromItem->type() == TextModelItemType::Text) {
        const auto textItem = static_cast<TextModelTextItem*>(fromItem);
        if (textItem->paragraphType() == TextParagraphType::SceneHeading
            || textItem->paragraphType() == TextParagraphType::BeatHeading
            || textItem->paragraphType() == TextParagraphType::PageHeading
            || textItem->paragraphType() == TextParagraphType::PanelHeading
            || textItem->paragraphType() == TextParagraphType::SequenceHeading
            || textItem->paragraphType() == TextParagraphType::ActHeading) {
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

void TextModel::insertFromMime(const QModelIndex& _index, int _positionInBlock,
                               const QString& _mimeData)
{
    if (!_index.isValid()) {
        return;
    }

    if (_mimeData.isEmpty()) {
        return;
    }

    //
    // Начинаем операцию изменения модели
    //
    emit rowsAboutToBeChanged();

    //
    // Определим элемент, внутрь, или после которого будем вставлять данные
    //
    auto item = itemForIndex(_index);

    //
    // Извлекаем остающийся в блоке текст, если нужно
    //
    QString sourceBlockEndContent;
    QVector<TextModelItem*> lastItemsFromSourceScene;
    if (item->type() == TextModelItemType::Text) {
        auto textItem = static_cast<TextModelTextItem*>(item);
        //
        // Если в заголовок папки
        //
        if (textItem->paragraphType() == TextParagraphType::SequenceHeading) {
            //
            // ... то вставим после него
            //
        }
        //
        // Если завершение папки
        //
        else if (textItem->paragraphType() == TextParagraphType::SequenceFooter) {
            //
            // ... то вставляем после папки
            //
            item = item->parent();
        }
        //
        // В остальных случаях
        //
        else {
            //
            // Если вставка идёт в самое начало блока, то просто переносим блок после вставляемого
            // фрагмента
            //
            if (textItem->text().isEmpty()) {
                lastItemsFromSourceScene.append(textItem);
            }
            //
            // В противном случае, дробим блок на две части
            //
            else if (textItem->text().length() > _positionInBlock) {
                const bool clearUuid = true;
                sourceBlockEndContent = mimeFromSelection(_index, _positionInBlock, _index,
                                                          textItem->text().length(), clearUuid);
                textItem->removeText(_positionInBlock);
                updateItem(textItem);
            }
        }
    } else {
        Log::warning(
            "Trying to insert from mime to position with no text item. Aborting insertion.");
        return;
    }

    //
    // Считываем данные и последовательно вставляем в модель
    //
    QXmlStreamReader contentReader(_mimeData);
    contentReader.readNextStartElement(); // document
    contentReader.readNextStartElement();
    bool isFirstTextItemHandled = false;
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
    while (!contentReader.atEnd()) {
        const auto currentTag = contentReader.name().toString();

        //
        // Если дошли до конца
        //
        if (currentTag == xml::kDocumentTag) {
            //
            // ... поместим в модель, все собранные элементы
            //
            insertCollectedItems();
            break;
        }

        TextModelItem* newItem = nullptr;
        //
        // При входе в папку или группу, если предыдущий текстовый элемент был в группе,
        // то вставлять их будем не после текстового элемента, а после группы
        //
        if ((textFolderTypeFromString(currentTag) != TextFolderType::Undefined
             || textGroupTypeFromString(currentTag) != TextGroupType::Undefined)
            && (lastItem->type() == TextModelItemType::Text
                || lastItem->type() == TextModelItemType::Splitter)) {
            //
            // ... вставим в модель, всё, что было собрано до этого момента
            //
            insertCollectedItems();

            //
            // ... родитель предыдущего элемента должен существовать и это должна быть группа
            //
            if (lastItem && lastItem->parent() != nullptr
                && lastItem->parent()->type() == TextModelItemType::Group) {
                //
                // ... и при этом вырезаем из него все текстовые блоки, идущие до конца группы/папки
                //
                auto lastItemParent = lastItem->parent();
                int movedItemIndex = lastItemParent->rowOfChild(lastItem) + 1;
                while (lastItemParent->childCount() > movedItemIndex) {
                    lastItemsFromSourceScene.append(lastItemParent->childAt(movedItemIndex));
                    ++movedItemIndex;
                }
                //
                // Собственно берём родителя вместо самого элемента
                //
                lastItem = lastItemParent;
                insertAfterItem = lastItemParent;
            }
        }


        if (textFolderTypeFromString(currentTag) != TextFolderType::Undefined) {
            newItem = createFolderItem(contentReader);
        } else if (textGroupTypeFromString(currentTag) != TextGroupType::Undefined) {
            auto newGroupItem = createGroupItem(contentReader);

            //
            // Если группа вставляется после группы, учитываем уровень группы, и при необходимости
            // поднимаемся ещё на уровень выше
            //
            if (insertAfterItem->type() == TextModelItemType::Group) {
                auto groupItem = static_cast<TextModelGroupItem*>(insertAfterItem);
                while (groupItem != nullptr && groupItem->level() > newGroupItem->level()) {
                    insertAfterItem = insertAfterItem->parent();
                    if (insertAfterItem != nullptr
                        && insertAfterItem->type() != TextModelItemType::Group) {
                        break;
                    }

                    groupItem = static_cast<TextModelGroupItem*>(insertAfterItem);
                };
            }

            newItem = newGroupItem;
        } else if (currentTag == xml::kSplitterTag) {
            newItem = createSplitterItem(contentReader);
        } else {
            auto newTextItem = createTextItem(contentReader);
            //
            // Если вставляется текстовый элемент внутрь уже существующего элемента
            //
            if (!isFirstTextItemHandled) {
                isFirstTextItemHandled = true;
                //
                // ... то просто объединим их
                //
                if (item->type() == TextModelItemType::Text
                    && !lastItemsFromSourceScene.contains(item)) {
                    auto textItem = static_cast<TextModelTextItem*>(item);
                    textItem->mergeWith(newTextItem);
                    updateItem(textItem);
                    delete newTextItem;
                    //
                    // ... и исключаем исходный блок из переноса, если он был туда помещён
                    //
                    lastItemsFromSourceScene.removeAll(textItem);
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

    //
    // Если есть оторванный от первого блока текст
    //
    if (!sourceBlockEndContent.isEmpty()) {
        contentReader.clear();
        contentReader.addData(sourceBlockEndContent);
        contentReader.readNextStartElement(); // document
        contentReader.readNextStartElement(); // potential text node
        if (textFolderTypeFromString(contentReader.name().toString()) != TextFolderType::Undefined
            || textGroupTypeFromString(contentReader.name().toString())
                != TextGroupType::Undefined) {
            contentReader.readNextStartElement(); // content
            contentReader.readNextStartElement(); // text node
        }
        auto item = createTextItem(contentReader);
        //
        // ... и последний вставленный элемент был текстовым
        //
        if (lastItem->type() == TextModelItemType::Text) {
            auto lastTextItem = static_cast<TextModelTextItem*>(lastItem);

            //
            // Объединим элементы
            //
            lastTextItem->mergeWith(item);
            updateItem(lastTextItem);
            delete item;
        }
        //
        // В противном случае, вставляем текстовый элемент после последнего вставленного
        //
        else {
            appendItem(item, lastItem);
            lastItem = item;
        }
    }

    //
    // Если есть оторванные текстовые блоки
    //
    if (!lastItemsFromSourceScene.isEmpty()) {
        //
        // Извлечём блоки из родителя
        //
        for (auto item : lastItemsFromSourceScene) {
            if (item->hasParent()) {
                auto itemParent = item->parent();
                takeItem(item, itemParent);

                //
                // Удалим родителя, если у него больше не осталось детей
                // NOTE: актуально для случая, когда в сцене был один пустой абзац заголовка
                //
                if (itemParent->childCount() == 0) {
                    removeItem(itemParent);
                }
            }
        }

        //
        // Просто вставляем их внутрь или после последнего элемента
        //
        for (auto item : lastItemsFromSourceScene) {
            auto textItem = static_cast<TextModelTextItem*>(item);
            //
            // Удаляем пустые элементы модели
            //
            if (textItem->text().isEmpty()) {
                delete textItem;
                textItem = nullptr;
                continue;
            }

            if (lastItem->type() == TextModelItemType::Group) {
                appendItem(item, lastItem);
            } else {
                insertItem(item, lastItem);
            }
            lastItem = item;
        }
    }

    //
    // Завершаем изменение
    //
    emit rowsChanged();
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
    return d->titlePageModel;
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
        beginResetModel();
        d->buildModel(document());
        endResetModel();
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

    beginResetModel();
    while (d->rootItem->childCount() > 0) {
        d->rootItem->removeItem(d->rootItem->childAt(0));
    }
    endResetModel();
}

QByteArray TextModel::toXml() const
{
    return d->toXml(document());
}

void TextModel::applyPatch(const QByteArray& _patch)
{
    Q_ASSERT(document());

#ifdef XML_CHECKS
    const auto newContent = dmpController().applyPatch(toXml(), _patch);
#endif

    //
    // Определить область изменения в xml
    //
    auto changes = dmpController().changedXml(toXml(), _patch);
    if (changes.first.xml.isEmpty() && changes.second.xml.isEmpty()) {
        Log::warning("Patch don't lead to any changes");
        return;
    }

    changes.first.xml = xml::prepareXml(changes.first.xml);
    changes.second.xml = xml::prepareXml(changes.second.xml);

#ifdef XML_CHECKS
    qDebug(changes.first.xml);
    qDebug("************************");
    qDebug(changes.second.xml);
    qDebug("\n\n\n");
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
    // Если элеметов очень много, то обсчитывать все изменения будет очень дорого,
    // поэтому применяем грубую силу - просто накатываем патч и обновляем модель целиком
    //
    const auto operationsLimit = 1000;
    if (oldItemsPlain.size() * newItemsPlain.size() / 2 > operationsLimit) {
        const auto newContent = dmpController().applyPatch(toXml(), _patch);
        clearDocument();
        document()->setContent(newContent);
        initDocument();
        //        beginResetModel();
        //        endResetModel();
        return;
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
    if (oldItemsPlain.constFirst()->isEqual(modelItem)) {
        auto oldItemParent = oldItemsPlain.first()->parent();
        auto modelItemParent = modelItem->parent();
        while (oldItemParent != nullptr) {
            oldItemParent->copyFrom(modelItemParent);
            oldItemParent = oldItemParent->parent();
            modelItemParent = modelItemParent->parent();
        }
    }
    if (newItemsPlain.constFirst()->isEqual(modelItem)) {
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
    emit rowsAboutToBeChanged();
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
                const int modelItemIndex = modelItem->parent()->rowOfChild(modelItem);
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
                    if (modelItem != nullptr && modelItem->parent() != _item->parent()) {
                        auto modelItemParent = modelItem->parent();
                        const int modelItemIndex = modelItemParent->rowOfChild(modelItem);
                        while (modelItemParent->childCount() > modelItemIndex) {
                            auto childItem
                                = modelItemParent->childAt(modelItemParent->childCount() - 1);
                            takeItem(childItem, modelItemParent);
                            insertItem(childItem, _item);
                            movedSiblingItems.prepend(childItem);
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
                takeItem(_item, _item->parent());
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
                takeItem(_item, _item->parent());
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

                takeItem(_item, itemParent);
                insertItem(_item, insertAfterItem);

                //
                // И вытаскиваем все последующие текстовые элементы в модели на уровень вставки
                //
                while (itemParent->childCount() > itemIndex) {
                    auto childItem = itemParent->childAt(itemParent->childCount() - 1);
                    if (childItem->type() != TextModelItemType::Text) {
                        break;
                    }

                    takeItem(childItem, itemParent);
                    insertItem(childItem, _item);
                    movedSiblingItems.prepend(childItem);
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
                takeItem(siblingItem, siblingItem->parent());
                insertItem(siblingItem, _item);
            }
            //
            // и очистим список для будущих свершений
            //
            movedSiblingItems.clear();
        }

        return true;
    };


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
            while (modelItem->hasChildren()) {
                auto childItem = modelItem->childAt(modelItem->childCount() - 1);
                takeItem(childItem, modelItem);
                insertItem(childItem, modelItem);
                movedSiblingItems.prepend(childItem);
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
                itemToInsert = createFolderItem();
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
                modelItem->copyFrom(newItem);
            }

            auto nextItem = findNextItem(modelItem);

            //
            // Если элемент был перемещён, скорректируем его позицию
            //
            const auto isPlacementChanged = updateItemPlacement(newItem, modelItem);
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
    }

    qDeleteAll(oldItems);
    qDeleteAll(newItems);

    emit rowsChanged();

#ifdef XML_CHECKS
    //
    // Делаем проверку на соответствие обновлённой модели прямому наложению патча
    //
    if (newContent != toXml()) {
        qDebug(newContent);
        qDebug("\n\n************************\n\n");
        qDebug(qUtf8Printable(QByteArray::fromPercentEncoding(_patch)));
        qDebug("\n\n************************\n\n");
        qDebug(toXml());
        qDebug("\n\n\n");
    }
    Q_ASSERT(newContent == toXml());
#endif
}

} // namespace BusinessLayer
