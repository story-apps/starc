#include "simple_text_model.h"

#include "simple_text_model_chapter_item.h"
#include "simple_text_model_text_item.h"
#include "simple_text_model_xml.h"

#include <business_layer/templates/simple_text_template.h>
#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
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

const char* kMimeType = "application/x-starc/text/item";

/**
 * @brief Найти первый текстовый элемент вложенный в заданный
 */
SimpleTextModelItem* firstTextItem(SimpleTextModelItem* _item)
{
    Q_ASSERT(_item->type() != SimpleTextModelItemType::Text);
    for (auto childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
        auto childItem = _item->childAt(childIndex);
        if (childItem->type() == SimpleTextModelItemType::Chapter) {
            return firstTextItem(childItem);
        } else {
            return childItem;
        }
    }
    return nullptr;
};

} // namespace

class SimpleTextModel::Implementation
{
public:
    Implementation();

    /**
     * @brief Построить модель структуры из xml хранящегося в документе
     */
    void buildModel(Domain::DocumentObject* _text);

    /**
     * @brief Сформировать xml из данных модели
     */
    QByteArray toXml(Domain::DocumentObject* _text) const;

    /**
     * @brief Обновить номера глав
     */
    void updateNumbering();


    /**
     * @brief Название документа
     */
    QString name;

    /**
     * @brief Корневой элемент дерева
     */
    SimpleTextModelChapterItem* rootItem = nullptr;

    /**
     * @brief Последние скопированные данные модели
     */
    struct {
        QModelIndex from;
        QModelIndex to;
        QMimeData* data = nullptr;
    } lastMime;
};

SimpleTextModel::Implementation::Implementation()
    : rootItem(new SimpleTextModelChapterItem)
{
}

void SimpleTextModel::Implementation::buildModel(Domain::DocumentObject* _text)
{
    if (_text == nullptr) {
        return;
    }

    QXmlStreamReader contentReader(_text->content());
    contentReader.readNextStartElement(); // document
    contentReader.readNextStartElement();
    while (!contentReader.atEnd()) {
        const auto currentTag = contentReader.name();
        if (currentTag == xml::kDocumentTag) {
            break;
        }

        if (currentTag == xml::kChapterTag) {
            rootItem->appendItem(new SimpleTextModelChapterItem(contentReader));
        } else {
            rootItem->appendItem(new SimpleTextModelTextItem(contentReader));
        }
    }

    const auto firstItem = firstTextItem(rootItem);
    if (firstItem != nullptr) {
        const auto textItem = static_cast<const SimpleTextModelTextItem*>(firstItem);
        name = textItem->text();
    }
}

QByteArray SimpleTextModel::Implementation::toXml(Domain::DocumentObject* _text) const
{
    if (_text == nullptr) {
        return {};
    }
    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(_text->type()) + "\" version=\"1.0\">\n";
    for (int childIndex = 0; childIndex < rootItem->childCount(); ++childIndex) {
        xml += rootItem->childAt(childIndex)->toXml();
    }
    xml += "</document>";
    return xml;
}

