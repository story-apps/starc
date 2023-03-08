#pragma once

#include <interfaces/ui/i_document_view.h>
#include <ui/abstract_navigator.h>

class QAbstractItemModel;
typedef QList<QModelIndex> QModelIndexList;


namespace Ui {

class ScreenplayBreakdownStructureView : public AbstractNavigator, public IDocumentView
{
    Q_OBJECT

public:
    explicit ScreenplayBreakdownStructureView(QWidget* _parent = nullptr);
    ~ScreenplayBreakdownStructureView() override;

    /**
     * @brief Реализация интерфейса IDocumentView
     */
    /** @{ */
    QWidget* asQWidget() override;
    void setEditingMode(ManagementLayer::DocumentEditingMode _mode) override;
    void setCurrentModelIndex(const QModelIndex& _mappedIndex) override;
    /** @} */

    /**
     * @brief Настроить навигатор в соответствии с параметрами заданными в настройках
     */
    void reconfigure();

    /**
     * @brief Задать заголовок навигатора
     */
    void setTitle(const QString& _title) override;

    /**
     * @brief Задать модель сцен сценария
     */
    void setModels(QAbstractItemModel* _scenesModel, QAbstractItemModel* _charactersModel,
                   QAbstractItemModel* _locationsModel);

    /**
     * @brief Развернуть список всех персонажей
     */
    void expandCharacters();

    /**
     * @brief Свернуть список всех персонажей
     */
    void collapseAllCharacters();

    /**
     * @brief Развернуть список локаций
     * @note Если level развен -1, то разворачивается всё дерево, 0 - первый уровенть, 1 - второй
     */
    void expandLocations(int _level = -1);

    /**
     * @brief Свернуть дерево локаций
     */
    void collapseAllLocations();

    /**
     * @brief Список выделенных элементов
     */
    QModelIndexList selectedIndexes() const;

signals:
    /**
     * @brief Пользователь выбрал элемент в навигаторе с заданным индексом
     */
    void currentSceneModelIndexChanged(const QModelIndex& _index);
    void currentCharacterSceneModelIndexChanged(const QModelIndex& _index);
    void currentLocationSceneModelIndexChanged(const QModelIndex& _index);

    /**
     * @brief Пользователь хочет открыть контекстное меню
     */
    void charactersViewContextMenuRequested(const QPoint& _pos);
    void locationsViewContextMenuRequested(const QPoint& _pos);

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
