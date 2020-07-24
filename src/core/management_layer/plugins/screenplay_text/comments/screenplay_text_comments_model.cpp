#include "screenplay_text_comments_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>

#include <utils/shugar.h>

#include <QColor>
#include <QDateTime>


namespace BusinessLayer
{

namespace {

    /**
     * @brief Определить одинаково ли форматирование и комментарии редакторских заметок
     */
    bool isReviewMarksPartiallyEqual(const ScreenplayTextModelTextItem::ReviewMark& _lhs,
                                     const ScreenplayTextModelTextItem::ReviewMark& _rhs) {
        return _lhs.textColor == _rhs.textColor
                && _lhs.backgroundColor == _rhs.backgroundColor
                && _lhs.isDone == _rhs.isDone
                && _lhs.comments.first() == _rhs.comments.first();
    }

} // namespace


class ScreenplayTextCommentsModel::Implementation
{
public:
    explicit Implementation(ScreenplayTextCommentsModel* _q);

    /**
     * @brief Получить предыдущий индекс по карте textItemIndexToReviewIndex
     */
    void saveReviewMark(ScreenplayTextModelTextItem* _textItem, const ScreenplayTextModelTextItem::ReviewMark& _reviewMark);

    void processSourceModelRowsInserted(const QModelIndex& _parent, int _firstRow, int _lastRow);
    void processSourceModelRowsRemoved(const QModelIndex& _parent, int _firstRow, int _lastRow);
    void processSourceModelDataChanged(const QModelIndex& _index);


    ScreenplayTextCommentsModel* q = nullptr;

    /**
     * @brief Модель сценария, на основе которой строится модель заметок
     */
    ScreenplayTextModel* model = nullptr;
    /**
     * @brief Список всех текстовых элементов модели, для того, чтобы понимать их последовательность
     */
    QVector<ScreenplayTextModelTextItem*> modelTextItems;

    /**
     * @brief Вспомогательная структура для хранения заметки и группы элементов, к которой она относится
     */
    struct ReviewMarkWrapper {
        /**
         * @brief Сама заметка, на основе которой строится элемент
         */
        ScreenplayTextModelTextItem::ReviewMark reviewMark;

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
        QVector<ScreenplayTextModelTextItem*> items;