void SimpleTextModel::Implementation::updateNumbering()
{
    int sceneNumber = 1;
    std::function<void(const SimpleTextModelItem*)> updateChildNumbering;
    updateChildNumbering = [&sceneNumber, &updateChildNumbering](const SimpleTextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case SimpleTextModelItemType::Chapter: {
                updateChildNumbering(childItem);
                auto chapterItem = static_cast<SimpleTextModelChapterItem*>(childItem);
                chapterItem->setNumber(sceneNumber++);
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


SimpleTextModel::SimpleTextModel(QObject* _parent)
    : AbstractModel({ xml::kDocumentTag, xml::kChapterTag, toString(TextParagraphType::Heading1),
                      toString(TextParagraphType::Heading2), toString(TextParagraphType::Heading3),
                      toString(TextParagraphType::Heading4), toString(TextParagraphType::Heading5),
                      toString(TextParagraphType::Heading6), toString(TextParagraphType::Text),
                      toString(TextParagraphType::InlineNote) },
                    _parent)
    , d(new Implementation)
{
}

SimpleTextModel::~SimpleTextModel() = default;

QString SimpleTextModel::name() const
{
    return d->name;
}

void SimpleTextModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    const auto item = firstTextItem(d->rootItem);
    if (item != nullptr && item->type() == SimpleTextModelItemType::Text) {
        auto textItem = static_cast<SimpleTextModelTextItem*>(item);
        textItem->setText(_name);
    }

    d->name = _name;
    emit nameChanged(d->name);
}

void SimpleTextModel::setDocumentName(const QString& _name)
{
    setName(_name);
    emit documentNameChanged(_name);
}

void SimpleTextModel::setDocumentContent(const QByteArray& _content)
{
    clearDocument();
    document()->setContent(_content);
    initDocument();
}

void SimpleTextModel::appendItem(SimpleTextModelItem* _item, SimpleTextModelItem* _parentItem)
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

void SimpleTextModel::prependItem(SimpleTextModelItem* _item, SimpleTextModelItem* _parentItem)
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

void SimpleTextModel::insertItem(SimpleTextModelItem* _item, SimpleTextModelItem* _afterSiblingItem)
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

void SimpleTextModel::takeItem(SimpleTextModelItem* _item, SimpleTextModelItem* _parentItem)
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

void SimpleTextModel::removeItem(SimpleTextModelItem* _item)
{
    removeItems(_item, _item);
}

void SimpleTextModel::removeItems(SimpleTextModelItem* _fromItem, SimpleTextModelItem* _toItem)
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
    d->updateNumbering();
    endRemoveRows();

    updateItem(parentItem);
}

void SimpleTextModel::updateItem(SimpleTextModelItem* _item)
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

    //
    // Обновим название документа
    //
    if (_item == d->rootItem) {
        const auto item = firstTextItem(d->rootItem);
        if (item == nullptr || item->type() != SimpleTextModelItemType::Text) {
            setDocumentName({});
        } else {
            const auto textItem = static_cast<SimpleTextModelTextItem*>(item);
            setDocumentName(textItem->text());
        }
    }
}

QModelIndex SimpleTextModel::index(int _row, int _column, const QModelIndex& _parent) const
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

QModelIndex SimpleTextModel::parent(const QModelIndex& _child) const
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

int SimpleTextModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int SimpleTextModel::rowCount(const QModelIndex& _parent) const
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

Qt::ItemFlags SimpleTextModel::flags(const QModelIndex& _index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    const auto item = itemForIndex(_index);
    switch (item->type()) {
    case SimpleTextModelItemType::Chapter: {
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        break;
    }

    default:
        break;
    }

    return flags;
}

QVariant SimpleTextModel::data(const QModelIndex& _index, int _role) const
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

bool SimpleTextModel::canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row,
                                      int _column, const QModelIndex& _parent) const
{
    Q_UNUSED(_action);
    Q_UNUSED(_row);
    Q_UNUSED(_column);
    Q_UNUSED(_parent);

    //
    // TODO: вставлять можно только с понижением уровня заголовка
    //

    return _data->formats().contains(mimeTypes().constFirst());
}

