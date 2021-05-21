#include "screenplay_text_model.h"

#include "screenplay_text_block_parser.h"
#include "screenplay_text_model_folder_item.h"
#include "screenplay_text_model_scene_item.h"
#include "screenplay_text_model_splitter_item.h"
#include "screenplay_text_model_text_item.h"
#include "screenplay_text_model_xml.h"
#include "screenplay_text_model_xml_writer.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/templates/screenplay_template.h>

#include <domain/document_object.h>

#include <utils/shugar.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/edit_distance.h>
#include <utils/tools/model_index_path.h>

#include <QMimeData>
#include <QXmlStreamReader>

#ifdef QT_DEBUG
#define XML_CHECKS
#endif

namespace BusinessLayer
{

namespace {
const char* kMimeType = "application/x-starc/screenplay/text/item";
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
     * @brief Модель информации о проекте
     */
    ScreenplayInformationModel* informationModel = nullptr;

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

    /**
     * @brief Последние скопированные данные модели
     */
    struct {
        QModelIndex from;
        QModelIndex to;
        QMimeData* data = nullptr;
    } lastMime;
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

    QXmlStreamReader contentReader(_screenplay->content());
    contentReader.readNextStartElement(); // document
    contentReader.readNextStartElement();
    while (!contentReader.atEnd()) {
        const auto currentTag = contentReader.name();
        if (currentTag == xml::kDocumentTag) {
            break;
        }

        if (currentTag == xml::kFolderTag) {
            rootItem->appendItem(new ScreenplayTextModelFolderItem(contentReader));
        } else if (currentTag == xml::kSceneTag) {
            rootItem->appendItem(new ScreenplayTextModelSceneItem(contentReader));
        } else if (currentTag == xml::kSplitterTag) {
            rootItem->appendItem(new ScreenplayTextModelSplitterItem(contentReader));
        } else {
            rootItem->appendItem(new ScreenplayTextModelTextItem(contentReader));
        }
    }
}

QByteArray ScreenplayTextModel::Implementation::toXml(Domain::DocumentObject* _screenplay) const
{
    if (_screenplay == nullptr) {
        return {};
    }

    const bool addXMlHeader = true;
    xml::ScreenplayTextModelXmlWriter xml(addXMlHeader);
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(_screenplay->type()) + "\" version=\"1.0\">\n";
    for (int childIndex = 0; childIndex < rootItem->childCount(); ++childIndex) {
        xml += rootItem->childAt(childIndex);
    }
    xml += "</document>";
    return xml.data();
}

void ScreenplayTextModel::Implementation::updateNumbering()
{
    int sceneNumber = informationModel->scenesNumberingStartAt();
    int dialogueNumber = 1;
    std::function<void(const ScreenplayTextModelItem*)> updateChildNumbering;
    updateChildNumbering = [this, &sceneNumber, &dialogueNumber, &updateChildNumbering] (const ScreenplayTextModelItem* _item) {
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
                    sceneItem->setNumber(sceneNumber++, informationModel->scenesNumbersPrefix());
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
    : AbstractModel({ xml::kDocumentTag,
                      xml::kFolderTag,
                      xml::kSceneTag,
                      toString(ScreenplayParagraphType::UnformattedText),
                      toString(ScreenplayParagraphType::SceneHeading),
                      toString(ScreenplayParagraphType::SceneCharacters),
                      toString(ScreenplayParagraphType::Action),
                      toString(ScreenplayParagraphType::Character),
                      toString(ScreenplayParagraphType::Parenthetical),
                      toString(ScreenplayParagraphType::Dialogue),
                      toString(ScreenplayParagraphType::Lyrics),
                      toString(ScreenplayParagraphType::Transition),
                      toString(ScreenplayParagraphType::Shot),
                      toString(ScreenplayParagraphType::InlineNote),
                      toString(ScreenplayParagraphType::FolderHeader),
                      toString(ScreenplayParagraphType::FolderFooter),
                      toString(ScreenplayParagraphType::PageSplitter) },
                    _parent),
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

    updateItem(_parentItem);
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
    _parentItem->prependItem(_item);
    d->updateNumbering();
    endInsertRows();

    updateItem(_parentItem);
}

void ScreenplayTextModel::insertItem(ScreenplayTextModelItem* _item, ScreenplayTextModelItem* _afterSiblingItem)
{
    if (_item == nullptr
        || _afterSiblingItem == nullptr
        || _afterSiblingItem->parent() == nullptr) {
        return;
    }

    auto parentItem = _afterSiblingItem->parent();
    if (parentItem->hasChild(_item)) {
        return;
    }

    const QModelIndex parentIndex = indexForItem(parentItem);
    const int itemRowIndex = parentItem->rowOfChild(_afterSiblingItem) + 1;
    beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
    parentItem->insertItem(itemRowIndex, _item);
    d->updateNumbering();
    endInsertRows();

    updateItem(parentItem);
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

    const QModelIndex parentItemIndex = indexForItem(_item).parent();
    const int itemRowIndex = _parentItem->rowOfChild(_item);
    beginRemoveRows(parentItemIndex, itemRowIndex, itemRowIndex);
    _parentItem->takeItem(_item);
    d->updateNumbering();
    endRemoveRows();

    updateItem(_parentItem);
}

void ScreenplayTextModel::removeItem(ScreenplayTextModelItem* _item)
{
    if (_item == nullptr
        || _item->parent() == nullptr) {
        return;
    }

    auto parentItem = _item->parent();
    const QModelIndex itemParentIndex = indexForItem(_item).parent();
    const int itemRowIndex = parentItem->rowOfChild(_item);
    beginRemoveRows(itemParentIndex, itemRowIndex, itemRowIndex);
    parentItem->removeItem(_item);
    d->updateNumbering();
    endRemoveRows();

    updateItem(parentItem);
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
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    const auto item = itemForIndex(_index);
    switch (item->type()) {
        case ScreenplayTextModelItemType::Folder: {
            flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
            break;
        }

        case ScreenplayTextModelItemType::Scene: {
            flags |= Qt::ItemIsDragEnabled;
            break;
        }

        default: break;
    }

    return flags;
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
    Q_UNUSED(_action);
    Q_UNUSED(_row);
    Q_UNUSED(_column);
    Q_UNUSED(_parent);

    return _data->formats().contains(mimeTypes().constFirst());
}

bool ScreenplayTextModel::dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column, const QModelIndex& _parent)
{
    Q_UNUSED(_column);

    //
    // _row - индекс, куда вставлять, если в папку, то он равен -1 и если в самый низ списка, то он тоже равен -1
    //

    if (_data == 0
        || !canDropMimeData(_data, _action, _row, _column, _parent)) {
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
                if (_parent.isValid()
                    && rowCount(_parent) == _row) {
                    //
                    // ... для папок, при вставке в самый конец также нужно учитывать
                    //     текстовый блок закрывающий папку
                    //
                    ++delta;
                }
                insertAnchorIndex = index(_row - delta, 0, _parent);
            }
            if (d->lastMime.from == insertAnchorIndex
                || d->lastMime.to == insertAnchorIndex) {
                return false;
            }
            ScreenplayTextModelItem* insertAnchorItem = itemForIndex(insertAnchorIndex);

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
            ScreenplayTextModelItem* lastItem = insertAnchorItem;
            while (!contentReader.atEnd()) {
                const auto currentTag = contentReader.name();
                if (currentTag == xml::kDocumentTag) {
                    break;
                }

                ScreenplayTextModelItem* newItem = nullptr;
                if (currentTag == xml::kFolderTag) {
                    newItem = new ScreenplayTextModelFolderItem(contentReader);
                } else if (currentTag == xml::kSceneTag) {
                    newItem = new ScreenplayTextModelSceneItem(contentReader);
                } else if (currentTag == xml::kSplitterTag) {
                    newItem = new ScreenplayTextModelSplitterItem(contentReader);
                } else {
                    newItem = new ScreenplayTextModelTextItem(contentReader);
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
                        if (lastItem->type() == ScreenplayTextModelItemType::Folder
                            && _parent.isValid()) {
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
                        if (lastItem->type() == ScreenplayTextModelItemType::Folder
                                && _parent.isValid()) {
                            insertItem(newItem, lastItem->childAt(lastItem->childCount() - 2));
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

QMimeData* ScreenplayTextModel::mimeData(const QModelIndexList& _indexes) const
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
    mimeData->setData(mimeTypes().constFirst(), mimeFromSelection(fromIndex, 0, toIndex, 1, clearUuid).toUtf8());

    d->lastMime = { fromIndex, toIndex, mimeData };

    return mimeData;
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

QString ScreenplayTextModel::mimeFromSelection(const QModelIndex& _from, int _fromPosition,
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
    xml::ScreenplayTextModelXmlWriter xml(addXMlHeader);
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type()) + "\" version=\"1.0\">\n";

    auto buildXmlFor = [&xml, fromItem, _fromPosition, toItem, _toPosition, _clearUuid]
                       (ScreenplayTextModelItem* _fromItemParent, int _fromItemRow)
    {
        for (int childIndex = _fromItemRow; childIndex < _fromItemParent->childCount(); ++childIndex) {
            const auto childItem = _fromItemParent->childAt(childIndex);

            switch (childItem->type()) {
                case ScreenplayTextModelItemType::Folder: {
                    const auto folderItem = static_cast<ScreenplayTextModelFolderItem*>(childItem);
                    xml += folderItem->toXml(fromItem, _fromPosition, toItem, _toPosition, _clearUuid);
                    break;
                }

                case ScreenplayTextModelItemType::Scene: {
                    const auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(childItem);
                    xml += sceneItem->toXml(fromItem, _fromPosition, toItem, _toPosition, _clearUuid);
                    break;
                }

                case ScreenplayTextModelItemType::Text: {
                    const auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);

                    //
                    // Не сохраняем закрывающие блоки неоткрытых папок, всё это делается внутри самих папок
                    //
                    if (textItem->paragraphType() == ScreenplayParagraphType::FolderFooter) {
                        break;
                    }

                    if (textItem == fromItem && textItem == toItem) {
                        xml += { textItem, _fromPosition, _toPosition - _fromPosition };
                    } else if (textItem == fromItem) {
                        xml += { textItem, _fromPosition, textItem->text().length() - _fromPosition };
                    } else if (textItem == toItem) {
                        xml += { textItem, 0, _toPosition};
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
            if (childItem == toItem
                || childItem->hasChild(toItem, recursively)) {
                return true;
            }
        }

        return false;
    };
    auto fromItemParent = fromItem->parent();
    auto fromItemRow = fromItemParent->rowOfChild(fromItem);
    //
    // Если построить нужно начиная с заголовка сцены или папки, то нужно захватить и саму сцену/папку
    //
    if (fromItem->type() == ScreenplayTextModelItemType::Text) {
        const auto textItem = static_cast<ScreenplayTextModelTextItem*>(fromItem);
        if (textItem->paragraphType() == ScreenplayParagraphType::SceneHeading
            || textItem->paragraphType() == ScreenplayParagraphType::FolderHeader) {
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
        fromItemRow = fromItemParent->rowOfChild(newFromItem) + 1; // +1, т.к. текущий мы уже обработали
    }

    xml += "</document>";
    return xml.data();
}

void ScreenplayTextModel::insertFromMime(const QModelIndex& _index, int _positionInBlock, const QString& _mimeData)
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
    QVector<ScreenplayTextModelItem*> lastItemsFromSourceScene;
    if (item->type() == ScreenplayTextModelItemType::Text) {
        auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
        //
        // Если в заголовок папки
        //
        if (textItem->paragraphType() == ScreenplayParagraphType::FolderHeader) {
            //
            // ... то вставим после него
            //
        }
        //
        // Если завершение папки
        //
        else if (textItem->paragraphType() == ScreenplayParagraphType::FolderFooter) {
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
            // Если вставка идёт в самое начало блока, то просто переносим блок после вставляемого фрагмента
            //
            if (_positionInBlock == 0) {
                lastItemsFromSourceScene.append(textItem);
            }
            //
            // В противном случае, дробим блок на две части
            //
            else if (textItem->text().length() > _positionInBlock) {
                const bool clearUuid = true;
                sourceBlockEndContent = mimeFromSelection(_index, _positionInBlock,
                                                          _index, textItem->text().length(), clearUuid);
                textItem->removeText(_positionInBlock);
                updateItem(textItem);
            }
        }
    }

    //
    // Считываем данные и последовательно вставляем в модель
    //
    QXmlStreamReader contentReader(_mimeData);
    contentReader.readNextStartElement(); // document
    contentReader.readNextStartElement();
    bool isFirstTextItemHandled = false;
    ScreenplayTextModelItem* lastItem = item;
    while (!contentReader.atEnd()) {
        const auto currentTag = contentReader.name();
        if (currentTag == xml::kDocumentTag) {
            break;
        }

        ScreenplayTextModelItem* newItem = nullptr;
        //
        // При входе в папку или сцену, если предыдущий текстовый элемент был в сцене,
        // то вставлять их будем не после текстового элемента, а после сцены
        //
        if ((currentTag == xml::kFolderTag
             || currentTag == xml::kSceneTag)
                && (lastItem->type() == ScreenplayTextModelItemType::Text
                    || lastItem->type() == ScreenplayTextModelItemType::Splitter)
                && lastItem->parent()->type() == ScreenplayTextModelItemType::Scene) {
            //
            // ... и при этом вырезаем из него все текстовые блоки, идущие до конца сцены/папки
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
        }


        if (currentTag == xml::kFolderTag) {
            newItem = new ScreenplayTextModelFolderItem(contentReader);
        } else if (currentTag == xml::kSceneTag) {
            newItem = new ScreenplayTextModelSceneItem(contentReader);
        } else if (currentTag == xml::kSplitterTag) {
            newItem = new ScreenplayTextModelSplitterItem(contentReader);
        } else {
            auto newTextItem = new ScreenplayTextModelTextItem(contentReader);
            //
            // Если вставляется текстовый элемент внутрь уже существующего элемента
            //
            if (!isFirstTextItemHandled
                && item->type() == ScreenplayTextModelItemType::Text) {
                //
                // ... то просто объединим их
                //
                isFirstTextItemHandled = true;
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
                if (!textItem->text().isEmpty()) {
                    textItem->mergeWith(newTextItem);
                } else {
                    textItem->copyFrom(newTextItem);
                }
                updateItem(textItem);
                delete newTextItem;
                //
                // ... и исключаем исходный блок из переноса, если он был туда помещён
                //
                lastItemsFromSourceScene.removeAll(textItem);
            }
            //
            // В противном случае вставляем текстовый элемент в модель
            //
            else {
                newItem = newTextItem;
            }
        }

        if (newItem != nullptr) {
            insertItem(newItem, lastItem);
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
        contentReader.readNextStartElement(); // text node
        auto item = new ScreenplayTextModelTextItem(contentReader);
        //
        // ... и последний вставленный элемент был текстовым
        //
        if (lastItem->type() == ScreenplayTextModelItemType::Text) {
            auto lastTextItem = static_cast<ScreenplayTextModelTextItem*>(lastItem);

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
            auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
            //
            // Удаляем пустые элементы модели
            //
            if (textItem->text().isEmpty()) {
                delete textItem;
                textItem = nullptr;
                continue;
            }

            if (lastItem->type() == ScreenplayTextModelItemType::Scene) {
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

void ScreenplayTextModel::setInformationModel(ScreenplayInformationModel* _model)
{
    if (d->informationModel == _model) {
        return;
    }

    if (d->informationModel) {
        disconnect(d->informationModel);
    }

    d->informationModel = _model;

    if (d->informationModel) {
        connect(d->informationModel, &ScreenplayInformationModel::scenesNumberingStartAtChanged,
                this, [this] { d->updateNumbering(); });
        connect(d->informationModel, &ScreenplayInformationModel::scenesNumbersPrefixChanged,
                this, [this] { d->updateNumbering(); });
    }
}

ScreenplayInformationModel* ScreenplayTextModel::informationModel() const
{
    return d->informationModel;
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

void ScreenplayTextModel::updateCharacterName(const QString& _oldName, const QString& _newName)
{
    const auto oldName = TextHelper::smartToUpper(_oldName);
    std::function<void(const ScreenplayTextModelItem*)> updateCharacterBlock;
    updateCharacterBlock = [this, oldName, _newName, &updateCharacterBlock] (const ScreenplayTextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
                case ScreenplayTextModelItemType::Folder:
                case ScreenplayTextModelItemType::Scene: {
                    updateCharacterBlock(childItem);
                    break;
                }

                case ScreenplayTextModelItemType::Text: {
                    auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                    if (textItem->paragraphType() == ScreenplayParagraphType::SceneCharacters
                        && SceneCharactersParser::characters(textItem->text()).contains(oldName)) {
                        auto text = textItem->text();
                        auto nameIndex = TextHelper::smartToUpper(text).indexOf(oldName);
                        while (nameIndex != -1) {
                            //
                            // Убедимся, что выделено именно имя, а не часть другого имени
                            //
                            const auto nameEndIndex = nameIndex + oldName.length();
                            const bool atLeftAllOk = nameIndex == 0
                                                     || text.at(nameIndex - 1) == ","
                                                 || (nameIndex > 2 && text.midRef(nameIndex - 2, 2) == ", ");
                            const bool atRightAllOk = nameEndIndex == text.length()
                                                      || text.at(nameEndIndex) == ","
                                                  || (text.length() > nameEndIndex + 1 && text.mid(nameEndIndex, 2) == " ,");
                            if (!atLeftAllOk || !atRightAllOk) {
                                nameIndex = TextHelper::smartToUpper(text).indexOf(oldName, nameIndex);
                                continue;
                            }

                            text.remove(nameIndex, oldName.length());
                            text.insert(nameIndex, _newName);
                            textItem->setText(text);
                            updateItem(textItem);
                            break;
                        }
                    } else if (textItem->paragraphType() == ScreenplayParagraphType::Character
                               && CharacterParser::name(textItem->text()) == oldName) {
                        auto text = textItem->text();
                        text.remove(0, oldName.length());
                        text.prepend(_newName);
                        textItem->setText(text);
                        updateItem(textItem);
                    }
                    break;
                }

                default: break;
            }
        }
    };

    emit rowsAboutToBeChanged();
    updateCharacterBlock(d->rootItem);
    emit rowsChanged();
}

void ScreenplayTextModel::updateLocationName(const QString& _oldName, const QString& _newName)
{
    const auto oldName = TextHelper::smartToUpper(_oldName);
    std::function<void(const ScreenplayTextModelItem*)> updateLocationBlock;
    updateLocationBlock = [this, oldName, _newName, &updateLocationBlock] (const ScreenplayTextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
                case ScreenplayTextModelItemType::Folder:
                case ScreenplayTextModelItemType::Scene: {
                    updateLocationBlock(childItem);
                    break;
                }

                case ScreenplayTextModelItemType::Text: {
                    auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                    if (textItem->paragraphType() == ScreenplayParagraphType::SceneHeading
                        && SceneHeadingParser::location(textItem->text()) == oldName) {
                        auto text = textItem->text();
                        const auto nameIndex = TextHelper::smartToUpper(text).indexOf(oldName);
                        text.remove(nameIndex, oldName.length());
                        text.insert(nameIndex, _newName);
                        textItem->setText(text);
                        updateItem(textItem);
                    }
                    break;
                }

                default: break;
            }
        }
    };

    emit rowsAboutToBeChanged();
    updateLocationBlock(d->rootItem);
    emit rowsChanged();
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
                case ScreenplayTextModelItemType::Folder:
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

    emit rowsAboutToBeChanged();
    updateChildDuration(d->rootItem);
    emit rowsChanged();
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

    emit rowsAboutToBeChanged();
    d->updateNumbering();
    emit rowsChanged();
}

void ScreenplayTextModel::clearDocument()
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

QByteArray ScreenplayTextModel::toXml() const
{
    return d->toXml(document());
}

void ScreenplayTextModel::applyPatch(const QByteArray& _patch)
{
    Q_ASSERT(document());

#ifdef XML_CHECKS
    const auto newContent = dmpController().applyPatch(toXml(), _patch);
#endif

    //
    // Определить область изменения в xml
    //
    auto changes = dmpController().changedXml(toXml(), _patch);
    changes.first.xml = xml::prepareXml(changes.first.xml);
    changes.second.xml = xml::prepareXml(changes.second.xml);

#ifdef XML_CHECKS
    qDebug(changes.first.xml.toUtf8());
    qDebug("************************");
    qDebug(changes.second.xml.toUtf8());
    qDebug("\n\n\n");
#endif

    //
    // Считываем элементы из обоих изменений для дальнейшего определения необходимых изменений
    //
    auto readItems = [] (const QString& _xml) {
        QXmlStreamReader _reader(_xml);
        xml::readNextElement(_reader); // document
        xml::readNextElement(_reader);

        QVector<ScreenplayTextModelItem*> items;
        while (!_reader.atEnd()) {
            const auto currentTag = _reader.name();
            ScreenplayTextModelItem* item = nullptr;
            if (currentTag == xml::kFolderTag) {
                item = new ScreenplayTextModelFolderItem(_reader);
            } else if (currentTag == xml::kSceneTag) {
                item = new ScreenplayTextModelSceneItem(_reader);
            } else if (currentTag == xml::kSplitterTag) {
                item = new ScreenplayTextModelSplitterItem(_reader);
            } else {
                item = new ScreenplayTextModelTextItem(_reader);
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
    std::function<QVector<ScreenplayTextModelItem*>(const QVector<ScreenplayTextModelItem*>&)> makeItemsPlain;
    makeItemsPlain = [&makeItemsPlain] (const QVector<ScreenplayTextModelItem*>& _items) {
        QVector<ScreenplayTextModelItem*> itemsPlain;
        for (auto item : _items) {
            itemsPlain.append(item);
            for (int row = 0; row < item->childCount(); ++row) {
                itemsPlain.append(makeItemsPlain({item->childAt(row)}));
            }
        }
        return itemsPlain;
    };
    auto oldItemsPlain = makeItemsPlain(oldItems);
    auto newItemsPlain = makeItemsPlain(newItems);

    //
    // Идём по структуре документа до момента достижения начала изменения
    //
    auto length = [this] {
        QByteArray xml = "<?xml version=\"1.0\"?>\n";
        xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type()) + "\" version=\"1.0\">\n";
        return xml.length();
    } ();
    std::function<ScreenplayTextModelItem*(ScreenplayTextModelItem*)> findStartItem;
    findStartItem = [changes, &length, &findStartItem] (ScreenplayTextModelItem* _item) -> ScreenplayTextModelItem*
    {
        if (changes.first.from == 0) {
            return _item->childAt(0);
        }

        ScreenplayTextModelItem* lastBrokenItem = nullptr;
        QScopedPointer<ScreenplayTextModelTextItem> lastBrokenItemCopy;
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            //
            // Определим дочерний элемент
            //
            auto child = _item->childAt(childIndex);
            if (child->type() == ScreenplayTextModelItemType::Text) {
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(child);
                if (textItem->isCorrection()) {
                    continue;
                }
                if (textItem->isBroken()) {
                    lastBrokenItem = textItem;
                    lastBrokenItemCopy.reset(new ScreenplayTextModelTextItem);
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
            if (changes.first.from >= length
                && changes.first.from < length + childLength) {
                //
                // Если есть дети, то уточняем поиск
                //
                int headerLength = 0;
                if (child->type() == ScreenplayTextModelItemType::Folder) {
                    auto folder = static_cast<ScreenplayTextModelFolderItem*>(child);
                    headerLength = QString(folder->xmlHeader()).length();
                } else if (child->type() == ScreenplayTextModelItemType::Scene) {
                    auto scene = static_cast<ScreenplayTextModelSceneItem*>(child);
                    headerLength = QString(scene->xmlHeader()).length();
                }

                if (child->hasChildren()
                    && changes.first.from >= length + headerLength) {
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
    while (oldItemsPlain.size() > 1
           && oldItemsPlain.constFirst()->type() != modelItem->type()) {
        oldItemsPlain.removeFirst();
    }
    while (newItemsPlain.size() > 1
           && newItemsPlain.constFirst()->type() != modelItem->type()
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
    std::function<ScreenplayTextModelItem*(ScreenplayTextModelItem*, bool)> findNextItemWithChildren;
    findNextItemWithChildren = [&findNextItemWithChildren] (ScreenplayTextModelItem* _item, bool _searchInChildren) -> ScreenplayTextModelItem*
    {
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
    auto findNextItem = [&findNextItemWithChildren] (ScreenplayTextModelItem* _item) {
        auto nextItem = findNextItemWithChildren(_item, true);
        //
        // Пропускаем текстовые декорации, т.к. они не сохраняются в модель
        //
        while (nextItem != nullptr
               && nextItem->type() == ScreenplayTextModelItemType::Text
               && static_cast<ScreenplayTextModelTextItem*>(nextItem)->isCorrection()) {
            nextItem = findNextItemWithChildren(nextItem, true);
        }
        return nextItem;
    };
    //
    // И применяем их
    //
    emit rowsAboutToBeChanged();
    ScreenplayTextModelItem* previousModelItem = nullptr;
    //
    // В некоторых ситуациях мы не знаем сразу, куда будут извлечены элементы из удаляемого элемента,
    // или когда элемент вставляется посреди и отрезает часть вложенных элементов, поэтому упаковываем
    // их в список для размещения в правильном месте в следующем проходе
    //
    QVector<ScreenplayTextModelItem*> movedSiblingItems;
    auto updateItemPlacement = [this, &modelItem, &previousModelItem, newItemsPlain, &movedSiblingItems]
                               (ScreenplayTextModelItem* _newItem, ScreenplayTextModelItem* _item)
    {
        //
        // Определим предыдущий элемент из списка новых, в дальнейшем будем опираться
        // на его расположение относительно текущего нового
        //
        const auto newItemIndex = newItemsPlain.indexOf(_newItem);
        ScreenplayTextModelItem* previousNewItem = newItemIndex > 0
                                                   ? newItemsPlain.at(newItemIndex - 1)
                                                   : nullptr;
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
                // Если нужно вставить перед первым элементом, то это случай вставки в начало документа
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
                    auto insertAfterItem = previousModelItem->parent(); // этот на один уровень опаздывает за предыдущим
                    while (previousNewItemParent != _newItem->parent()) {
                        previousNewItemParent = previousNewItemParent->parent();
                        insertAfterItem = insertAfterItem->parent();
                    }

                    insertItem(_item, insertAfterItem);

                    //
                    // И вытаскиваем все последующие элементы на уровень нового, если есть откуда вытянуть конечно же
                    //
                    if (modelItem != nullptr) {
                        auto modelItemParent = modelItem->parent();
                        const int modelItemIndex = modelItemParent->rowOfChild(modelItem);
                        while (modelItemParent->childCount() > modelItemIndex) {
                            auto childItem = modelItemParent->childAt(modelItemParent->childCount() - 1);
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
                auto insertAfterItem = previousModelItem->parent(); // этот на один уровень опаздывает за предыдущим
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
                // И вытаскиваем все последующие элементы в модели на уровень вставки
                //
                while (itemParent->childCount() > itemIndex) {
                    auto childItem = itemParent->childAt(itemParent->childCount() - 1);
                    takeItem(childItem, itemParent);
                    insertItem(childItem, _item);
                    movedSiblingItems.prepend(childItem);
                }
            }
        }

        //
        // Если у нас в буфере есть перенесённые элементы и текущий является их предводителем
        //
        if (!movedSiblingItems.isEmpty()
            && movedSiblingItems.constFirst() == _item) {
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
        if (modelItem != nullptr
            && modelItem->type() == ScreenplayTextModelItemType::Text) {
            auto textItem = static_cast<ScreenplayTextModelTextItem*>(modelItem);
            if (textItem->isBroken()) {
                auto nextItem = findNextItemWithChildren(textItem, false);;
                while (nextItem != nullptr
                       && nextItem->type() == ScreenplayTextModelItemType::Text) {
                    auto nextTextItem = static_cast<ScreenplayTextModelTextItem*>(nextItem);
                    if (nextTextItem->isCorrection()) {
                        auto itemToRemove = nextItem;
                        nextItem = findNextItemWithChildren(nextItem, false);
                        removeItem(itemToRemove);
                        continue;
                    }

                    textItem->setText(textItem->text() + " ");
                    textItem->mergeWith(nextTextItem);
                    textItem->setBroken(false);
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
                removeItem(modelItem);
                modelItem = nextItem;
                break;
            }

            case edit_distance::OperationType::Insert: {
                //
                // Создаём новый элемент
                //
                ScreenplayTextModelItem* itemToInsert = nullptr;
                switch (newItem->type()) {
                    case ScreenplayTextModelItemType::Folder: {
                        itemToInsert = new ScreenplayTextModelFolderItem;
                        break;
                    }

                    case ScreenplayTextModelItemType::Scene: {
                        itemToInsert = new ScreenplayTextModelSceneItem;
                        break;
                    }

                    case ScreenplayTextModelItemType::Text: {
                        itemToInsert = new ScreenplayTextModelTextItem;
                        break;
                    }

                    case ScreenplayTextModelItemType::Splitter: {
                        itemToInsert = new ScreenplayTextModelSplitterItem(
                                           static_cast<ScreenplayTextModelSplitterItem*>(newItem)
                                           ->splitterType());
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