        bool operator== (const ReviewMarkWrapper& _other) const;
    };
    /**
     * @brief Список заметок сценария
     */
    QVector<ReviewMarkWrapper> reviewMarks;
};

//        //
//        // Вспомогательные функции
//        //
//        auto saveReviewMark = [this] (const QModelIndex& _itemIndex,
//                                      ScreenplayTextModelTextItem* _textItem,
//                                      const ScreenplayTextModelTextItem::ReviewMark& _reviewMark)
//        {
//            Implementation::ReviewMarkWrapper reviewMarkWrapper;
//            reviewMarkWrapper.items.append(_textItem);
//            reviewMarkWrapper.itemsReviewMark = _reviewMark;
//            reviewMarkWrapper.fromInFirstItem = _reviewMark.from;
//            reviewMarkWrapper.toInLastItem = _reviewMark.from + _reviewMark.length;

//            const ModelIndexPath indexPath = {_itemIndex, _reviewMark.from};
//            d->reviewMarks.insert(indexPath, reviewMarkWrapper);
//            const auto reviewMarkRow = std::distance(d->reviewMarks.begin(), d->reviewMarks.find(indexPath));
//            beginInsertRows({}, reviewMarkRow, reviewMarkRow);
//            endInsertRows();
//        };


//        connect(d->model, &ScreenplayTextModel::rowsInserted, this, [this, saveReviewMark] (const QModelIndex& _parent, int _first, int _last) {
//            //
//            // Для каждого из абзацев
//            //
//            for (int itemRow = _first; itemRow <= _last; ++itemRow) {
//                const auto itemIndex = d->model->index(itemRow, 0, _parent);
//                const auto item = d->model->itemForIndex(itemIndex);
//                if (item == nullptr
//                    || item->type() != ScreenplayTextModelItemType::Text) {
//                    continue;
//                }

//                //
//                // Если вставился абзац без заметок, пропускаем его
//                //
//                auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
//                if (textItem->reviewMarks().isEmpty()) {
//                    continue;
//                }

//                //
//                // Если в абзаце есть заметка
//                //
//                for (const auto& reviewMark : std::as_const(textItem->reviewMarks())) {
//                    //
//                    // Если заметка внутри абзаца, просто сохраняем её
//                    //
//                    if (0 < reviewMark.from && (reviewMark.from + reviewMark.length) < textItem->text().length()) {
//                        saveReviewMark(itemIndex, textItem, reviewMark);
//                    }

//                    //
//                    // Если заметка начинается в начале абзаца, проверяем нельзя ли её присовокупить к заметке в конце предыдущего абзаца
//                    //

//                    //
//                    // Если заметка заканчивается в конце абзаца, проверяем нельзя ли её присовокупить к заметке в начале следующего абзаца
//                    //
//                }
//            }
//        });
//        connect(d->model, &ScreenplayTextModel::rowsRemoved, this, [this] (const QModelIndex& _parent, int _first, int _last) {
//            //
//            // Для каждого из абзацев
//            //
//            for (int itemRow = _first; itemRow <= _last; ++itemRow) {

//                //
//                // Если нет заметок связанных с удаляемым абзацем, пропускаем
//                //

//                //
//                // Если есть заметки в удаляемом абзаце
//                //
//                {
//                    //
//                    // Если заметка была посередине абзаца, то просто удаляем её
//                    //

//                    //
//                    // Если абзац был внутри заметки, которая занимала несколько абзацев, то убираем её из списка
//                    //
//                    {
//                        //
//                        // Если в этой абзаце был конец заметки, то делаем концом заметки конец предыдущего абзаца
//                        //

//                        //
//                        // Если в этом абзаце была середина заметки, то просто удаляем его из списков
//                        //
//                    }
//                }
//            }
//        });

ScreenplayTextCommentsModel::Implementation::Implementation(ScreenplayTextCommentsModel* _q)
    : q(_q)
{
}

void ScreenplayTextCommentsModel::Implementation::saveReviewMark(
    ScreenplayTextModelTextItem* _textItem, const ScreenplayTextModelTextItem::ReviewMark& _reviewMark)
{
    bool reviewMarkAdded = false;

    do {
        if (reviewMarks.empty()) {
            break;
        }

        //
        // Если заметка начинается в начале абзаца, проверяем нельзя ли её присовокупить к заметке в конце предыдущего абзаца
        //
        if (_reviewMark.from == 0
                && _reviewMark.length != _textItem->text().length()) {
            if (_textItem == modelTextItems.constFirst()) {
                break;
            }

            //
            // Смотрим заметки которые есть в предыдущем блоке
            //
            auto previousTextItem = modelTextItems.at(modelTextItems.indexOf(_textItem) - 1);
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
            const auto previousTextItemLastReviewMarkWrapper = previosTextItemReviewMarkWrappers.constLast();
            //
            // ... присовокупляем, если она имеет аналогичное форматирование и заканчивается в конце абзаца
            //
            if (isReviewMarksPartiallyEqual(previousTextItemLastReviewMarkWrapper.reviewMark, _reviewMark)
                    && previousTextItemLastReviewMarkWrapper.toInLastItem == previousTextItem->text().length()) {
                auto& reviewMarkWrapper = reviewMarks[reviewMarks.indexOf(previousTextItemLastReviewMarkWrapper)];
                reviewMarkWrapper.items.append(_textItem);
                reviewMarkWrapper.toInLastItem = _reviewMark.length;
                reviewMarkAdded = true;
            }
        }
    } once;

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
        const auto reviewMarkWrapperTextItemIndex = modelTextItems.indexOf(insertBeforeReviewMarkWrapper.items.constLast());
        //
        // Если элемент идёт после вставляемого
        //
        if (textItemIndex < reviewMarkWrapperTextItemIndex
            //
            // ... или если в том же элементе, тогда сортируем по расположению самой заметки
            //
            || (textItemIndex == reviewMarkWrapperTextItemIndex
                && reviewMarkWrapper.fromInFirstItem < insertBeforeReviewMarkWrapper.fromInFirstItem)) {
            break;
        }
    }
    q->beginInsertRows({}, insertIndex, insertIndex);
    reviewMarks.insert(insertIndex, reviewMarkWrapper);
    q->endInsertRows();
}

void ScreenplayTextCommentsModel::Implementation::processSourceModelRowsInserted(const QModelIndex& _parent, int _firstRow, int _lastRow)
{

}

void ScreenplayTextCommentsModel::Implementation::processSourceModelRowsRemoved(const QModelIndex& _parent, int _firstRow, int _lastRow)
{

}

