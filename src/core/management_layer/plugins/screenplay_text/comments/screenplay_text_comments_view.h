#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

class QAbstractItemModel;
template<typename T> class QList;
typedef QList<QModelIndex> QModelIndexList;


namespace Ui
{

/**
 * @brief Виджет комментариев сценария
 */
class ScreenplayTextCommentsView : public StackWidget
{
    Q_OBJECT

public:
    explicit ScreenplayTextCommentsView(QWidget* _parent = nullptr);
    ~ScreenplayTextCommentsView() override;

    /**
     * @brief Установить модель комментариев
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Показать виджет добавления/редактирования комментария
     */
    void showAddCommentView(const QColor& _withColor);

signals:
    /**
     * @brief Пользователь хочет добавить комментарий
     */
    void addCommentRequested(const QColor& _color, const QString& _text);

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
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

