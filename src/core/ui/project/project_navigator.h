#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

class QAbstractItemModel;


namespace Ui {

/**
 * @brief Навигатор по структуре проекта
 */
class ProjectNavigator : public StackWidget
{
    Q_OBJECT

public:
    /**
     * @brief Типы кнопок навигатора
     */
    enum class ActionButton {
        AddDocument,
        EmptyRecycleBin,
    };

public:
    explicit ProjectNavigator(QWidget* _parent = nullptr);
    ~ProjectNavigator() override;

    /**
     * @brief Задать режим работы
     */
    void setReadOnly(bool _readOnly);

    /**
     * @brief Задать модель документов проекта
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Задать модель контекстного меню навигатора
     */
    void setContextMenuActions(const QVector<QAction*>& _actions);

    /**
     * @brief Сохранить состояние
     */
    QVariant saveState() const;

    /**
     * @brief Восстановить состояние
     */
    void restoreState(bool _isNewProject, const QVariant& _state);

    /**
     * @brief Выделить элемент с заданным индексом в дереве
     */
    void setCurrentIndex(const QModelIndex& _index);

    /**
     * @brief Получить индекс выделенного элемента в дереве
     */
    QModelIndex currentIndex() const;

    /**
     * @brief Отобразить навигатор проекта
     */
    void showProjectNavigator();

    /**
     * @brief Показан ли в данный момент навигатор проекта
     */
    bool isProjectNavigatorShown() const;

    /**
     * @brief Показать кнопку заданного типа в нижней части навигатора
     */
    void showButton(ActionButton _type);

    /**
     * @brief Задать доступность кнопки действия в нижней части навигатора
     */
    void setButtonEnabled(bool _enabled);

signals:
    /**
     * @brief Пользователь выбрал заданный элемент структуры
     */
    void itemSelected(const QModelIndex& _index);

    /**
     * @brief Пользователь сделал двойной клик на элементе структуры
     */
    void itemDoubleClicked(const QModelIndex& _index);

    /**
     * @brief Пользователь хочет открыть навигацию по элементу
     */
    void itemNavigationRequested(const QModelIndex& _index);

    /**
     * @brief Пользователь хочет открыть контекстное меню и поэтому нужно обновить контекстное меню
     */
    void contextMenuUpdateRequested(const QModelIndex& _index);

    /**
     * @brief Пользователь нажал кнопку добавления документа
     */
    void addDocumentClicked();

    /**
     * @brief Нажата кнопка очистки корзины
     */
    void emptyRecycleBinClicked();

protected:
    /**
     * @brief Отлавливаем события отображения тултипа для дерева документов
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

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