void ScreenplayTextCommentsModel::Implementation::processSourceModelDataChanged(const QModelIndex& _index)
{
    if (!_index.isValid()) {
        return;
    }

    const auto item = model->itemForIndex(_index);
    if (item == nullptr
        || item->type() != ScreenplayTextModelItemType::Text) {
        return;
    }

    auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
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
//            bool reviewMarkAdded = false;
//            do {
//                if (reviewMarks.empty()) {
//                    break;
//                }

//                //
//                // Если заметка начинается в начале абзаца, проверяем нельзя ли её присовокупить к заметке в конце предыдущего абзаца
//                //
//                if (reviewMark.from == 0
//                    && reviewMark.length != textItem->text().length()) {
//                    if (textItem == modelTextItems.constFirst()) {
//                        break;
//                    }

//                    //
//                    // Смотрим заметки которые есть в предыдущем блоке
//                    //
//                    auto previousTextItem = modelTextItems.at(modelTextItems.indexOf(textItem) - 1);
//                    QVector<ReviewMarkWrapper> previosTextItemReviewMarkWrappers;
//                    for (const auto& reviewMarkWrapper : std::as_const(reviewMarks)) {
//                        if (reviewMarkWrapper.items.contains(previousTextItem)) {
//                            previosTextItemReviewMarkWrappers.append(reviewMarkWrapper);
//                            continue;
//                        }

//                        if (!previosTextItemReviewMarkWrappers.isEmpty()) {
//                            break;
//                        }
//                    }
//                    if (previosTextItemReviewMarkWrappers.isEmpty()) {
//                        break;
//                    }

//                    //
//                    // Берём последнюю из заметок
//                    //
//                    const auto previousTextItemLastReviewMarkWrapper = previosTextItemReviewMarkWrappers.constLast();
//                    //
//                    // ... присовокупляем, если она имеет аналогичное форматирование и заканчивается в конце абзаца
//                    //
//                    if (isReviewMarksPartiallyEqual(previousTextItemLastReviewMarkWrapper.reviewMark, reviewMark)
//                        && previousTextItemLastReviewMarkWrapper.toInLastItem == previousTextItem->text().length()) {
//                        auto& reviewMarkWrapper = reviewMarks[reviewMarks.indexOf(previousTextItemLastReviewMarkWrapper)];
//                        reviewMarkWrapper.items.append(textItem);
//                        reviewMarkWrapper.toInLastItem = reviewMark.length;
//                        reviewMarkAdded = true;
//                    }
//                }
//            } once;

//            if (reviewMarkAdded) {
//                continue;
//            }

//            //
//            // Если заметка внутри абзаца, просто сохраняем её
//            //
//            Implementation::ReviewMarkWrapper reviewMarkWrapper;
//            reviewMarkWrapper.items.append(textItem);
//            reviewMarkWrapper.reviewMark = reviewMark;
//            reviewMarkWrapper.fromInFirstItem = reviewMark.from;
//            reviewMarkWrapper.toInLastItem = reviewMark.end();

//            int insertIndex = 0;
//            const auto textItemIndex = modelTextItems.indexOf(textItem);
//            for (; insertIndex < reviewMarks.size(); ++insertIndex) {
//                const auto reviewMarkWrapperTextItemIndex = modelTextItems.indexOf(reviewMarks.at(insertIndex).items.constLast());
//                if (textItemIndex < reviewMarkWrapperTextItemIndex) {
//                    break;
//                }
//            }
//            q->beginInsertRows({}, insertIndex, insertIndex);
//            reviewMarks.insert(insertIndex, reviewMarkWrapper);
//            q->endInsertRows();
        }
    }
    //
    // Если в абзаце были заметки
    //
    else {
        int newReviewMarkIndex = 0;
        int oldReviewMarkIndex = 0;
        QVector<ReviewMarkWrapper> reviewMarkWrappersToAdd;

        for (; newReviewMarkIndex < textItem->reviewMarks().size(); ++newReviewMarkIndex) {
            const auto newReviewMark = textItem->reviewMarks().at(newReviewMarkIndex);
            const auto oldReviewMarkWrapper = oldReviewMarkWrappers.value(oldReviewMarkIndex);
            //
            // Обновим заметку, если она осталась на месте
            //
            if (isReviewMarksPartiallyEqual(newReviewMark, oldReviewMarkWrapper.reviewMark)) {
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
                q->dataChanged(changedItemModelIndex, changedItemModelIndex);

                ++oldReviewMarkIndex;
            }
            //
            // В противном случае добавляем новую заметку
            //
            else {
                saveReviewMark(textItem, newReviewMark);
            }
        }

        //
        // Удаляем старые заметки, которых нет в обновлённом блоке
        //
        for (; oldReviewMarkIndex < oldReviewMarkWrappers.size(); ++oldReviewMarkIndex) {
            const auto oldReviewMarkWrapper = oldReviewMarkWrappers.value(oldReviewMarkIndex);
            const auto oldReviewMarkWrapperIndex = reviewMarks.indexOf(oldReviewMarkWrapper);
            q->beginRemoveRows({}, oldReviewMarkWrapperIndex, oldReviewMarkWrapperIndex);
            reviewMarks.remove(oldReviewMarkWrapperIndex);
            q->endRemoveRows();
        }
    }
}

