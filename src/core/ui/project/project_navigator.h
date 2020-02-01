#pragma once

#include <ui/widgets/widget/widget.h>

class QAbstractItemModel;


namespace Ui
{

/**
 * @brief Навигатор по структуре проекта
 */
class ProjectNavigator : public Widget
{
    Q_OBJECT

public:
    explicit ProjectNavigator(QWidget* _parent = nullptr);
    ~ProjectNavigator() override;

    /**
     * @brief Задать модель документов проекта
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Задать модель контекстного меню навигатора
     */
    void setContextMenuModel(QAbstractItemModel* _model);

    /**
     * @brief Сохранить состояние
     */
    QVariant saveState() const;

    /**
     * @brief Восстановить состояние
     */
    void restoreState(const QVariant& _state);

signals:
    /**
     * @brief Пользователь выбрал заданный элемент структуры
     */
    void itemSelected(const QModelIndex& _index);

    /**
     * @brief Пользователь хочет открыть контекстное меню
     */
    void contextMenuUpdateRequested(const QModelIndex& _index);

    /**
     * @brief Пользователь выбрал пункт контекстного меню
     */
    void contextMenuItemClicked(const QModelIndex& _itemIndex, const QModelIndex& _contextMenuIndex);

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

}
