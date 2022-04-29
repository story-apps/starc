#pragma once

#include <ui/widgets/widget/widget.h>

class QAbstractItemModel;


namespace Ui {

/**
 * @brief Представление-редактор справочников сценария
 */
class DictionariesView : public Widget
{
    Q_OBJECT

public:
    explicit DictionariesView(QWidget* _parent = nullptr);
    ~DictionariesView() override;

    /**
     * @brief Задать список типов словарей
     */
    void setTypes(QAbstractItemModel* _types);

    /**
     * @brief Задать список элементов текущего словаря
     */
    void setDictionaryItems(QAbstractItemModel* _items);

    /**
     * @brief Текущий выбранный тип справочника
     */
    QModelIndex currentTypeIndex() const;

    /**
     * @brief Активировать редактирование последнего элемента
     * @note Используется при добавлении новых элементов, чтобы улучшить юзабилити
     */
    void editLastItem();

signals:
    /**
     * @brief Сменился тип текущего словаря
     */
    void typeChanged(const QModelIndex& _typeIndex);

    /**
     * @brief Пользователь хочет добавить элемент в словарь
     */
    void addItemRequested(const QModelIndex& _typeIndex);

    /**
     * @brief Пользователь хочет изменить элемент в словаре
     */
    void editItemRequested(const QModelIndex& _typeIndex, const QModelIndex& _itemIndex,
                           const QString& _item);

    /**
     * @brief Пользователь хочет удалить элемент
     */
    void removeItemRequested(const QModelIndex& _typeIndex, const QModelIndex& _itemIndex);

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

} // namespace Ui
