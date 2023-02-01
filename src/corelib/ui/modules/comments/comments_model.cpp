#include "comments_model.h"

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

class CommentsModel::Implementation
{
public:
    explicit Implementation(CommentsModel* _q);

    void saveReviewMark(TextModelTextItem* _textItem,
                        const TextModelTextItem::ReviewMark& _reviewMark);

    void processSourceModelRowsInserted(const QModelIndex& _parent, int _firstRow, int _lastRow);
    void processSourceModelRowsRemoved(const QModelIndex& _parent, int _firstRow, int _lastRow);
    void processSourceModelDataChanged(const QModelIndex& _index);


    CommentsModel* q = nullptr;

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
     * @brief Вспомогательная структура для хранения заметки и группы элементов, к которой она
     * относится
     */
    struct ReviewMarkWrapper {
        /**
         * @brief Сама заметка, на основе которой строится элемент
         */
        TextModelTextItem::ReviewMark reviewMark;

        /**
         * @brief Начало заметки в первом из элементов
         */
        int fromInFirstItem = 0;

        /**
         * @brief Конец заметки в последнем из элементов
         */
        int toInLastItem = 0;

        /**
         * @brief Список абзацев, в которых находится заметка
         */
        QVector<TextModelTextItem*> items;


        bool operator==(const ReviewMarkWrapper& _other) const;
    };
    /**
     * @brief Список заметок сценария
     */
    QVector<ReviewMarkWrapper> reviewMarks;
};

CommentsModel::Implementation::Implementation(CommentsModel* _q)
    : q(_q)
{
}

