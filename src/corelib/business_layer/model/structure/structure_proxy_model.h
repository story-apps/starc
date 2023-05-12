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

    /**
     * @brief Необходимо ли показывать подэлементы наборов
     */
    void setSubitemsVisible(bool _visible);

    /**
     * @brief Задать видимость конкретных документов
     */
    void setProjectInfoVisible(bool _visible);
    void setRecycleBinVisible(bool _visible);

protected:
    /**
     * @brief Фильтровать в соответствии с настройками и с базовыми правилами
     */
    bool filterAcceptsRow(int _sourceRow, const QModelIndex& _sourceParent) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
