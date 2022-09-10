#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

#include <QModelIndex>
#include <QtContainerFwd>


namespace Ui {

/**
 * @brief Виджет комментариев сценария
 */
class CORE_LIBRARY_EXPORT CommentsView : public StackWidget
{
    Q_OBJECT

public:
    explicit CommentsView(QWidget* _parent = nullptr);
    ~CommentsView() override;

    /**
     * @brief Задать возможность редактирования
     */
    bool isReadOnly() const;
    void setReadOnly(bool _readOnly);

    /**
     * @brief Установить модель комментариев
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Установить текущим элемент для заданного индекса модели сценария и позиции в блоке
     */
    void setCurrentIndex(const QModelIndex& _index);

    /**
     * @brief Показать виджет добавления/редактирования комментария
     */
    void showAddCommentView(const QColor& _withColor, const QString& _withText = {},
                            int _topMargin = 0);

    /**
     * @brief Отобразить обуждение комментария
     */
    void showCommentRepliesView(const QModelIndex& _commentIndex);

signals:
    /**
     * @brief Пользователь хочет добавить редакторскую заметку
     */
    void addReviewMarkRequested(const QColor& _color, const QString& _text);

    /**
     * @brief Пользователь хочет изменить редакторскую заметку
     */
    void changeReviewMarkRequested(const QModelIndex& _index, const QString& _text);

    /**
     * @brief Пользователь хочет добавить комментарий к редакторской заметке
     */
    void addReviewMarkReplyRequested(const QModelIndex& _index, const QString& _text);

    /**
     * @brief Пользователь хочет обновить текст комментария к редакторской заметке
     */
    void editReviewMarkReplyRequested(const QModelIndex& _index, int _replyIndex,
                                      const QString& _text);

    /**
     * @brief Пользователь хочет удалить комментарий к редакторской заметке
     */
    void removeReviewMarkReplyRequested(const QModelIndex& _index, int _replyIndex);

    /**
     * @brief Пользователь выбрал комментарий
     */
    void commentSelected(const QModelIndex& _index);

    /**
     * @brief Пользователь хочет пометить завершёнными выбранные заметки
     */
    void markAsDoneRequested(const QModelIndexList& _indexes);

    /**
     * @brief Пользователь хочет пометить незавершёнными выбранные заметки
     */
    void markAsUndoneRequested(const QModelIndexList& _indexes);

    /**
     * @brief Пользователь хочет удалить выбранные заметки
     */
    void removeRequested(const QModelIndexList& _indexes);

protected:
    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