void CommentsModel::Implementation::saveReviewMark(TextModelTextItem* _textItem,
                                                   const TextModelTextItem::ReviewMark& _reviewMark)
{
    bool reviewMarkAdded = false;

    do {
        if (reviewMarks.empty()) {
            break;
        }

        if (!modelTextItems.isEmpty() && _textItem == modelTextItems.constFirst()) {
            break;
        }

        //
        // Если заметка начинается в начале абзаца, проверяем нельзя ли её присовокупить к заметке в
        // конце предыдущего абзаца
        //
        if (_reviewMark.from == 0) {
            //
            // Смотрим заметки которые есть в предыдущем блоке
            //
            const auto previewTextItemIndex = modelTextItems.indexOf(_textItem) - 1;
            if (previewTextItemIndex < 0) {
                break;
            }
            auto previousTextItem = modelTextItems.at(previewTextItemIndex);
            QVector<ReviewMarkWrapper> previosTextItemReviewMarkWrappers;
            for (const auto& reviewMarkWrapper : std::as_const(reviewMarks)) {
                if (reviewMarkWrapper.items.contains(previousTextItem)) {
                    previosTextItemReviewMarkWrappers.append(reviewMarkWrapper);
                    continue;
                }

                if (!previosTextItemReviewMarkWrappers.isEmpty()) {
                    break;
                }
            }
            if (previosTextItemReviewMarkWrappers.isEmpty()) {
                break;
            }

            //
            // Берём последнюю из заметок
            //
            const auto previousTextItemLastReviewMarkWrapper
                = previosTextItemReviewMarkWrappers.constLast();
            //
            // ... присовокупляем, если она имеет аналогичное форматирование и заканчивается в конце
            // абзаца
            //
            if (previousTextItemLastReviewMarkWrapper.reviewMark.isPartiallyEqual(_reviewMark)
                && previousTextItemLastReviewMarkWrapper.toInLastItem
                    == previousTextItem->text().length()) {
                auto& reviewMarkWrapper
                    = reviewMarks[reviewMarks.indexOf(previousTextItemLastReviewMarkWrapper)];
                reviewMarkWrapper.items.append(_textItem);
                reviewMarkWrapper.toInLastItem = _reviewMark.length;
                reviewMarkAdded = true;
            }
        }
        //
        // А если не вначале, то возможно просто разное форматирование текста внутри одной заметки
        //
        else {
            //
            // Смотрим заметки которые есть в текущем блоке
            //
            QVector<ReviewMarkWrapper> textItemReviewMarkWrappers;
            for (const auto& reviewMarkWrapper : std::as_const(reviewMarks)) {
                if (reviewMarkWrapper.items.contains(_textItem)) {
                    textItemReviewMarkWrappers.append(reviewMarkWrapper);
                    continue;
                }

                if (!textItemReviewMarkWrappers.isEmpty()) {
                    break;
                }
            }
            if (textItemReviewMarkWrappers.isEmpty()) {
                break;
            }

            //
            // Ищем ближайшую заметку
            //
            for (const auto& textItemReviewMarkWrapper :
                 std::as_const(textItemReviewMarkWrappers)) {
                //
                // ... присовокупляем, если она имеет аналогичное форматирование и заканчивается в
                // конце абзаца
                //
                if (textItemReviewMarkWrapper.reviewMark.isPartiallyEqual(_reviewMark)) {
                    auto& reviewMarkWrapper
                        = reviewMarks[reviewMarks.indexOf(textItemReviewMarkWrapper)];
                    //
                    // ... если выделения в одном блоке
                    //
                    if (reviewMarkWrapper.items.size() == 1) {
                        //
                        // ... расширяется вперёд
                        //
                        if (reviewMarkWrapper.toInLastItem == _reviewMark.from) {
                            reviewMarkWrapper.toInLastItem = _reviewMark.end();
                            reviewMarkAdded = true;
                        }
                        //
                        // ... просто изменилось форматирование внутри блока
                        //
                        else if (reviewMarkWrapper.fromInFirstItem <= _reviewMark.from
                                 && reviewMarkWrapper.toInLastItem >= _reviewMark.end()) {
                            reviewMarkAdded = true;
                        }
                    }
                    //
                    // ... если выделение на несколько блоков
                    //
                    else {
                        //
                        // ... расширяется вперёд
                        //
                        if (reviewMarkWrapper.items.constLast() == _textItem
                            && reviewMarkWrapper.toInLastItem == _reviewMark.from) {
                            reviewMarkWrapper.toInLastItem = _reviewMark.end();
                            reviewMarkAdded = true;
                        }
                        //
                        // ... просто изменилось форматирование внутри
                        //
                        else if ((reviewMarkWrapper.items.constFirst() == _textItem
                                  && reviewMarkWrapper.fromInFirstItem <= _reviewMark.from)
                                 || (reviewMarkWrapper.items.constFirst() != _textItem
                                     && reviewMarkWrapper.items.constLast() != _textItem)
                                 || (reviewMarkWrapper.items.constLast() == _textItem
                                     && reviewMarkWrapper.toInLastItem >= _reviewMark.end())) {
                            reviewMarkAdded = true;
                        }
                    }
                }
            }
        }
    }
    once;

    if (reviewMarkAdded) {
        return;
    }

    //
    // Если заметка внутри абзаца, просто сохраняем её
    //
    Implementation::ReviewMarkWrapper reviewMarkWrapper;
    reviewMarkWrapper.items.append(_textItem);
    reviewMarkWrapper.reviewMark = _reviewMark;
    reviewMarkWrapper.fromInFirstItem = _reviewMark.from;
    reviewMarkWrapper.toInLastItem = _reviewMark.end();

    int insertIndex = 0;
    const auto textItemIndex = modelTextItems.indexOf(_textItem);
    for (; insertIndex < reviewMarks.size(); ++insertIndex) {
        const auto insertBeforeReviewMarkWrapper = reviewMarks.at(insertIndex);
        const auto reviewMarkWrapperTextItemIndex
            = modelTextItems.indexOf(insertBeforeReviewMarkWrapper.items.constLast());
        //
        // Если элемент идёт после вставляемого
        //
        if (textItemIndex < reviewMarkWrapperTextItemIndex
            //
            // ... или если в том же элементе, тогда сортируем по расположению самой заметки
            //
            || (textItemIndex == reviewMarkWrapperTextItemIndex
                && reviewMarkWrapper.fromInFirstItem
                    < insertBeforeReviewMarkWrapper.fromInFirstItem)) {
            break;
        }
    }
    q->beginInsertRows({}, insertIndex, insertIndex);
    reviewMarks.insert(insertIndex, reviewMarkWrapper);
    q->endInsertRows();
}

