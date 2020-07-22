#include "screenplay_text_comments_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>

#include <QColor>
#include <QDateTime>


namespace BusinessLayer
{

namespace {

    class ModelIndexPath {
    public:
        ModelIndexPath(const QModelIndex& _index);
        ModelIndexPath(const QModelIndex& _index, int _weight);
        bool operator< (const ModelIndexPath& _other) const;

    private:
        const QModelIndex index;
        const int weight;
    };

    ModelIndexPath::ModelIndexPath(const QModelIndex& _index)
        : index(_index),
          weight(0)
    {
    }

    ModelIndexPath::ModelIndexPath(const QModelIndex& _index, int _weight)
        : index(_index),
          weight(_weight)
    {
    }

    bool ModelIndexPath::operator<(const ModelIndexPath& _other) const
    {
        auto buildPath = [] (const QModelIndex& _index, int _weight) {
            QList<int> path = { _weight };
            QModelIndex parent = _index;
            while (parent.isValid()) {
                path.prepend(parent.row());
                parent = parent.parent();
            }
            return path;
        };

        const auto selfPath = buildPath(index, weight);
        const auto otherPath = buildPath(_other.index, _other.weight);
        return selfPath < otherPath;
    }
}

class ScreenplayTextCommentsModel::Implementation
{
public:
    ScreenplayTextModel* model = nullptr;

    struct ReviewMarkWrapper {
        /**
         * @brief Список абзацев, в которых находится заметка
         */
        QVector<ScreenplayTextModelTextItem*> items;

        /**
         * @brief Сама заметка, на основе которой строится элемент
         */
        ScreenplayTextModelTextItem::ReviewMark itemsReviewMark;

        /**
         * @brief Заметка начинается в этой позиции в первом из абзацев, в которых она расположена
         */
        int fromInFirstItem = 0;

