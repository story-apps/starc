#pragma once

#include <QAbstractListModel>

#include <corelib_global.h>


namespace BusinessLayer {

enum class TextParagraphType;
class TextModel;


/**
 * @brief Модель комментариев к тексту
 */
class CORE_LIBRARY_EXPORT CommentsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    /**
     * @brief Роли данных из модели
     */
    enum DataRole {
        ReviewMarkAuthorNameRole = Qt::UserRole + 1,
        ReviewMarkAuthorEmailRole,
        ReviewMarkCreationDateRole,
        ReviewMarkSourceTextRole,
        ReviewMarkCommentRole,
        ReviewMarkIsEditedRole,
        ReviewMarkColorRole,
        ReviewMarkIsRevisionRole,
        ReviewMarkIsAdditionRole,
        ReviewMarkIsRemovalRole,
        ReviewMarkIsDoneRole,
        ReviewMarkRepliesRole
    };

public:
    explicit CommentsModel(QObject* _parent = nullptr);
    ~CommentsModel() override;

    /**
     * @brief Искать комментарии только в блоках заданных в списке
     * @note Если список пустой, то комментарии будут искаться везде
     */
    void setParagraphTypesFiler(const QVector<TextParagraphType>& _types);

    /**
     * @brief Задать модель текста сценария
     */
    void setTextModel(BusinessLayer::TextModel* _model);

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
     * @brief Является ли элемент в заданном индексе добавлением, или удалением
     */
    bool isChange(const QModelIndex& _index) const;

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
     * @brief Применить изменение
     */
    void applyChanges(const QModelIndexList& _indexes);

    /**
     * @brief Отменить изменение
     */
    void cancelChanges(const QModelIndexList& _index);

    /**
     * @brief Добавить комментарий к редакторской заметке
     */
    void addReply(const QModelIndex& _index, const QString& _comment);

    /**
     * @brief Изменить комментарий к редакторской заметке
     */
    void editReply(const QModelIndex& _index, int _replyIndex, const QString& _comment);

    /**
     * @brief Удалить комментарий к редакторской заметке
     */
    void removeReply(const QModelIndex& _index, int _replyIndex);

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
