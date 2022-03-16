#pragma once

#include <QAbstractListModel>

#include <corelib_global.h>


namespace BusinessLayer {

class TextModel;


/**
 * @brief Модель комментариев к тексту сценария
 */
class CORE_LIBRARY_EXPORT CommentsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    /**
     * @brief Роли данных из модели
     */
    enum DataRole {
        ReviewMarkAuthorEmailRole = Qt::UserRole + 1,
        ReviewMarkCreationDateRole,
        ReviewMarkCommentRole,
        ReviewMarkIsEditedRole,
        ReviewMarkColorRole,
        ReviewMarkIsDoneRole,
        ReviewMarkRepliesRole
    };

public:
    explicit CommentsModel(QObject* _parent = nullptr);
    ~CommentsModel() override;

    /**
     * @brief Задать модель текста сценария
     */
    void setModel(BusinessLayer::TextModel* _model);

    /**
     * @brief Получить индекс элемента из модели сценария, в котором находится заданный комментарий,
     *        а так же его смещение внутри блока
     */
    struct PositionHint {
        QModelIndex index;
        int blockPosition;
    };
    PositionHint mapToModel(const QModelIndex& _index);

    /**
     * @brief Получить индекс заметки из индекса элемента модели сценария и позиции в абзаце
     */
    QModelIndex mapFromModel(const QModelIndex& _index, int _positionInBlock);

    /**
     * @brief Задать текст комментария с заданным индексом
     */
    void setComment(const QModelIndex& _index, const QString& _comment);

    /**
     * @brief Пометить заданные элементы выполнеными
     */
    void markAsDone(const QModelIndexList& _indexes);

    /**
     * @brief Пометить заданные элементы невыполнеными
     */
    void markAsUndone(const QModelIndexList& _indexes);

    /**
     * @brief Добавить комментарий к редакторской заметке
     */
    void addReply(const QModelIndex& _index, const QString& _comment);

    /**
     * @brief Удалить выбранные элементы
     */
    void remove(const QModelIndexList& _indexes);

    /**
     * @brief Реализация модели списка
     */
    int rowCount(const QModelIndex& _parent = {}) const override;
    QVariant data(const QModelIndex& _index, int _role) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
