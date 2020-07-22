#include "screenplay_text_comments_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>

#include <QColor>
#include <QDateTime>


namespace BusinessLayer
{

class ScreenplayTextCommentsModel::Implementation
{
public:
    ScreenplayTextModel* model = nullptr;

    struct ReviewComment {
        QString author;
        QDateTime date;
        QString text;
    };
    struct ReviewMark {
        /**
         * @brief Список абзацев, в которых находится заметка
         */
        QVector<ScreenplayTextModelTextItem*> items;

        /**
         * @brief Заметка начинается в этой позиции в первом из абзацев, в которых она расположена
         */
        int fromInFirstItem = 0;

        /**
         * @brief Заметка заканчивается в этой позиции в последнем из абзацев, в которых она расположена
         */
        int toInLastItem = 0;

        QColor textColor;
        QColor backgroundColor;
        bool isDone = false;
        QVector<ReviewComment> comments;
    };
    QVector<ReviewMark> reviewMarks;
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
    }

    d->model = _model;

    if (d->model != nullptr) {
        connect(d->model, &ScreenplayTextModel::rowsInserted, this, [this] (const QModelIndex& _parent, int _first, int _last) {
            //
            // Для каждого из абзацев
            //

            //
            // Если вставился абзац без заметок, пропускаем его
            //

            //
            // Если в абзаце есть заметка
            //
            {
                //
                // Если заметка внутри абзаца, просто сохраняем её
                //

                //
                // Если заметка начинается в начале абзаца, проверяем нельзя ли её присовокупить к заметке в конце предыдущего абзаца
                //

                //
                // Если заметка заканчивается в конце абзаца, проверяем нельзя ли её присовокупить к заметке в начале следующего абзаца
                //
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
        connect(d->model, &ScreenplayTextModel::dataChanged, this, [this] (const QModelIndex& _topLeft, const QModelIndex& _bottomRight) {
            //
            // Если в абзаце не было заметки
            //
            {
                //
                // Если заметки и не появилось, игнорируем абзац
                //

                //
                // Если заметка появилась
                //
                {
                    //
                    // Если заметка внутри абзаца, просто сохраняем её
                    //

                    //
                    // Если заметка начинается в начале абзаца, проверяем нельзя ли её присовокупить к заметке в конце предыдущего абзаца
                    //

                    //
                    // Если заметка заканчивается в конце абзаца, проверяем нельзя ли её присовокупить к заметке в начале следующего абзаца
                    //
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

    const auto reviewMark = d->reviewMarks.at(_index.row());
    switch (_role) {
        case ReviewMarkAuthorEmail: {
            return reviewMark.comments.constFirst().author;
        }

        case ReviewMarkCreationDate: {
            return reviewMark.comments.constFirst().date;
        }

        case ReviewMarkComment: {
            return reviewMark.comments.constFirst().text;

        }

        case ReviewMarkIsDone: {
            return reviewMark.isDone;

        }
    }

    return {};
}

} // namespace BusinessLayer