        /**
         * @brief Заметка заканчивается в этой позиции в последнем из абзацев, в которых она расположена
         */
        int toInLastItem = 0;
    };
    QMap<ModelIndexPath, ReviewMarkWrapper> reviewMarks;
};


ScreenplayTextCommentsModel::ScreenplayTextCommentsModel(QObject* _parent)
    : QAbstractListModel(_parent),
      d(new Implementation)
{

}

ScreenplayTextCommentsModel::~ScreenplayTextCommentsModel() = default;

void ScreenplayTextCommentsModel::setModel(ScreenplayTextModel* _model)
{
    if (d->model != nullptr) {
        d->model->disconnect(this);
        d->reviewMarks.clear();
    }

    d->model = _model;

    if (d->model != nullptr) {
        connect(d->model, &ScreenplayTextModel::rowsInserted, this, [this] (const QModelIndex& _parent, int _first, int _last) {
            //
            // Для каждого из абзацев
            //
            for (int itemRow = _first; itemRow <= _last; ++itemRow) {
                const auto itemIndex = d->model->index(itemRow, 0, _parent);
                const auto item = d->model->itemForIndex(itemIndex);
                if (item == nullptr
                    || item->type() != ScreenplayTextModelItemType::Text) {
                    continue;
                }

                //
                // Если вставился абзац без заметок, пропускаем его
                //
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
                if (textItem->reviewMarks().isEmpty()) {
                    continue;
                }

                //
                // Если в абзаце есть заметка
                //
                for (const auto& reviewMark : std::as_const(textItem->reviewMarks())) {
                    //
                    // Если заметка внутри абзаца, просто сохраняем её
                    //
                    if (0 < reviewMark.from && (reviewMark.from + reviewMark.length) < textItem->text().length()) {
                        Implementation::ReviewMarkWrapper reviewMarkWrapper;
                        reviewMarkWrapper.items.append(textItem);
                        reviewMarkWrapper.itemsReviewMark = reviewMark;
                        reviewMarkWrapper.fromInFirstItem = reviewMark.from;
                        reviewMarkWrapper.toInLastItem = reviewMark.from + reviewMark.length;
                        d->reviewMarks.insert({itemIndex, reviewMark.from}, reviewMarkWrapper);
                    }

                    //
                    // Если заметка начинается в начале абзаца, проверяем нельзя ли её присовокупить к заметке в конце предыдущего абзаца
                    //

                    //
                    // Если заметка заканчивается в конце абзаца, проверяем нельзя ли её присовокупить к заметке в начале следующего абзаца
                    //
                }
            }
        });
        connect(d->model, &ScreenplayTextModel::rowsRemoved, this, [this] (const QModelIndex& _parent, int _first, int _last) {
            //
            // Для каждого из абзацев
            //

            //
            // Если нет заметок связанных с удаляемым абзацем, пропускаем
            //

            //
            // Если есть заметки в удаляемом абзаце
            //
            {
                //
                // Если заметка была посередине абзаца, то просто удаляем её
                //

                //
                // Если абзац был внутри заметки, которая занимала несколько абзацев, то убираем её из списка
                //
                {
                    //
                    // Если в этой абзаце был конец заметки, то делаем концом заметки конец предыдущего абзаца
                    //

                    //
                    // Если в этом абзаце была середина заметки, то просто удаляем его из списков
                    //
                }
            }
        });
        connect(d->model, &ScreenplayTextModel::dataChanged, this, [this] (const QModelIndex& _topLeft) {
            const auto itemIndex = _topLeft;
            const auto item = d->model->itemForIndex(itemIndex);
            if (item == nullptr
                || item->type() != ScreenplayTextModelItemType::Text) {
                return;
            }

            //
            // Если в абзаце не было заметки
            //
            const auto itemIter = d->reviewMarks.lowerBound(itemIndex);
            if (itemIter == d->reviewMarks.end()) {
                //
                // Если заметки и не появилось, игнорируем абзац
                //
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
                if (textItem->reviewMarks().isEmpty()) {
                    return;
                }
                //
                // Если заметка появилась
                //
                else {
                    for (const auto& reviewMark : std::as_const(textItem->reviewMarks())) {
                        //
                        // Если заметка внутри абзаца, просто сохраняем её
                        //
                        if (0 < reviewMark.from && (reviewMark.from + reviewMark.length) < textItem->text().length()) {
                            Implementation::ReviewMarkWrapper reviewMarkWrapper;
                            reviewMarkWrapper.items.append(textItem);
                            reviewMarkWrapper.itemsReviewMark = reviewMark;
                            reviewMarkWrapper.fromInFirstItem = reviewMark.from;
                            reviewMarkWrapper.toInLastItem = reviewMark.from + reviewMark.length;
                            d->reviewMarks.insert({itemIndex, reviewMark.from}, reviewMarkWrapper);
                        }

                        //
                        // Если заметка начинается в начале абзаца, проверяем нельзя ли её присовокупить к заметке в конце предыдущего абзаца
                        //

                        //
                        // Если заметка заканчивается в конце абзаца, проверяем нельзя ли её присовокупить к заметке в начале следующего абзаца
                        //
                    }
                }
            }

            //
            // Если в абзаце была заметка
            //
            {
                //
                // Если теперь нет заметки
                //
                {
                    //
                    // Если заметка была посередине абзаца, то просто удаляем её
                    //

                    //
                    // Если абзац был внутри заметки, которая занимала несколько абзацев, то убираем её из списка
                    //
                    {
                        //
                        // Если в этой абзаце был конец заметки, то делаем концом заметки конец предыдущего абзаца
                        //

                        //
                        // Если в этом абзаце была середина заметки, то просто удаляем его из списков
                        //
                    }
                }

                //
                // Если таки есть, то обновляем все параметры заметки (начало, конец, цвет и т.п.)
                //
            }
        });
    }
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

    const auto reviewMark = d->reviewMarks.values().at(_index.row());
    switch (_role) {
        case ReviewMarkAuthorEmail: {
            return reviewMark.itemsReviewMark.comments.constFirst().author;
        }

        case ReviewMarkCreationDate: {
            return reviewMark.itemsReviewMark.comments.constFirst().date;
        }

        case ReviewMarkComment: {
            return reviewMark.itemsReviewMark.comments.constFirst().text;

        }

        case ReviewMarkColor: {
            if (reviewMark.itemsReviewMark.backgroundColor.isValid()) {
                return reviewMark.itemsReviewMark.backgroundColor;
            } else {
                return reviewMark.itemsReviewMark.textColor;
            }

        }

        case ReviewMarkIsDone: {
            return reviewMark.itemsReviewMark.isDone;

        }
    }

    return {};
}

} // namespace BusinessLayer
