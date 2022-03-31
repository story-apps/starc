#pragma once

#include <QSortFilterProxyModel>


namespace BusinessLayer {

/**
 * @brief Прокси модель для отображения структуры тритмента сценария в навигаторе
 */
class ScreenplayTreatmentStructureModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ScreenplayTreatmentStructureModel(QObject* _parent = nullptr);
    ~ScreenplayTreatmentStructureModel() override;

    /**
     * @brief Задать необходимость отображать биты
     */
    void showBeats(bool _show);

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