void CommentsModel::Implementation::processSourceModelRowsInserted(const QModelIndex& _parent,
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

        //
        // Вставляем текстовый элемент в общий список на своё место
        //
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

        //
        // Если новый элемент вставился посередине уже существующей заметки
        //
        if (lastInsertPosition > 0) {
            auto previousTextItem = modelTextItems.value(lastInsertPosition - 1);
            QVector<ReviewMarkWrapper> previousItemReviewMarkWrappers;
            for (const auto& reviewMarkWrapper : std::as_const(reviewMarks)) {
                if (reviewMarkWrapper.items.contains(previousTextItem)) {
                    previousItemReviewMarkWrappers.append(reviewMarkWrapper);
                    continue;
                }

                if (!previousItemReviewMarkWrappers.isEmpty()) {
                    break;
                }
            }

            //
            // Если на границе вставляемого элемента есть состовная редакторская заметка
            //
            if (!previousItemReviewMarkWrappers.isEmpty()
                && previousItemReviewMarkWrappers.constLast().items.size() > 1) {
                const auto previousItemReviewMarkWrapper
                    = previousItemReviewMarkWrappers.constLast();
                const auto previousItemReviewMarkWrapperIndex
                    = reviewMarks.indexOf(previousItemReviewMarkWrapper);
                //
                // ... и вставляемый блок находится у неё в середине, то разделяем её на две
                //
                if (previousTextItem != previousItemReviewMarkWrapper.items.constLast()) {
                    //
                    // ... вырежем из верхней, всё, что перенеслось в нижнюю часть
                    //
                    auto topCorrectedReviewMarkWrapper = previousItemReviewMarkWrapper;
                    do {
                        topCorrectedReviewMarkWrapper.items.removeLast();
                    } while (topCorrectedReviewMarkWrapper.items.constLast() != previousTextItem);
                    topCorrectedReviewMarkWrapper.toInLastItem
                        = topCorrectedReviewMarkWrapper.items.constLast()
                              ->reviewMarks()
                              .constLast()
                              .end();
                    reviewMarks[previousItemReviewMarkWrapperIndex] = topCorrectedReviewMarkWrapper;
                    //
                    // ... добавим заметку снизу
                    //
                    auto bottomCorrectedReviewMarkWrapper = previousItemReviewMarkWrapper;
                    while (bottomCorrectedReviewMarkWrapper.items.constFirst()
                           != previousTextItem) {
                        bottomCorrectedReviewMarkWrapper.items.removeFirst();
                    };
                    bottomCorrectedReviewMarkWrapper.items.removeFirst();
                    bottomCorrectedReviewMarkWrapper.fromInFirstItem
                        = bottomCorrectedReviewMarkWrapper.items.constFirst()
                              ->reviewMarks()
                              .constFirst()
                              .from;
                    const int bottomCorrectedReviewMarkWrapperIndex
                        = previousItemReviewMarkWrapperIndex + 1;
                    q->beginInsertRows({}, bottomCorrectedReviewMarkWrapperIndex,
                                       bottomCorrectedReviewMarkWrapperIndex);
                    reviewMarks.insert(bottomCorrectedReviewMarkWrapperIndex,
                                       bottomCorrectedReviewMarkWrapper);
                    q->endInsertRows();
                }
            }
        }

        //
        // Определяем заметки из вставленного блока
        //
        processSourceModelDataChanged(itemIndex);
    }
}

