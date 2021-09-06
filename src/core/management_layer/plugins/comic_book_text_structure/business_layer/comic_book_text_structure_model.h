#pragma once

#include <QSortFilterProxyModel>


namespace BusinessLayer {

/**
 * @brief Прокси модель для отображения структуры комикса в навигаторе
 */
class ComicBookTextStructureModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ComicBookTextStructureModel(QObject* _parent = nullptr);
    ~ComicBookTextStructureModel() override;

    /**
     * @brief Переопределяем, чтобы отлавливать модель комикса и сохранять её для дальнейшей работы
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
