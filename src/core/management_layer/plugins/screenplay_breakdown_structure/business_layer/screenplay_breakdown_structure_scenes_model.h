#pragma once

#include <QSortFilterProxyModel>


namespace BusinessLayer {

/**
 * @brief Прокси модель для отображения структуры сценария в навигаторе
 */
class ScreenplayBreakdownStructureScenesModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ScreenplayBreakdownStructureScenesModel(QObject* _parent = nullptr);
    ~ScreenplayBreakdownStructureScenesModel() override;

    /**
     * @brief Переопределяем, чтобы отлавливать модель сценария и сохранять её для дальнейшей работы
     */
    void setSourceModel(QAbstractItemModel* _sourceModel) override;

    /**
     * @brief Определяем собственную фильтрацию, которая будет пропускать текстовые элементы модели
     */
    bool filterAcceptsRow(int _sourceRow, const QModelIndex& _sourceParent) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