void CommentsModel::Implementation::processSourceModelRowsRemoved(const QModelIndex& _parent,
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
        // Пропускаем корректировочные блоки и блоки не проходящие фильтр
        //
        if (textItem->isCorrection()
            || (!typesFilter.isEmpty() && !typesFilter.contains(textItem->paragraphType()))) {
            //
            // Исключим его из списка
            //
            modelTextItems.removeAll(textItem);
            continue;
        }

        QVector<ReviewMarkWrapper> reviewMarkWrappersToDelete;
        for (const auto& reviewMarkWrapper : std::as_const(reviewMarks)) {
            if (reviewMarkWrapper.items.contains(textItem)) {
                reviewMarkWrappersToDelete.append(reviewMarkWrapper);
                continue;
            }

            if (!reviewMarkWrappersToDelete.isEmpty()) {
                break;
            }
        }

        //
        // Удаляем заметки
        //
        for (const auto& reviewMarkWrapper : reviewMarkWrappersToDelete) {
            const auto reviewMarkWrapperIndex = reviewMarks.lastIndexOf(reviewMarkWrapper);

            //
            // Если эта заметка относится только к текущему блоку, просто удалим её
            //
            if (reviewMarkWrapper.items.size() == 1) {
                q->beginRemoveRows({}, reviewMarkWrapperIndex, reviewMarkWrapperIndex);
                reviewMarks.removeAll(reviewMarkWrapper);
                q->endRemoveRows();
            }
            //
            // В противном случае исключаем текущий блок из блоков заметки
            //
            else {
                //
                // ... отрезаем от начала
                //
                if (textItem == reviewMarkWrapper.items.constFirst()) {
                    auto correctedReviewMarkWrapper = reviewMarkWrapper;
                    correctedReviewMarkWrapper.items.removeFirst();
                    correctedReviewMarkWrapper.fromInFirstItem
                        = correctedReviewMarkWrapper.items.constFirst()
                              ->reviewMarks()
                              .constFirst()
                              .from;
                    reviewMarks[reviewMarkWrapperIndex] = correctedReviewMarkWrapper;
                }
                //
                // ... отрезаем от конца
                //
                else if (textItem == reviewMarkWrapper.items.constLast()) {
                    auto correctedReviewMarkWrapper = reviewMarkWrapper;
                    correctedReviewMarkWrapper.items.removeLast();
                    correctedReviewMarkWrapper.toInLastItem
                        = correctedReviewMarkWrapper.items.constLast()
                              ->reviewMarks()
                              .constLast()
                              .end();
                    reviewMarks[reviewMarkWrapperIndex] = correctedReviewMarkWrapper;
                }
                //
                // ... вырезаем из середины
                //
                else {
                    auto correctedReviewMarkWrapper = reviewMarkWrapper;
                    correctedReviewMarkWrapper.items.removeAll(textItem);
                    reviewMarks[reviewMarkWrapperIndex] = correctedReviewMarkWrapper;
                }
            }
        }

        //
        // Удаляем текстовый элемент из общего списка
        //
        modelTextItems.removeAll(textItem);
    }
}