bool ScreenplayTextCommentsModel::Implementation::ReviewMarkWrapper::operator==(
    const ScreenplayTextCommentsModel::Implementation::ReviewMarkWrapper& _other) const
{
    return reviewMark == _other.reviewMark
            && fromInFirstItem == _other.fromInFirstItem
            && toInLastItem == _other.toInLastItem
            && items == _other.items;
}


// ****


ScreenplayTextCommentsModel::ScreenplayTextCommentsModel(QObject* _parent)
    : QAbstractListModel(_parent),
      d(new Implementation(this))
{

}

ScreenplayTextCommentsModel::~ScreenplayTextCommentsModel() = default;

void ScreenplayTextCommentsModel::setModel(ScreenplayTextModel* _model)
{
    beginResetModel();

    if (d->model != nullptr) {
        d->model->disconnect(this);
        d->reviewMarks.clear();
    }

    d->model = _model;

    if (d->model != nullptr) {
        std::function<void(const QModelIndex&)> readReviewMarksFromModel;
        readReviewMarksFromModel = [this, &readReviewMarksFromModel] (const QModelIndex& _parent) {
            using namespace BusinessLayer;
            for (int itemRow = 0; itemRow < d->model->rowCount(_parent); ++itemRow) {
                const auto itemIndex = d->model->index(itemRow, 0, _parent);
                const auto item = d->model->itemForIndex(itemIndex);
                switch (item->type()) {
                    case ScreenplayTextModelItemType::Text: {
                        //
                        // Сохраним текстовый элемент в плоском списке
                        //
                        const auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
                        d->modelTextItems.append(textItem);

                        //
                        // Если вставился абзац без заметок, пропускаем его
                        //
                        if (textItem->reviewMarks().isEmpty()) {
                            break;
                        }

                        //
                        // Если в абзаце есть заметка
                        //
                        for (const auto& reviewMark : std::as_const(textItem->reviewMarks())) {
                            //
                            // Если заметка начинается в начале абзаца, проверяем нельзя ли её присовокупить к заметке в конце предыдущего абзаца
                            //
                            if (reviewMark.from == 0
                                && !d->reviewMarks.isEmpty()) {
                                auto& lastReviewMarkWrapper = d->reviewMarks.last();
                                //
                                // В предыдущем блоке есть заметка с аналогичным форматированием и заканчивается она в конце абзаца
                                //
                                if (lastReviewMarkWrapper.items.last() == d->modelTextItems.at(d->modelTextItems.size() - 2)
                                    && isReviewMarksPartiallyEqual(lastReviewMarkWrapper.reviewMark, reviewMark)
                                    && lastReviewMarkWrapper.toInLastItem == lastReviewMarkWrapper.items.constLast()->text().length()) {
                                    lastReviewMarkWrapper.items.append(textItem);
                                    lastReviewMarkWrapper.toInLastItem = reviewMark.length;
                                    continue;
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

                    default: break;
                }

                //
                // Считываем информацию о детях
                //
                readReviewMarksFromModel(itemIndex);
            }
        };
        readReviewMarksFromModel({});

        connect(d->model, &ScreenplayTextModel::rowsInserted, this, [this] (const QModelIndex& _parent, int _first, int _last) {
            d->processSourceModelRowsInserted(_parent, _first, _last);
        });
        connect(d->model, &ScreenplayTextModel::rowsRemoved, this, [this] (const QModelIndex& _parent, int _first, int _last) {
            d->processSourceModelRowsRemoved(_parent, _first, _last);
        });
        connect(d->model, &ScreenplayTextModel::dataChanged, this, [this] (const QModelIndex& _topLeft, const QModelIndex& _bottomRight) {
            Q_ASSERT(_topLeft == _bottomRight);
            d->processSourceModelDataChanged(_topLeft);
        });
    }

    endResetModel();
}

int ScreenplayTextCommentsModel::rowCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)

    return d->reviewMarks.size();
}

QVariant ScreenplayTextCommentsModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    if (_index.row() >= d->reviewMarks.size()) {
        return {};
    }

    const auto reviewMarkWrapper = d->reviewMarks.at(_index.row());
    switch (_role) {
        case ReviewMarkAuthorEmail: {
            return reviewMarkWrapper.reviewMark.comments.constFirst().author;
        }

        case ReviewMarkCreationDate: {
            return reviewMarkWrapper.reviewMark.comments.constFirst().date;
        }

        case ReviewMarkComment: {
            return reviewMarkWrapper.reviewMark.comments.constFirst().text;

        }

        case ReviewMarkColor: {
            if (reviewMarkWrapper.reviewMark.backgroundColor.isValid()) {
                return reviewMarkWrapper.reviewMark.backgroundColor;
            } else {
                return reviewMarkWrapper.reviewMark.textColor;
            }

        }

        case ReviewMarkIsDone: {
            return reviewMarkWrapper.reviewMark.isDone;

        }
    }

    return {};
}

} // namespace BusinessLayer
