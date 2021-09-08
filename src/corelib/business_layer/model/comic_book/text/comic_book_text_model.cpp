#include "comic_book_text_model.h"

#include "comic_book_text_block_parser.h"
#include "comic_book_text_model_folder_item.h"
#include "comic_book_text_model_page_item.h"
#include "comic_book_text_model_panel_item.h"
#include "comic_book_text_model_splitter_item.h"
#include "comic_book_text_model_text_item.h"
#include "comic_book_text_model_xml.h"
#include "comic_book_text_model_xml_writer.h"

#include <business_layer/model/comic_book/comic_book_dictionaries_model.h>
#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/templates/comic_book_template.h>
#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/text_helper.h>
#include <utils/shugar.h>
#include <utils/tools/edit_distance.h>
#include <utils/tools/model_index_path.h>

#include <QMimeData>
#include <QXmlStreamReader>

#ifdef QT_DEBUG
#define XML_CHECKS
#endif

namespace BusinessLayer {

namespace {
const char* kMimeType = "application/x-starc/comicbook/text/item";
}

class ComicBookTextModel::Implementation
{
public:
    Implementation();

    /**
     * @brief Построить модель структуры из xml хранящегося в документе
     */
    void buildModel(Domain::DocumentObject* _comicBook);

    /**
     * @brief Сформировать xml из данных модели
     */
    QByteArray toXml(Domain::DocumentObject* _comicBook) const;

    /**
     * @brief Обновить номера страниц, панелей и реплик
     */
    void updateNumbering();


    /**
     * @brief Корневой элемент дерева
     */
    ComicBookTextModelFolderItem* rootItem = nullptr;

    /**
     * @brief Модель информации о проекте
     */
    ComicBookInformationModel* informationModel = nullptr;

    /**
     * @brief Модель справочников
     */
    ComicBookDictionariesModel* dictionariesModel = nullptr;

    /**
     * @brief Модель персонажей
     */
    CharactersModel* charactersModel = nullptr;

    /**
     * @brief Последние скопированные данные модели
     */
    struct {
        QModelIndex from;
        QModelIndex to;
        QMimeData* data = nullptr;
    } lastMime;
};

ComicBookTextModel::Implementation::Implementation()
    : rootItem(new ComicBookTextModelFolderItem)
{
}

void ComicBookTextModel::Implementation::buildModel(Domain::DocumentObject* _comicBook)
{
    if (_comicBook == nullptr) {
        return;
    }

    QXmlStreamReader contentReader(_comicBook->content());
    contentReader.readNextStartElement(); // document
    contentReader.readNextStartElement();
    while (!contentReader.atEnd()) {
        const auto currentTag = contentReader.name();
        if (currentTag == xml::kDocumentTag) {
            break;
        }

        if (currentTag == xml::kFolderTag) {
            rootItem->appendItem(new ComicBookTextModelFolderItem(contentReader));
        } else if (currentTag == xml::kPageTag) {
            rootItem->appendItem(new ComicBookTextModelPageItem(contentReader));
        } else if (currentTag == xml::kPanelTag) {
            rootItem->appendItem(new ComicBookTextModelPanelItem(contentReader));
        } else if (currentTag == xml::kSplitterTag) {
            rootItem->appendItem(new ComicBookTextModelSplitterItem(contentReader));
        } else {
            rootItem->appendItem(new ComicBookTextModelTextItem(contentReader));
        }
    }
}

QByteArray ComicBookTextModel::Implementation::toXml(Domain::DocumentObject* _comicBook) const
{
    if (_comicBook == nullptr) {
        return {};
    }

    const bool addXMlHeader = true;
    xml::ComicBookTextModelXmlWriter xml(addXMlHeader);
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(_comicBook->type())
        + "\" version=\"1.0\">\n";
    for (int childIndex = 0; childIndex < rootItem->childCount(); ++childIndex) {
        xml += rootItem->childAt(childIndex);
    }
    xml += "</document>";
    return xml.data();
}

void ComicBookTextModel::Implementation::updateNumbering()
{
    int pageNumber = 1;
    int panelNumber = 1;
    int dialogueNumber = 1;
    std::function<void(const ComicBookTextModelItem*)> updateChildNumbering;
    updateChildNumbering = [this, &pageNumber, &panelNumber, &dialogueNumber,
                            &updateChildNumbering](const ComicBookTextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case ComicBookTextModelItemType::Folder: {
                updateChildNumbering(childItem);
                break;
            }

            case ComicBookTextModelItemType::Page: {
                panelNumber = 1;
                dialogueNumber = 1;
                updateChildNumbering(childItem);

                auto pageItem = static_cast<ComicBookTextModelPageItem*>(childItem);
                pageItem->updateNumber(pageNumber, dictionariesModel->singlePageIntros(),
                                       dictionariesModel->multiplePageIntros());
                break;
            }

            case ComicBookTextModelItemType::Panel: {
                updateChildNumbering(childItem);
                auto panelItem = static_cast<ComicBookTextModelPanelItem*>(childItem);
                if (panelItem->setNumber(panelNumber)) {
                    panelNumber++;
                }
                break;
            }

            case ComicBookTextModelItemType::Text: {
                auto textItem = static_cast<ComicBookTextModelTextItem*>(childItem);
                if (textItem->paragraphType() == ComicBookParagraphType::Character
                    && !textItem->isCorrection()) {
                    textItem->setNumber(dialogueNumber++);
                }
                break;
            }

            default:
                break;
            }
        }
    };
    updateChildNumbering(rootItem);
}


