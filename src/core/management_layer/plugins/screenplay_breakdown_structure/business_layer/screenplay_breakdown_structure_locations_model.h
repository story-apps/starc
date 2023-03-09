#pragma once

#include <QAbstractItemModel>


namespace BusinessLayer {

enum class ScreenplayBreakdownSortOrder;
class ScreenplayTextModel;

/**
 * @brief Модель списка персонажей и сцен, где они участвуют
 */
class ScreenplayBreakdownStructureLocationsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ScreenplayBreakdownStructureLocationsModel(QObject* _parent = nullptr);
    ~ScreenplayBreakdownStructureLocationsModel() override;

    /**
     * @brief Задать модель текста сценария
     */
    void setSourceModel(BusinessLayer::ScreenplayTextModel* _model);

    /**
     * @brief Отсортировать модель в заданной последовательности
     */
    void sortBy(ScreenplayBreakdownSortOrder _sortOrder);

    /**
     * @brief Задать текущий элемент модели текста сценария
     */
    void setSourceModelCurrentIndex(const QModelIndex& _index);

    /**
     * @brief Реализация базовых вещей для древовидной модели
     */
    /** @{ */
    QModelIndex index(int _row, int _column, const QModelIndex& _parent = {}) const override;
    QModelIndex parent(const QModelIndex& _child) const override;
    int columnCount(const QModelIndex& _parent = {}) const override;
    int rowCount(const QModelIndex& _parent = {}) const override;
    Qt::ItemFlags flags(const QModelIndex& _index) const override;
    QVariant data(const QModelIndex& _index, int _role) const override;
    //! Реализация перетаскивания элементов
    bool canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column,
                         const QModelIndex& _parent = {}) const override;
    bool dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column,
                      const QModelIndex& _parent = {}) override;
    QMimeData* mimeData(const QModelIndexList& _indexes) const override;
    QStringList mimeTypes() const override;
    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