bool SimpleTextModel::dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row,
                                   int _column, const QModelIndex& _parent)
{
    //
    // FIXME: возможен кейс, когда юзер тащит два заголовка один например
    //        четвёртого уровка, а следующий второго и вставляет их после
    //        главы третьего уровня, тогда второй элемент должен вставать
    //        не внутрь, а после
    //
    //        в главе 2 уровня лежит глава 4 уровня, переносишь в начало главы
    //        2 уровня главу 3-го уровня, глава 4го уровня должна встать внутрь
    //

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
            if (_parent.isValid() && rowCount(_parent) == _row) {
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
        SimpleTextModelItem* insertAnchorItem = itemForIndex(insertAnchorIndex);

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
        SimpleTextModelItem* lastItem = insertAnchorItem;
        while (!contentReader.atEnd()) {
            const auto currentTag = contentReader.name();
            if (currentTag == xml::kDocumentTag) {
                break;
            }

            //
            // ... вставляем главу
            //
            if (currentTag == xml::kChapterTag) {
                auto newChapterItem = new SimpleTextModelChapterItem(contentReader);

                if (!isFirstItemHandled) {
                    isFirstItemHandled = true;
                    //
                    // Вставить в начало главы
                    //
                    if (_row == 0) {
                        prependItem(newChapterItem, lastItem);
                    }
                    //
                    // Вставить в конец или середину главы
                    //
                    else {
                        //
                        // Если вставки идёт в конец, берём за якорь последний вложенный элемент
                        //
                        if (_row == -1) {
                            lastItem = lastItem->childAt(lastItem->childCount() - 1);
                        }

                        //
                        // Если вставка идёт после главы
                        //
                        if (lastItem->type() == SimpleTextModelItemType::Chapter) {
                            auto lastChapterItem
                                = static_cast<SimpleTextModelChapterItem*>(lastItem);
                            //
                            // ... и её уровень ниже, либо равен вставляемой
                            //
                            if (lastChapterItem->level() >= newChapterItem->level()) {
                                //
                                // ... то вставляем новую главу после неё
                                //
                                insertItem(newChapterItem, lastChapterItem);
                            }
                            //
                            // ... а если уровень вставляемой ниже
                            //
                            else {
                                //
                                // ... то вставляем внутрь
                                //
                                lastItem
                                    = lastChapterItem->childAt(lastChapterItem->childCount() - 1);
                                while (lastItem->type() == SimpleTextModelItemType::Chapter) {
                                    lastChapterItem
                                        = static_cast<SimpleTextModelChapterItem*>(lastItem);
                                    if (lastChapterItem->level() >= newChapterItem->level()) {
                                        insertItem(newChapterItem, lastChapterItem);
                                        break;
                                    }

                                    lastItem = lastChapterItem->childAt(
                                        lastChapterItem->childCount() - 1);
                                }
                                if (lastItem->type() != SimpleTextModelItemType::Chapter) {
                                    insertItem(newChapterItem, lastItem);
                                }
                            }
                        }
                        //
                        // А если вставка идёт после текстового элемента, то добавим после него
                        //
                        else {
                            insertItem(newChapterItem, lastItem);
                        }
                    }
                } else {
                    insertItem(newChapterItem, lastItem);
                }

                lastItem = newChapterItem;
            }
            //
            // ... вставляем текст
            //
            else {
                auto newTextItem = new SimpleTextModelTextItem(contentReader);

                if (!isFirstItemHandled) {
                    isFirstItemHandled = true;
                    //
                    // Вставить в начало lastItem
                    //
                    if (_row == 0) {
                        prependItem(newTextItem, lastItem);
                    }
                    //
                    // Вставить в конец lastItem
                    //
                    else if (_row == -1) {
                        appendItem(newTextItem, lastItem);
                    }
                    //
                    // Вставить после lastItem
                    //
                    else {
                        insertItem(newTextItem, lastItem);
                    }
                } else {
                    insertItem(newTextItem, lastItem);
                }

                lastItem = newTextItem;
            }
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

QMimeData* SimpleTextModel::mimeData(const QModelIndexList& _indexes) const
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

QStringList SimpleTextModel::mimeTypes() const
{
    return { kMimeType };
}

Qt::DropActions SimpleTextModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions SimpleTextModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QString SimpleTextModel::mimeFromSelection(const QModelIndex& _from, int _fromPosition,
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


    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type())
        + "\" version=\"1.0\">\n";

    auto buildXmlFor = [&xml, fromItem, _fromPosition, toItem, _toPosition,
                        _clearUuid](SimpleTextModelItem* _fromItemParent, int _fromItemRow) {
        for (int childIndex = _fromItemRow; childIndex < _fromItemParent->childCount();
             ++childIndex) {
            const auto childItem = _fromItemParent->childAt(childIndex);

            switch (childItem->type()) {
            case SimpleTextModelItemType::Chapter: {
                const auto folderItem = static_cast<SimpleTextModelChapterItem*>(childItem);
                xml += folderItem->toXml(fromItem, _fromPosition, toItem, _toPosition, _clearUuid);
                break;
            }

            case SimpleTextModelItemType::Text: {
                const auto textItem = static_cast<SimpleTextModelTextItem*>(childItem);
                if (textItem == fromItem && textItem == toItem) {
                    xml += textItem->toXml(_fromPosition, _toPosition - _fromPosition);
                } else if (textItem == fromItem) {
                    xml += textItem->toXml(_fromPosition,
                                           textItem->text().length() - _fromPosition);
                } else if (textItem == toItem) {
                    xml += textItem->toXml(0, _toPosition);
                } else {
                    xml += textItem->toXml();
                }
                break;
            }

            default: {
                xml += childItem->toXml();
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
    if (fromItem->type() == SimpleTextModelItemType::Text) {
        const auto textItem = static_cast<SimpleTextModelTextItem*>(fromItem);
        if (textItem->paragraphType() == TextParagraphType::Heading1
            || textItem->paragraphType() == TextParagraphType::Heading2
            || textItem->paragraphType() == TextParagraphType::Heading3
            || textItem->paragraphType() == TextParagraphType::Heading4
            || textItem->paragraphType() == TextParagraphType::Heading5
            || textItem->paragraphType() == TextParagraphType::Heading6) {
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
    return xml;
}

void SimpleTextModel::insertFromMime(const QModelIndex& _index, int _positionInBlock,
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
    QVector<SimpleTextModelItem*> lastItemsFromSourceScene;
    if (item->type() == SimpleTextModelItemType::Text) {
        auto textItem = static_cast<SimpleTextModelTextItem*>(item);
        //
        // Если в заголовок папки
        //
        if (textItem->paragraphType() == TextParagraphType::Heading1
            || textItem->paragraphType() == TextParagraphType::Heading2
            || textItem->paragraphType() == TextParagraphType::Heading3
            || textItem->paragraphType() == TextParagraphType::Heading4
            || textItem->paragraphType() == TextParagraphType::Heading5
            || textItem->paragraphType() == TextParagraphType::Heading6) {
            //
            // ... то вставим после него
            //
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
    }

    //
    // Считываем данные и последовательно вставляем в модель
    //
    QXmlStreamReader contentReader(_mimeData);
    contentReader.readNextStartElement(); // document
    contentReader.readNextStartElement();
    bool isFirstTextItemHandled = false;
    SimpleTextModelItem* lastItem = item;
    while (!contentReader.atEnd()) {
        const auto currentTag = contentReader.name();
        if (currentTag == xml::kDocumentTag) {
            break;
        }

        SimpleTextModelItem* newItem = nullptr;
        if (currentTag == xml::kChapterTag) {
            newItem = new SimpleTextModelChapterItem(contentReader);
        } else {
            auto newTextItem = new SimpleTextModelTextItem(contentReader);
            //
            // Если вставляется текстовый элемент внутрь уже существующего элемента
            //
            if (!isFirstTextItemHandled) {
                isFirstTextItemHandled = true;

                //
                // ... то просто объединим их
                //
                if (item->type() == SimpleTextModelItemType::Text
                    && !lastItemsFromSourceScene.contains(item)) {
                    auto textItem = static_cast<SimpleTextModelTextItem*>(item);
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
        auto item = new SimpleTextModelTextItem(contentReader);
        //
        // ... и последний вставленный элемент был текстовым
        //
        if (lastItem->type() == SimpleTextModelItemType::Text) {
            auto lastTextItem = static_cast<SimpleTextModelTextItem*>(lastItem);

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
            auto textItem = static_cast<SimpleTextModelTextItem*>(item);
            //
            // Удаляем пустые элементы модели
            //
            if (textItem->text().isEmpty()) {
                delete textItem;
                textItem = nullptr;
                continue;
            }

            insertItem(item, lastItem);

            lastItem = item;
        }
    }

    //
    // Завершаем изменение
    //
    emit rowsChanged();
}

SimpleTextModelItem* SimpleTextModel::itemForIndex(const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return d->rootItem;
    }

    auto item = static_cast<SimpleTextModelItem*>(_index.internalPointer());
    if (item == nullptr) {
        return d->rootItem;
    }

    return item;
}

QModelIndex SimpleTextModel::indexForItem(SimpleTextModelItem* _item) const
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

void SimpleTextModel::initDocument()
{
    //
    // Если документ пустой, создаём первоначальную структуру
    //
    if (document()->content().isEmpty()) {
        auto textItem = new SimpleTextModelTextItem;
        textItem->setParagraphType(TextParagraphType::Text);
        appendItem(textItem);
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
}

void SimpleTextModel::clearDocument()
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

QByteArray SimpleTextModel::toXml() const
{
    return d->toXml(document());
}

void SimpleTextModel::applyPatch(const QByteArray& _patch)
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

        QVector<SimpleTextModelItem*> items;
        while (!_reader.atEnd()) {
            const auto currentTag = _reader.name();
            SimpleTextModelItem* item = nullptr;
            if (currentTag == xml::kChapterTag) {
                item = new SimpleTextModelChapterItem(_reader);
            } else {
                item = new SimpleTextModelTextItem(_reader);
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
    std::function<QVector<SimpleTextModelItem*>(const QVector<SimpleTextModelItem*>&)>
        makeItemsPlain;
    makeItemsPlain = [&makeItemsPlain](const QVector<SimpleTextModelItem*>& _items) {
        QVector<SimpleTextModelItem*> itemsPlain;
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
    std::function<SimpleTextModelItem*(SimpleTextModelItem*)> findStartItem;
    findStartItem
        = [changes, &length, &findStartItem](SimpleTextModelItem* _item) -> SimpleTextModelItem* {
        if (changes.first.from == 0) {
            return _item->childAt(0);
        }

        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto child = _item->childAt(childIndex);
            const auto childLength = QString(child->toXml()).length();

            //
            // В этом элементе начинается изменение
            //
            if (changes.first.from >= length && changes.first.from < length + childLength) {
                //
                // Если есть дети, то уточняем поиск
                //
                int headerLength = 0;
                if (child->type() == SimpleTextModelItemType::Chapter) {
                    auto folder = static_cast<SimpleTextModelChapterItem*>(child);
                    headerLength = QString(folder->xmlHeader()).length();
                }

                if (child->hasChildren() && changes.first.from >= length + headerLength) {
                    length += headerLength;
                    return findStartItem(child);
                }
                //
                // В противном случае завершаем поиск
                //
                else {
                    return child;
                }
            }

            length += childLength;
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
    std::function<SimpleTextModelItem*(SimpleTextModelItem*, bool)> findNextItemWithChildren;
    findNextItemWithChildren
        = [&findNextItemWithChildren](SimpleTextModelItem* _item,
                                      bool _searchInChildren) -> SimpleTextModelItem* {
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
    auto findNextItem = [&findNextItemWithChildren](SimpleTextModelItem* _item) {
        return findNextItemWithChildren(_item, true);
    };
    //
    // И применяем их
    //
    emit rowsAboutToBeChanged();
    SimpleTextModelItem* previousModelItem = nullptr;
    //
    // В некоторых ситуациях мы не знаем сразу, куда будут извлечены элементы из удаляемого
    // элемента, или когда элемент вставляется посреди и отрезает часть вложенных элементов, поэтому
    // упаковываем их в список для размещения в правильном месте в следующем проходе
    //
    QVector<SimpleTextModelItem*> movedSiblingItems;
    auto updateItemPlacement = [this, &modelItem, &previousModelItem, newItemsPlain,
                                &movedSiblingItems](SimpleTextModelItem* _newItem,
                                                    SimpleTextModelItem* _item) {
        //
        // Определим предыдущий элемент из списка новых, в дальнейшем будем опираться
        // на его расположение относительно текущего нового
        //
        const auto newItemIndex = newItemsPlain.indexOf(_newItem);
        SimpleTextModelItem* previousNewItem
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
            SimpleTextModelItem* itemToInsert = nullptr;
            switch (newItem->type()) {
            case SimpleTextModelItemType::Chapter: {
                itemToInsert = new SimpleTextModelChapterItem;
                break;
            }

            case SimpleTextModelItemType::Text: {
                itemToInsert = new SimpleTextModelTextItem;
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
