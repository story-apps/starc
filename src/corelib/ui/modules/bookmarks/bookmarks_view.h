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
     * @brief Установить модель закладок
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Установить текущим элемент для заданного индекса модели сценария
     */
    void setCurrentIndex(const QModelIndex& _index);

    /**
     * @brief Показать виджет добавления/редактирования закладки
     */
    void showAddBookmarkView(const QModelIndex& _index);

signals:
    /**
     * @brief Пользователь хочет добавить закладку
     */
    void addBookmarkRequested(const QString& _text, const QColor& _color);

    /**
     * @brief Пользователь хочет изменить закладку
     */
    void changeBookmarkRequested(const QModelIndex& _index, const QString& _text,
                                 const QColor& _color);

    /**
     * @brief Пользователь выбрал закладку
     */
    void bookmarkSelected(const QModelIndex& _index);

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