void CommentsModel::Implementation::processSourceModelDataChanged(const QModelIndex& _index)
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

    QVector<ReviewMarkWrapper> oldReviewMarkWrappers;
    for (const auto& reviewMarkWrapper : std::as_const(reviewMarks)) {
        if (reviewMarkWrapper.items.contains(textItem)) {
            oldReviewMarkWrappers.append(reviewMarkWrapper);
            continue;
        }

        if (!oldReviewMarkWrappers.isEmpty()) {
            break;
        }
    }

    //
    // Если в абзаце не было заметок
    //
    if (oldReviewMarkWrappers.isEmpty()) {
        //
        // Если заметки и не появилось, игнорируем абзац
        //
        if (textItem->reviewMarks().isEmpty()) {
            return;
        }

        //
        // Если заметка появилась
        //
        for (const auto& reviewMark : std::as_const(textItem->reviewMarks())) {
            saveReviewMark(textItem, reviewMark);
        }
    }
    //
    // Если в абзаце были заметки
    //
    else {
        for (int newReviewMarkIndex = 0; newReviewMarkIndex < textItem->reviewMarks().size();
             ++newReviewMarkIndex) {
            bool hasSimilar = false;
            const auto newReviewMark = textItem->reviewMarks().at(newReviewMarkIndex);
            for (int oldReviewMarkIndex = 0; oldReviewMarkIndex < oldReviewMarkWrappers.size();
                 ++oldReviewMarkIndex) {
                const auto oldReviewMarkWrapper = oldReviewMarkWrappers.value(oldReviewMarkIndex);
                //
                // Обновим заметку, если она осталась на месте
                //
                if (newReviewMark.isPartiallyEqual(oldReviewMarkWrapper.reviewMark)) {
                    auto newReviewMarkWrapper = oldReviewMarkWrapper;
                    newReviewMarkWrapper.reviewMark = newReviewMark;
                    if (newReviewMarkWrapper.items.size() == 1) {
                        newReviewMarkWrapper.fromInFirstItem = newReviewMark.from;
                        newReviewMarkWrapper.toInLastItem = newReviewMark.end();
                    } else if (textItem == newReviewMarkWrapper.items.constFirst()) {
                        newReviewMarkWrapper.fromInFirstItem = newReviewMark.from;
                    } else if (textItem == newReviewMarkWrapper.items.last()) {
                        newReviewMarkWrapper.toInLastItem = newReviewMark.end();
                    }

                    //
                    // Перезаписываем на обновлённый
                    //
                    const auto wrapperIndex = reviewMarks.indexOf(oldReviewMarkWrapper);
                    reviewMarks[wrapperIndex] = newReviewMarkWrapper;
                    const auto changedItemModelIndex = q->index(wrapperIndex, 0);
                    emit q->dataChanged(changedItemModelIndex, changedItemModelIndex);

                    oldReviewMarkWrappers.removeAt(oldReviewMarkIndex);
                    hasSimilar = true;
                    break;
                }
            }

            //
            // Если идентичной заметки не нашлось, добавляем новую заметку
            //
            if (!hasSimilar) {
                saveReviewMark(textItem, newReviewMark);
            }
        }

        //
        // Удаляем старые заметки, которых нет в обновлённом блоке
        //
        for (int oldReviewMarkIndex = 0; oldReviewMarkIndex < oldReviewMarkWrappers.size();
             ++oldReviewMarkIndex) {
            const auto oldReviewMarkWrapper = oldReviewMarkWrappers.value(oldReviewMarkIndex);
            const auto oldReviewMarkWrapperIndex = reviewMarks.indexOf(oldReviewMarkWrapper);
            q->beginRemoveRows({}, oldReviewMarkWrapperIndex, oldReviewMarkWrapperIndex);
            reviewMarks.remove(oldReviewMarkWrapperIndex);
            q->endRemoveRows();

            //
            // Если заметка включала в себя несколько блоков, то перестроим для них заметки
            //
            if (oldReviewMarkWrapper.items.size() > 1) {
                auto itemsToUpdate = oldReviewMarkWrapper.items;
                itemsToUpdate.removeAll(textItem);
                for (auto item : itemsToUpdate) {
                    processSourceModelDataChanged(model->indexForItem(item));
                }
            }
        }
    }
}

bool CommentsModel::Implementation::ReviewMarkWrapper::operator==(
    const CommentsModel::Implementation::ReviewMarkWrapper& _other) const
{
    return reviewMark == _other.reviewMark && fromInFirstItem == _other.fromInFirstItem
        && toInLastItem == _other.toInLastItem && items == _other.items;
}


// ****


CommentsModel::CommentsModel(QObject* _parent)
    : QAbstractListModel(_parent)
    , d(new Implementation(this))
{
}

CommentsModel::~CommentsModel() = default;

void CommentsModel::setParagraphTypesFiler(const QVector<TextParagraphType>& _types)
{
    d->typesFilter = { _types.begin(), _types.end() };
}

