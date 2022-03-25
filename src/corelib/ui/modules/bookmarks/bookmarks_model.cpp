#include "bookmarks_model.h"

#include <business_layer/model/text/text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/text_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <utils/shugar.h>
#include <utils/tools/model_index_path.h>

#include <QColor>
#include <QDateTime>
#include <QPointer>


namespace BusinessLayer {

class BookmarksModel::Implementation
{
public:
    explicit Implementation(BookmarksModel* _q);

    void processSourceModelRowsInserted(const QModelIndex& _parent, int _firstRow, int _lastRow);
    void processSourceModelRowsRemoved(const QModelIndex& _parent, int _firstRow, int _lastRow);
    void processSourceModelDataChanged(const QModelIndex& _index);


    BookmarksModel* q = nullptr;

    /**
     * @brief Фильтр типов параграфов, в которых разрешён поиск заметок
     */
    QSet<TextParagraphType> typesFilter;

    /**
     * @brief Модель сценария, на основе которой строится модель заметок
     */
    QPointer<TextModel> model;
    /**
     * @brief Список всех текстовых элементов модели, для того, чтобы понимать их последовательность
     */
    QVector<TextModelTextItem*> modelTextItems;

    /**
     * @brief Список элементов сценария с заметками
     */
    QVector<TextModelTextItem*> bookmarks;
};

BookmarksModel::Implementation::Implementation(BookmarksModel* _q)
    : q(_q)
{
}

void BookmarksModel::Implementation::processSourceModelRowsInserted(const QModelIndex& _parent,
                                                                    int _firstRow, int _lastRow)
{
    //
    // Для каждого из вставленных
    //
    const int invalidPosition = -1;
    int lastInsertPosition = invalidPosition;
    for (int row = _firstRow; row <= _lastRow; ++row) {
        //
        // Игнорируем не текстовые элементы
        //
        const auto itemIndex = model->index(row, 0, _parent);
        const auto item = model->itemForIndex(itemIndex);
        if (item == nullptr || item->type() != TextModelItemType::Text) {
            if (item && item->hasChildren()) {
                processSourceModelRowsInserted(itemIndex, 0, item->childCount() - 1);
            }
            continue;
        }

        auto textItem = static_cast<TextModelTextItem*>(item);

        //
        // Пропускаем корректировочные блоки
        //
        if (textItem->isCorrection()) {
            continue;
        }

        //
        // Если задан фильтр и блок не проходит его, пропускаем данный блок
        //
        if (!typesFilter.isEmpty() && !typesFilter.contains(textItem->paragraphType())) {
            break;
        }

        //
        // Вставляем текстовый элемент в общий список на своё место
        //
        if (lastInsertPosition == invalidPosition) {
            const auto itemIndexPath = ModelIndexPath(itemIndex);
            for (int index = 0; index < modelTextItems.size(); ++index) {
                const auto modelTextItem = modelTextItems.at(index);
                const auto modelTextItemIndex = model->indexForItem(modelTextItem);
                if (itemIndexPath < ModelIndexPath(modelTextItemIndex)) {
                    lastInsertPosition = index;
                    break;
                }
            }
            if (lastInsertPosition == invalidPosition) {
                lastInsertPosition = modelTextItems.size();
            }
        } else {
            ++lastInsertPosition;
        }
        modelTextItems.insert(lastInsertPosition, textItem);

        if (!textItem->bookmark().has_value() || !textItem->bookmark()->isValid()) {
            continue;
        }

        //
        // Определяем заметки из вставленного блока
        //
        processSourceModelDataChanged(itemIndex);
    }
}

void BookmarksModel::Implementation::processSourceModelRowsRemoved(const QModelIndex& _parent,
                                                                   int _firstRow, int _lastRow)
{
    //
    // Для каждого из удаляемых
    //
    for (int row = _firstRow; row <= _lastRow; ++row) {
        //
        // Игнорируем не текстовые элементы
        //
        const auto itemIndex = model->index(row, 0, _parent);
        const auto item = model->itemForIndex(itemIndex);
        if (item == nullptr) {
            continue;
        } else if (item->type() != TextModelItemType::Text && item->hasChildren()) {
            processSourceModelRowsRemoved(itemIndex, 0, item->childCount() - 1);
            continue;
        }

        //
        // Собираем и удаляем все заметки, связанные с удаляемым элементом
        //
        auto textItem = static_cast<TextModelTextItem*>(item);

        //
        // Исключим удаляемый элемент из списка
        //
        modelTextItems.removeAll(textItem);

        //
        // Пропускаем корректировочные блоки и блоки не проходящие фильтр
        //
        if (textItem->isCorrection()
            || (!typesFilter.isEmpty() && !typesFilter.contains(textItem->paragraphType()))) {
            continue;
        }

        //
        // Пропускаем блоки, которых не было в списке блоков с закладками
        //
        const auto indexToRemove = bookmarks.indexOf(textItem);
        if (indexToRemove == -1) {
            continue;
        }

        //
        // Удаляем элемент с закладкой из модели
        //
        q->beginRemoveRows({}, indexToRemove, indexToRemove);
        bookmarks.removeAt(indexToRemove);
        q->endRemoveRows();

        //
        // Удаляем текстовый элемент из общего списка
        //
        modelTextItems.removeAll(textItem);
    }
}

void BookmarksModel::Implementation::processSourceModelDataChanged(const QModelIndex& _index)
{
    if (!_index.isValid()) {
        return;
    }

    const auto item = model->itemForIndex(_index);
    if (item == nullptr || item->type() != TextModelItemType::Text) {
        return;
    }

    auto textItem = static_cast<TextModelTextItem*>(item);

    //
    // Пропускаем корректировочные блоки и блоки не проходящие фильтр
    //
    if (textItem->isCorrection()
        || (!typesFilter.isEmpty() && !typesFilter.contains(textItem->paragraphType()))) {
        //
        // А если раньше блок был не корректировочным, исключим его из списка
        //
        modelTextItems.removeAll(textItem);
        return;
    }

    //
    // Сохраним закладку, если ещё не была сохранена
    //
    if (textItem->bookmark().has_value() && textItem->bookmark()->isValid()) {
        const auto bookmarkItemIndex = bookmarks.indexOf(textItem);
        if (bookmarkItemIndex == -1) {
            //
            // Определим правильное место для вставки
            //
            bool inserted = false;
            for (int siblingIndex = modelTextItems.indexOf(textItem) - 1; siblingIndex >= 0;
                 --siblingIndex) {
                const auto siblingItem = modelTextItems.at(siblingIndex);
                const auto& siblingItemBookmark = siblingItem->bookmark();
                if (!siblingItemBookmark.has_value() || !siblingItemBookmark->isValid()) {
                    continue;
                }

                const auto indexToInsert = bookmarks.indexOf(siblingItem) + 1;
                q->beginInsertRows({}, indexToInsert, indexToInsert);
                bookmarks.insert(indexToInsert, textItem);
                q->endInsertRows();
                inserted = true;
                break;
            }
            //
            // Если перед элементом не нашлось другого с закладкой, то вставим в самое начало
            //
            if (!inserted) {
                q->beginInsertRows({}, 0, 0);
                bookmarks.prepend(textItem);
                q->endInsertRows();
            }
        }
        //
        // А если закладка была сохранена, то уведомим о том, что элемент изменился
        //
        else {
            const auto updatedItemIndex = q->index(bookmarkItemIndex, 0);
            emit q->dataChanged(updatedItemIndex, updatedItemIndex);
        }
    }
    //
    // или удалим
    //
    else {
        bookmarks.removeAll(textItem);
    }
}


// ****


BookmarksModel::BookmarksModel(QObject* _parent)
    : QAbstractListModel(_parent)
    , d(new Implementation(this))
{
}

BookmarksModel::~BookmarksModel() = default;

void BookmarksModel::setParagraphTypesFiler(const QVector<TextParagraphType>& _types)
{
    d->typesFilter = { _types.begin(), _types.end() };
}

void BookmarksModel::setTextModel(TextModel* _model)
{
    beginResetModel();

    if (d->model != nullptr) {
        d->model->disconnect(this);
        d->modelTextItems.clear();
        d->bookmarks.clear();
    }

    d->model = _model;

    if (d->model != nullptr) {
        std::function<void(const QModelIndex&)> readBookmarksFromModel;
        readBookmarksFromModel = [this, &readBookmarksFromModel](const QModelIndex& _parent) {
            using namespace BusinessLayer;
            for (int itemRow = 0; itemRow < d->model->rowCount(_parent); ++itemRow) {
                const auto itemIndex = d->model->index(itemRow, 0, _parent);
                const auto item = d->model->itemForIndex(itemIndex);
                if (item->type() == TextModelItemType::Text) {
                    const auto textItem = static_cast<TextModelTextItem*>(item);

                    //
                    // Пропускаем корректировочные блоки
                    //
                    if (textItem->isCorrection()) {
                        continue;
                    }

                    //
                    // Сохраним текстовый элемент в плоском списке
                    //
                    d->modelTextItems.append(textItem);

                    //
                    // Если вставился абзац без закладок, пропускаем его
                    //
                    if (!textItem->bookmark().has_value() || !textItem->bookmark()->isValid()) {
                        continue;
                    }

                    //
                    // Если задан фильтр абзац не проходит его, пропускаем абзац
                    //
                    if (!d->typesFilter.isEmpty()
                        && !d->typesFilter.contains(textItem->paragraphType())) {
                        continue;
                    }

                    //
                    // Если в абзаце есть закладка, сохраним
                    //
                    d->bookmarks.append(textItem);
                }

                //
                // Считываем информацию о детях
                //
                readBookmarksFromModel(itemIndex);
            }
        };
        readBookmarksFromModel({});

        connect(d->model, &TextModel::modelReset, this, [this] { setTextModel(d->model); });
        connect(d->model, &TextModel::rowsInserted, this,
                [this](const QModelIndex& _parent, int _first, int _last) {
                    d->processSourceModelRowsInserted(_parent, _first, _last);
                });
        connect(d->model, &TextModel::rowsAboutToBeRemoved, this,
                [this](const QModelIndex& _parent, int _first, int _last) {
                    d->processSourceModelRowsRemoved(_parent, _first, _last);
                });
        connect(d->model, &TextModel::dataChanged, this,
                [this](const QModelIndex& _topLeft, const QModelIndex& _bottomRight) {
                    Q_ASSERT(_topLeft == _bottomRight);
                    d->processSourceModelDataChanged(_topLeft);
                });
    }

    endResetModel();
}

QModelIndex BookmarksModel::mapToModel(const QModelIndex& _index)
{
    if (!_index.isValid() || _index.row() >= d->bookmarks.size()) {
        return {};
    }

    return d->model->indexForItem(d->bookmarks.at(_index.row()));
}

QModelIndex BookmarksModel::mapFromModel(const QModelIndex& _index)
{
    if (!_index.isValid()) {
        return {};
    }

    const auto item = d->model->itemForIndex(_index);
    if (item == nullptr || item->type() != TextModelItemType::Text) {
        return {};
    }

    auto textItem = static_cast<TextModelTextItem*>(item);
    return index(d->bookmarks.indexOf(textItem), 0);
}

void BookmarksModel::setName(const QModelIndex& _index, const QString& _name)
{
    auto item = d->bookmarks.at(_index.row());
    if (!item->bookmark().has_value()) {
        return;
    }

    auto bookmark = item->bookmark().value();
    bookmark.name = _name;
    item->setBookmark(bookmark);
    d->model->updateItem(item);
}

void BookmarksModel::setColor(const QModelIndex& _index, const QColor& _color)
{
    auto item = d->bookmarks.at(_index.row());
    if (!item->bookmark().has_value()) {
        return;
    }

    auto bookmark = item->bookmark().value();
    bookmark.color = _color;
    item->setBookmark(bookmark);
    d->model->updateItem(item);
}

void BookmarksModel::remove(const QModelIndexList& _indexes)
{
    for (const auto& index : reversed(_indexes)) {
        const auto textItem = d->bookmarks.at(index.row());
        textItem->clearBookmark();
        d->model->updateItem(textItem);
    }
}

int BookmarksModel::rowCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)

    return d->bookmarks.size();
}

QVariant BookmarksModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    if (_index.row() >= d->bookmarks.size()) {
        return {};
    }

    const auto item = d->bookmarks.at(_index.row());
    switch (_role) {
    case Qt::DecorationRole: {
        return u8"\U000F00C0";
    }

    case BookmarkColorRole: {
        return item->bookmark()->color;
    }

    case BookmarkNameRole: {
        return item->bookmark()->name;
    }

    case BookmarkItemTextRole: {
        return item->text();
    }

    default: {
        return {};
    }
    }
}

} // namespace BusinessLayer