// ****


ComicBookTextModel::ComicBookTextModel(QObject* _parent)
    : AbstractModel(
        {
            xml::kDocumentTag,
            xml::kFolderTag,
            xml::kPageTag,
            xml::kPanelTag,
            toString(ComicBookParagraphType::UnformattedText),
            toString(ComicBookParagraphType::Page),
            toString(ComicBookParagraphType::Panel),
            toString(ComicBookParagraphType::Description),
            toString(ComicBookParagraphType::Character),
            toString(ComicBookParagraphType::Dialogue),
            toString(ComicBookParagraphType::InlineNote),
            toString(ComicBookParagraphType::FolderHeader),
            toString(ComicBookParagraphType::FolderFooter),
            toString(ComicBookParagraphType::PageSplitter),
        },
        _parent)
    , d(new Implementation)
{
}

ComicBookTextModel::~ComicBookTextModel() = default;

void ComicBookTextModel::appendItem(ComicBookTextModelItem* _item,
                                    ComicBookTextModelItem* _parentItem)
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

void ComicBookTextModel::prependItem(ComicBookTextModelItem* _item,
                                     ComicBookTextModelItem* _parentItem)
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

void ComicBookTextModel::insertItem(ComicBookTextModelItem* _item,
                                    ComicBookTextModelItem* _afterSiblingItem)
{
    if (_item == nullptr || _afterSiblingItem == nullptr
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

void ComicBookTextModel::takeItem(ComicBookTextModelItem* _item,
                                  ComicBookTextModelItem* _parentItem)
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

void ComicBookTextModel::removeItem(ComicBookTextModelItem* _item)
{
    if (_item == nullptr || _item->parent() == nullptr) {
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

void ComicBookTextModel::updateItem(ComicBookTextModelItem* _item)
{
    if (_item == nullptr || !_item->isChanged()) {
        return;
    }

    const QModelIndex indexForUpdate = indexForItem(_item);
    emit dataChanged(indexForUpdate, indexForUpdate);
    _item->setChanged(false);

    if (_item->parent() != nullptr) {
        updateItem(_item->parent());
    } else {
        d->updateNumbering();
    }
}

QModelIndex ComicBookTextModel::index(int _row, int _column, const QModelIndex& _parent) const
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

QModelIndex ComicBookTextModel::parent(const QModelIndex& _child) const
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

int ComicBookTextModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int ComicBookTextModel::rowCount(const QModelIndex& _parent) const
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

Qt::ItemFlags ComicBookTextModel::flags(const QModelIndex& _index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    const auto item = itemForIndex(_index);
    switch (item->type()) {
    case ComicBookTextModelItemType::Folder:
    case ComicBookTextModelItemType::Page: {
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        break;
    }

    case ComicBookTextModelItemType::Panel: {
        flags |= Qt::ItemIsDragEnabled;
        break;
    }

    default:
        break;
    }

    return flags;
}

QVariant ComicBookTextModel::data(const QModelIndex& _index, int _role) const
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

bool ComicBookTextModel::canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row,
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
        = _parent.isValid() ? itemForIndex(_parent)->type() : ComicBookTextModelItemType::Folder;

    //
    // Получим первый из перемещаемых элементов
    //
    QXmlStreamReader contentReader(_data->data(mimeTypes().constFirst()));
    contentReader.readNextStartElement(); // document
    contentReader.readNextStartElement();
    ComicBookTextModelItemType firstItemType = ComicBookTextModelItemType::Text;
    do {
        const auto currentTag = contentReader.name();
        if (currentTag == xml::kDocumentTag) {
            break;
        }

        if (currentTag == xml::kFolderTag) {
            firstItemType = ComicBookTextModelItemType::Folder;
        } else if (currentTag == xml::kPageTag) {
            firstItemType = ComicBookTextModelItemType::Page;
        } else if (currentTag == xml::kPanelTag) {
            firstItemType = ComicBookTextModelItemType::Panel;
        } else if (currentTag == xml::kSplitterTag) {
            firstItemType = ComicBookTextModelItemType::Splitter;
        }
    }
    once;

    //
    // Собственно определяем правила перемещения
    //
    switch (firstItemType) {
    case ComicBookTextModelItemType::Page: {
        return parentItemType == ComicBookTextModelItemType::Folder;
    }

    case ComicBookTextModelItemType::Panel: {
        return parentItemType == ComicBookTextModelItemType::Page;
    }

    default: {
        return false;
    }
    }
}

bool ComicBookTextModel::dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row,
                                      int _column, const QModelIndex& _parent)
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
            if (_parent.isValid() && rowCount(_parent) == _row
                && itemForIndex(_parent)->type() == ComicBookTextModelItemType::Folder) {
                //
                // ... для папок, при вставке в самый конец также нужно учитывать
                //     текстовый блок закрывающий папку
                //
                ++delta;
            }
            insertAnchorIndex = index(_row - delta, 0, _parent);
        }
        if (d->lastMime.from == insertAnchorIndex || d->lastMime.to == insertAnchorIndex) {
            return false;
        }
        ComicBookTextModelItem* insertAnchorItem = itemForIndex(insertAnchorIndex);

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
        ComicBookTextModelItem* lastItem = insertAnchorItem;
        while (!contentReader.atEnd()) {
            const auto currentTag = contentReader.name();
            if (currentTag == xml::kDocumentTag) {
                break;
            }

            ComicBookTextModelItem* newItem = nullptr;
            if (currentTag == xml::kFolderTag) {
                newItem = new ComicBookTextModelFolderItem(contentReader);
            } else if (currentTag == xml::kPageTag) {
                newItem = new ComicBookTextModelPageItem(contentReader);
            } else if (currentTag == xml::kPanelTag) {
                newItem = new ComicBookTextModelPanelItem(contentReader);
            } else if (currentTag == xml::kSplitterTag) {
                newItem = new ComicBookTextModelSplitterItem(contentReader);
            } else {
                newItem = new ComicBookTextModelTextItem(contentReader);
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
                    if (lastItem->type() == ComicBookTextModelItemType::Folder
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
                // Вставить в конец
                //
                else if (_row == -1) {
                    //
                    // При вставке в папку, нужно не забыть про завершающий папку блок
                    //
                    if (lastItem->type() == ComicBookTextModelItemType::Folder
                        && _parent.isValid()) {
                        insertItem(newItem, lastItem->childAt(lastItem->childCount() - 2));
                    } else if (lastItem->type() == ComicBookTextModelItemType::Page
                               && _parent.isValid()) {
                        insertItem(newItem, lastItem->childAt(lastItem->childCount() - 1));
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

QMimeData* ComicBookTextModel::mimeData(const QModelIndexList& _indexes) const
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

QStringList ComicBookTextModel::mimeTypes() const
{
    return { kMimeType };
}

Qt::DropActions ComicBookTextModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions ComicBookTextModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QString ComicBookTextModel::mimeFromSelection(const QModelIndex& _from, int _fromPosition,
                                              const QModelIndex& _to, int _toPosition,
                                              bool _clearUuid) const
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
    xml::ComicBookTextModelXmlWriter xml(addXMlHeader);
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type())
        + "\" version=\"1.0\">\n";

    auto buildXmlFor = [&xml, fromItem, _fromPosition, toItem, _toPosition,
                        _clearUuid](ComicBookTextModelItem* _fromItemParent, int _fromItemRow) {
        for (int childIndex = _fromItemRow; childIndex < _fromItemParent->childCount();
             ++childIndex) {
            const auto childItem = _fromItemParent->childAt(childIndex);

            switch (childItem->type()) {
            case ComicBookTextModelItemType::Folder: {
                const auto folderItem = static_cast<ComicBookTextModelFolderItem*>(childItem);
                xml += folderItem->toXml(fromItem, _fromPosition, toItem, _toPosition, _clearUuid);
                break;
            }

            case ComicBookTextModelItemType::Page: {
                const auto pageItem = static_cast<ComicBookTextModelPageItem*>(childItem);
                xml += pageItem->toXml(fromItem, _fromPosition, toItem, _toPosition, _clearUuid);
                break;
            }

            case ComicBookTextModelItemType::Panel: {
                const auto panelItem = static_cast<ComicBookTextModelPanelItem*>(childItem);
                xml += panelItem->toXml(fromItem, _fromPosition, toItem, _toPosition, _clearUuid);
                break;
            }

            case ComicBookTextModelItemType::Text: {
                const auto textItem = static_cast<ComicBookTextModelTextItem*>(childItem);

                //
                // Не сохраняем закрывающие блоки неоткрытых папок, всё это делается внутри самих
                // папок
                //
                if (textItem->paragraphType() == ComicBookParagraphType::FolderFooter) {
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
    // Если построить нужно начиная с папки, страницы или панели, то нужно захватить и саму
    // папку/страницу/панель
    //
    if (fromItem->type() == ComicBookTextModelItemType::Text) {
        const auto textItem = static_cast<ComicBookTextModelTextItem*>(fromItem);
        if (textItem->paragraphType() == ComicBookParagraphType::FolderHeader
            || textItem->paragraphType() == ComicBookParagraphType::Page
            || textItem->paragraphType() == ComicBookParagraphType::Panel) {
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

void ComicBookTextModel::insertFromMime(const QModelIndex& _index, int _positionInBlock,
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
    QVector<ComicBookTextModelItem*> lastItemsFromSourceScene;
    if (item->type() == ComicBookTextModelItemType::Text) {
        auto textItem = static_cast<ComicBookTextModelTextItem*>(item);
        //
        // Если в заголовок папки
        //
        if (textItem->paragraphType() == ComicBookParagraphType::FolderHeader) {
            //
            // ... то вставим после него
            //
        }
        //
        // Если завершение папки
        //
        else if (textItem->paragraphType() == ComicBookParagraphType::FolderFooter) {
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
            if (_positionInBlock == 0) {
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
        qDebug("here");
    }

    //
    // Считываем данные и последовательно вставляем в модель
    //
    QXmlStreamReader contentReader(_mimeData);
    contentReader.readNextStartElement(); // document
    contentReader.readNextStartElement();
    bool isFirstTextItemHandled = false;
    ComicBookTextModelItem* lastItem = item;
    while (!contentReader.atEnd()) {
        const auto currentTag = contentReader.name();
        if (currentTag == xml::kDocumentTag) {
            break;
        }

        ComicBookTextModelItem* newItem = nullptr;
        //
        // При входе в папку, сраницу или панель, если предыдущий текстовый элемент был на странице
        // или панели, то вставлять их будем не после текстового элемента, а после страницы/панели
        //
        if ((currentTag == xml::kFolderTag || currentTag == xml::kPageTag
             || currentTag == xml::kPanelTag)
            && (lastItem->type() == ComicBookTextModelItemType::Text
                || lastItem->type() == ComicBookTextModelItemType::Splitter)
            && (lastItem->parent()->type() == ComicBookTextModelItemType::Page
                || lastItem->parent()->type() == ComicBookTextModelItemType::Panel)) {
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
            newItem = new ComicBookTextModelFolderItem(contentReader);
        } else if (currentTag == xml::kPageTag) {
            newItem = new ComicBookTextModelPageItem(contentReader);
        } else if (currentTag == xml::kPanelTag) {
            newItem = new ComicBookTextModelPanelItem(contentReader);
        } else if (currentTag == xml::kSplitterTag) {
            newItem = new ComicBookTextModelSplitterItem(contentReader);
        } else {
            auto newTextItem = new ComicBookTextModelTextItem(contentReader);
            //
            // Если вставляется текстовый элемент внутрь уже существующего элемента
            //
            if (!isFirstTextItemHandled && item->type() == ComicBookTextModelItemType::Text) {
                //
                // ... то просто объединим их
                //
                isFirstTextItemHandled = true;
                auto textItem = static_cast<ComicBookTextModelTextItem*>(item);
                textItem->mergeWith(newTextItem);
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
        auto item = new ComicBookTextModelTextItem(contentReader);
        //
        // ... и последний вставленный элемент был текстовым
        //
        if (lastItem->type() == ComicBookTextModelItemType::Text) {
            auto lastTextItem = static_cast<ComicBookTextModelTextItem*>(lastItem);

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
            auto textItem = static_cast<ComicBookTextModelTextItem*>(item);
            //
            // Удаляем пустые элементы модели
            //
            if (textItem->text().isEmpty()) {
                delete textItem;
                textItem = nullptr;
                continue;
            }

            if (lastItem->type() == ComicBookTextModelItemType::Page
                || lastItem->type() == ComicBookTextModelItemType::Panel) {
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

ComicBookTextModelItem* ComicBookTextModel::itemForIndex(const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return d->rootItem;
    }

    auto item = static_cast<ComicBookTextModelItem*>(_index.internalPointer());
    if (item == nullptr) {
        return d->rootItem;
    }

    return item;
}

QModelIndex ComicBookTextModel::indexForItem(ComicBookTextModelItem* _item) const
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

void ComicBookTextModel::setInformationModel(ComicBookInformationModel* _model)
{
    if (d->informationModel == _model) {
        return;
    }

    if (d->informationModel) {
        disconnect(d->informationModel);
    }

    d->informationModel = _model;
}

ComicBookInformationModel* ComicBookTextModel::informationModel() const
{
    return d->informationModel;
}

void ComicBookTextModel::setDictionariesModel(ComicBookDictionariesModel* _model)
{
    d->dictionariesModel = _model;
}

ComicBookDictionariesModel* ComicBookTextModel::dictionariesModel() const
{
    return d->dictionariesModel;
}

void ComicBookTextModel::setCharactersModel(CharactersModel* _model)
{
    d->charactersModel = _model;
}

CharactersModel* ComicBookTextModel::charactersModel() const
{
    return d->charactersModel;
}

void ComicBookTextModel::updateCharacterName(const QString& _oldName, const QString& _newName)
{
    const auto oldName = TextHelper::smartToUpper(_oldName);
    std::function<void(const ComicBookTextModelItem*)> updateCharacterBlock;
    updateCharacterBlock
        = [this, oldName, _newName, &updateCharacterBlock](const ComicBookTextModelItem* _item) {
              for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
                  auto childItem = _item->childAt(childIndex);
                  switch (childItem->type()) {
                  case ComicBookTextModelItemType::Folder:
                  case ComicBookTextModelItemType::Page:
                  case ComicBookTextModelItemType::Panel: {
                      updateCharacterBlock(childItem);
                      break;
                  }

                  case ComicBookTextModelItemType::Text: {
                      auto textItem = static_cast<ComicBookTextModelTextItem*>(childItem);
                      if (textItem->paragraphType() == ComicBookParagraphType::Character
                          && ComicBookCharacterParser::name(textItem->text()) == oldName) {
                          auto text = textItem->text();
                          text.remove(0, oldName.length());
                          text.prepend(_newName);
                          textItem->setText(text);
                          updateItem(textItem);
                      }
                      break;
                  }

                  default:
                      break;
                  }
              }
          };

    emit rowsAboutToBeChanged();
    updateCharacterBlock(d->rootItem);
    emit rowsChanged();
}

void ComicBookTextModel::initDocument()
{
    //
    // Если документ пустой, создаём первоначальную структуру
    //
    if (document()->content().isEmpty()) {
        auto pageText = new ComicBookTextModelTextItem;
        pageText->setParagraphType(ComicBookParagraphType::Page);
        auto page = new ComicBookTextModelPageItem;
        page->appendItem(pageText);
        appendItem(page);
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

void ComicBookTextModel::clearDocument()
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

QByteArray ComicBookTextModel::toXml() const
{
    return d->toXml(document());
}

void ComicBookTextModel::applyPatch(const QByteArray& _patch)
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
    qDebug(changes.first.xml);
    qDebug("************************");
    qDebug(changes.second.xml);
    qDebug("\n\n\n");
#endif

    //
    // Считываем элементы из обоих изменений для дальнейшего определения необходимых изменений
    //
    auto readItems = [](const QString& _xml) {
        QXmlStreamReader _reader(_xml);
        xml::readNextElement(_reader); // document
        xml::readNextElement(_reader);

        QVector<ComicBookTextModelItem*> items;
        while (!_reader.atEnd()) {
            const auto currentTag = _reader.name();
            ComicBookTextModelItem* item = nullptr;
            if (currentTag == xml::kFolderTag) {
                item = new ComicBookTextModelFolderItem(_reader);
            } else if (currentTag == xml::kPageTag) {
                item = new ComicBookTextModelPageItem(_reader);
            } else if (currentTag == xml::kPanelTag) {
                item = new ComicBookTextModelPanelItem(_reader);
            } else if (currentTag == xml::kSplitterTag) {
                item = new ComicBookTextModelSplitterItem(_reader);
            } else {
                item = new ComicBookTextModelTextItem(_reader);
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
    std::function<QVector<ComicBookTextModelItem*>(const QVector<ComicBookTextModelItem*>&)>
        makeItemsPlain;
    makeItemsPlain = [&makeItemsPlain](const QVector<ComicBookTextModelItem*>& _items) {
        QVector<ComicBookTextModelItem*> itemsPlain;
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
    // Идём по структуре документа до момента достижения начала изменения
    //
    auto length = [this] {
        QByteArray xml = "<?xml version=\"1.0\"?>\n";
        xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type())
            + "\" version=\"1.0\">\n";
        return xml.length();
    }();
    std::function<ComicBookTextModelItem*(ComicBookTextModelItem*)> findStartItem;
    findStartItem = [changes, &length,
                     &findStartItem](ComicBookTextModelItem* _item) -> ComicBookTextModelItem* {
        if (changes.first.from == 0) {
            return _item->childAt(0);
        }

        ComicBookTextModelItem* lastBrokenItem = nullptr;
        QScopedPointer<ComicBookTextModelTextItem> lastBrokenItemCopy;
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            //
            // Определим дочерний элемент
            //
            auto child = _item->childAt(childIndex);
            if (child->type() == ComicBookTextModelItemType::Text) {
                auto textItem = static_cast<ComicBookTextModelTextItem*>(child);
                if (textItem->isCorrection()) {
                    continue;
                }
                if (textItem->isBreakCorrectionStart()) {
                    lastBrokenItem = textItem;
                    lastBrokenItemCopy.reset(new ComicBookTextModelTextItem);
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
                if (child->type() == ComicBookTextModelItemType::Folder) {
                    auto folder = static_cast<ComicBookTextModelFolderItem*>(child);
                    headerLength = QString(folder->xmlHeader()).length();
                } else if (child->type() == ComicBookTextModelItemType::Page) {
                    auto page = static_cast<ComicBookTextModelPageItem*>(child);
                    headerLength = QString(page->xmlHeader()).length();
                } else if (child->type() == ComicBookTextModelItemType::Panel) {
                    auto panel = static_cast<ComicBookTextModelPanelItem*>(child);
                    headerLength = QString(panel->xmlHeader()).length();
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
    std::function<ComicBookTextModelItem*(ComicBookTextModelItem*, bool)> findNextItemWithChildren;
    findNextItemWithChildren
        = [&findNextItemWithChildren](ComicBookTextModelItem* _item,
                                      bool _searchInChildren) -> ComicBookTextModelItem* {
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
    auto findNextItem = [&findNextItemWithChildren](ComicBookTextModelItem* _item) {
        auto nextItem = findNextItemWithChildren(_item, true);
        //
        // Пропускаем текстовые декорации, т.к. они не сохраняются в модель
        //
        while (nextItem != nullptr && nextItem->type() == ComicBookTextModelItemType::Text
               && static_cast<ComicBookTextModelTextItem*>(nextItem)->isCorrection()) {
            nextItem = findNextItemWithChildren(nextItem, true);
        }
        return nextItem;
    };
    //
    // И применяем их
    //
    emit rowsAboutToBeChanged();
    ComicBookTextModelItem* previousModelItem = nullptr;
    //
    // В некоторых ситуациях мы не знаем сразу, куда будут извлечены элементы из удаляемого
    // элемента, или когда элемент вставляется посреди и отрезает часть вложенных элементов, поэтому
    // упаковываем их в список для размещения в правильном месте в следующем проходе
    //
    QVector<ComicBookTextModelItem*> movedSiblingItems;
    auto updateItemPlacement = [this, &modelItem, &previousModelItem, newItemsPlain,
                                &movedSiblingItems](ComicBookTextModelItem* _newItem,
                                                    ComicBookTextModelItem* _item) {
        //
        // Определим предыдущий элемент из списка новых, в дальнейшем будем опираться
        // на его расположение относительно текущего нового
        //
        const auto newItemIndex = newItemsPlain.indexOf(_newItem);
        ComicBookTextModelItem* previousNewItem
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
                    if (modelItem != nullptr) {
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
        if (modelItem != nullptr && modelItem->type() == ComicBookTextModelItemType::Text) {
            auto textItem = static_cast<ComicBookTextModelTextItem*>(modelItem);
            if (textItem->isBreakCorrectionStart()) {
                auto nextItem = findNextItemWithChildren(textItem, false);
                ;
                while (nextItem != nullptr
                       && nextItem->type() == ComicBookTextModelItemType::Text) {
                    auto nextTextItem = static_cast<ComicBookTextModelTextItem*>(nextItem);
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
            removeItem(modelItem);
            modelItem = nextItem;
            break;
        }

        case edit_distance::OperationType::Insert: {
            //
            // Создаём новый элемент
            //
            ComicBookTextModelItem* itemToInsert = nullptr;
            switch (newItem->type()) {
            case ComicBookTextModelItemType::Folder: {
                itemToInsert = new ComicBookTextModelFolderItem;
                break;
            }

            case ComicBookTextModelItemType::Page: {
                itemToInsert = new ComicBookTextModelPageItem;
                break;
            }

            case ComicBookTextModelItemType::Panel: {
                itemToInsert = new ComicBookTextModelPanelItem;
                break;
            }

            case ComicBookTextModelItemType::Text: {
                itemToInsert = new ComicBookTextModelTextItem;
                break;
            }

            case ComicBookTextModelItemType::Splitter: {
                itemToInsert = new ComicBookTextModelSplitterItem(
                    static_cast<ComicBookTextModelSplitterItem*>(newItem)->splitterType());
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
