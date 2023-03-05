#pragma once

#include <QSortFilterProxyModel>

namespace BusinessLayer {

/**
 * @brief Прокси модель для сортировки моделей данных
 */
class ScreenplayBreakdownStructureModelProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    /**
     * @brief Порядок сортировки элементов
     */
    enum class SortOrder {
        Undefined,
        ByScriptOrder,
        Alphabetically,
        ByDuration,
    };

public:
    explicit ScreenplayBreakdownStructureModelProxy(QObject* _parent = nullptr);
    ~ScreenplayBreakdownStructureModelProxy() override;

    /**
     * @brief Отсортировать модель
     */
    void setSortOrder(SortOrder _sortOrder);

protected:
    bool lessThan(const QModelIndex& _sourceLeft, const QModelIndex& _sourceRight) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