void CommentsModel::setTextModel(TextModel* _model)
{
    beginResetModel();

    if (d->model != nullptr) {
        d->model->disconnect(this);
        d->modelTextItems.clear();
        d->reviewMarks.clear();
    }

    d->model = _model;

    if (d->model != nullptr) {
        std::function<void(const QModelIndex&)> readReviewMarksFromModel;
        readReviewMarksFromModel = [this, &readReviewMarksFromModel](const QModelIndex& _parent) {
            using namespace BusinessLayer;
            for (int itemRow = 0; itemRow < d->model->rowCount(_parent); ++itemRow) {
                const auto itemIndex = d->model->index(itemRow, 0, _parent);
                const auto item = d->model->itemForIndex(itemIndex);
                switch (item->type()) {
                case TextModelItemType::Text: {
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
                    // Если вставился абзац без заметок, пропускаем его
                    //
                    if (textItem->reviewMarks().isEmpty()) {
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
                    // Если в абзаце есть заметка
                    //
                    for (const auto& reviewMark : std::as_const(textItem->reviewMarks())) {
                        //
                        // Если заметка начинается в начале абзаца, проверяем нельзя ли её
                        // присовокупить к заметке в конце предыдущего абзаца
                        //
                        if (!d->reviewMarks.isEmpty()) {
                            if (reviewMark.from == 0) {
                                auto& lastReviewMarkWrapper = d->reviewMarks.last();
                                //
                                // В предыдущем блоке есть заметка с аналогичным форматированием и
                                // заканчивается она в конце абзаца
                                //
                                if (lastReviewMarkWrapper.items.last()
                                        == d->modelTextItems.at(d->modelTextItems.size() - 2)
                                    && lastReviewMarkWrapper.reviewMark.isPartiallyEqual(reviewMark)
                                    && lastReviewMarkWrapper.toInLastItem
                                        == lastReviewMarkWrapper.items.constLast()
                                               ->text()
                                               .length()) {
                                    lastReviewMarkWrapper.items.append(textItem);
                                    lastReviewMarkWrapper.toInLastItem = reviewMark.length;
                                    continue;
                                }
                            }
                            //
                            // А если не вначале, то возможно в этой заметке просто разное
                            // форматирование текста
                            //
                            else {
                                auto& lastReviewMarkWrapper = d->reviewMarks.last();
                                //
                                // В предыдущем блоке есть заметка с аналогичным форматированием и
                                // заканчивается она в конце абзаца
                                //
                                if (lastReviewMarkWrapper.items.last() == textItem
                                    && lastReviewMarkWrapper.reviewMark.isPartiallyEqual(reviewMark)
                                    && lastReviewMarkWrapper.toInLastItem == reviewMark.from) {
                                    lastReviewMarkWrapper.items.append(textItem);
                                    lastReviewMarkWrapper.toInLastItem = reviewMark.end();
                                    continue;
                                }
                            }
                        }

                        //
                        // В противном случае, прото сохраняем заметку
                        //
                        Implementation::ReviewMarkWrapper reviewMarkWrapper;
                        reviewMarkWrapper.items.append(textItem);
                        reviewMarkWrapper.reviewMark = reviewMark;
                        reviewMarkWrapper.fromInFirstItem = reviewMark.from;
                        reviewMarkWrapper.toInLastItem = reviewMark.end();
                        d->reviewMarks.append(reviewMarkWrapper);
                    }
                    break;
                }

                default:
                    break;
                }

                //
                // Считываем информацию о детях
                //
                readReviewMarksFromModel(itemIndex);
            }
        };
        readReviewMarksFromModel({});

        connect(d->model, &TextModel::modelAboutToBeReset, this, [this] {
            d->modelTextItems.clear();
            d->reviewMarks.clear();
        });
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

CommentsModel::PositionHint CommentsModel::mapToModel(const QModelIndex& _index)
{
    if (!_index.isValid() || _index.row() >= d->reviewMarks.size()) {
        return {};
    }

    const auto reviewMarkWrapper = d->reviewMarks.at(_index.row());
    return { d->model->indexForItem(reviewMarkWrapper.items.constFirst()),
             reviewMarkWrapper.fromInFirstItem };
}

QModelIndex CommentsModel::mapFromModel(const QModelIndex& _index, int _positionInBlock)
{
    if (!_index.isValid()) {
        return {};
    }

    const auto item = d->model->itemForIndex(_index);
    if (item == nullptr || item->type() != TextModelItemType::Text) {
        return {};
    }

    auto textItem = static_cast<TextModelTextItem*>(item);

    //
    // Пропускаем корректировочные блоки
    //
    if (textItem->isCorrection()) {
        return {};
    }

    for (const auto& reviewMarkWrapper : std::as_const(d->reviewMarks)) {
        if (!reviewMarkWrapper.items.contains(textItem)) {
            continue;
        }

        for (const auto& reviewMark : textItem->reviewMarks()) {
            if (!reviewMark.isPartiallyEqual(reviewMarkWrapper.reviewMark)) {
                continue;
            }

            if (reviewMark.from <= _positionInBlock && _positionInBlock <= reviewMark.end()) {
                return index(d->reviewMarks.indexOf(reviewMarkWrapper), 0);
            }
        }
    }
    return {};
}

void CommentsModel::setComment(const QModelIndex& _index, const QString& _comment)
{
    auto reviewMarkWrapper = d->reviewMarks.at(_index.row());
    for (auto textItem : std::as_const(reviewMarkWrapper.items)) {
        auto updatedReviewMarks = textItem->reviewMarks();
        for (auto& reviewMark : updatedReviewMarks) {
            if (reviewMark.isPartiallyEqual(reviewMarkWrapper.reviewMark)) {
                reviewMark.comments.first().text = _comment;
                reviewMark.comments.first().isEdited = true;
            }
        }
        textItem->setReviewMarks(updatedReviewMarks);
        d->model->updateItem(textItem);
    }
}

void CommentsModel::markAsDone(const QModelIndexList& _indexes)
{
    for (const auto& index : _indexes) {
        const auto reviewMarkWrapper = d->reviewMarks.at(index.row());
        for (auto textItem : reviewMarkWrapper.items) {
            auto updatedReviewMarks = textItem->reviewMarks();
            for (auto& reviewMark : updatedReviewMarks) {
                if (reviewMark.isPartiallyEqual(reviewMarkWrapper.reviewMark)) {
                    reviewMark.isDone = true;
                }
            }
            textItem->setReviewMarks(updatedReviewMarks);
            d->model->updateItem(textItem);
        }
    }
}

void CommentsModel::markAsUndone(const QModelIndexList& _indexes)
{
    for (const auto& index : _indexes) {
        const auto reviewMarkWrapper = d->reviewMarks.at(index.row());
        for (auto textItem : reviewMarkWrapper.items) {
            auto updatedReviewMarks = textItem->reviewMarks();
            for (auto& reviewMark : updatedReviewMarks) {
                if (reviewMark.isPartiallyEqual(reviewMarkWrapper.reviewMark)) {
                    reviewMark.isDone = false;
                }
            }
            textItem->setReviewMarks(updatedReviewMarks);
            d->model->updateItem(textItem);
        }
    }
}

void CommentsModel::addReply(const QModelIndex& _index, const QString& _comment)
{
    const auto replyDateTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    const auto reviewMarkWrapper = d->reviewMarks.at(_index.row());
    for (auto textItem : reviewMarkWrapper.items) {
        auto updatedReviewMarks = textItem->reviewMarks();
        for (auto& reviewMark : updatedReviewMarks) {
            if (reviewMark.isPartiallyEqual(reviewMarkWrapper.reviewMark)) {
                reviewMark.comments.append(
                    { DataStorageLayer::StorageFacade::settingsStorage()->accountName(),
                      DataStorageLayer::StorageFacade::settingsStorage()->accountEmail(),
                      replyDateTime, _comment });
            }
        }
        textItem->setReviewMarks(updatedReviewMarks);
        d->model->updateItem(textItem);
    }
}

void CommentsModel::editReply(const QModelIndex& _index, int _replyIndex, const QString& _comment)
{
    const auto replyDateTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    const auto reviewMarkWrapper = d->reviewMarks.at(_index.row());
    for (auto textItem : reviewMarkWrapper.items) {
        auto updatedReviewMarks = textItem->reviewMarks();
        for (auto& reviewMark : updatedReviewMarks) {
            if (reviewMark.isPartiallyEqual(reviewMarkWrapper.reviewMark)) {
                reviewMark.comments[_replyIndex].text = _comment;
                reviewMark.comments[_replyIndex].date = replyDateTime;
            }
        }
        textItem->setReviewMarks(updatedReviewMarks);
        d->model->updateItem(textItem);
    }
}

void CommentsModel::removeReply(const QModelIndex& _index, int _replyIndex)
{
    const auto reviewMarkWrapper = d->reviewMarks.at(_index.row());
    for (auto textItem : reviewMarkWrapper.items) {
        auto updatedReviewMarks = textItem->reviewMarks();
        for (auto& reviewMark : updatedReviewMarks) {
            if (reviewMark.isPartiallyEqual(reviewMarkWrapper.reviewMark)) {
                reviewMark.comments.removeAt(_replyIndex);
            }
        }
        textItem->setReviewMarks(updatedReviewMarks);
        d->model->updateItem(textItem);
    }
}

void CommentsModel::remove(const QModelIndexList& _indexes)
{
    for (const auto& index : reversed(_indexes)) {
        const auto reviewMarkWrapper = d->reviewMarks.at(index.row());
        for (auto textItem : reviewMarkWrapper.items) {
            auto updatedReviewMarks = textItem->reviewMarks();
            updatedReviewMarks.erase(std::remove_if(updatedReviewMarks.begin(),
                                                    updatedReviewMarks.end(),
                                                    [reviewMarkWrapper](const auto& _reviewMark) {
                                                        return _reviewMark.isPartiallyEqual(
                                                            reviewMarkWrapper.reviewMark);
                                                    }),
                                     updatedReviewMarks.end());
            textItem->setReviewMarks(updatedReviewMarks);
            d->model->updateItem(textItem);
        }
    }
}

int CommentsModel::rowCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)

    return d->reviewMarks.size();
}

QVariant CommentsModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    if (_index.row() >= d->reviewMarks.size()) {
        return {};
    }

    const auto reviewMarkWrapper = d->reviewMarks.at(_index.row());
    switch (_role) {
    case ReviewMarkAuthorNameRole: {
        return reviewMarkWrapper.reviewMark.comments.constFirst().author;
    }

    case ReviewMarkAuthorEmailRole: {
        return reviewMarkWrapper.reviewMark.comments.constFirst().authorEmail;
    }

    case ReviewMarkCreationDateRole: {
        return reviewMarkWrapper.reviewMark.comments.constFirst().date;
    }

    case ReviewMarkCommentRole: {
        return reviewMarkWrapper.reviewMark.comments.constFirst().text;
    }

    case ReviewMarkIsEditedRole: {
        return reviewMarkWrapper.reviewMark.comments.constFirst().isEdited;
    }

    case ReviewMarkColorRole: {
        if (reviewMarkWrapper.reviewMark.backgroundColor.isValid()) {
            return reviewMarkWrapper.reviewMark.backgroundColor;
        } else {
            return reviewMarkWrapper.reviewMark.textColor;
        }
    }

    case ReviewMarkIsDoneRole: {
        return reviewMarkWrapper.reviewMark.isDone;
    }

    case ReviewMarkRepliesRole: {
        if (reviewMarkWrapper.reviewMark.comments.isEmpty()) {
            return {};
        }
        return QVariant::fromValue(reviewMarkWrapper.reviewMark.comments);
    }

    default: {
        return {};
    }
    }
}

} // namespace BusinessLayer
