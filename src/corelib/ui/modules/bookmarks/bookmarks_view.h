#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

#include <QModelIndex>


namespace Ui {
/**
 * @brief Навигатор по закладкам текстового документа
 */
class CORE_LIBRARY_EXPORT BookmarksView : public StackWidget
{
    Q_OBJECT

public:
    explicit BookmarksView(QWidget* _parent = nullptr);
    ~BookmarksView() override;

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
    void showAddBookmarkView(const QColor& _withColor, const QString& _withText = {});

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
     * @brief Пользователь хочет комментарий к редакторской заметке
     */
    void addReviewMarkReplyRequested(const QModelIndex& _index, const QString& _text);

    /**
     * @brief Пользователь выбрал закладку
     */
    void bookmarkSelected(const QModelIndex& _index);

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
