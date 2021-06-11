#pragma once

#include <QSortFilterProxyModel>

#include <corelib_global.h>


namespace BusinessLayer {

class StructureModel;

/**
 * @brief Прокси модель, для фильтрации исходной модели структуры проекта
 */
class CORE_LIBRARY_EXPORT StructureProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit StructureProxyModel(StructureModel* _parent = nullptr);
    ~StructureProxyModel() override;

protected:
    bool filterAcceptsRow(int _sourceRow, const QModelIndex& _sourceParent) const override;
};

} // namespace BusinessLayer
