#pragma once

#include <QSortFilterProxyModel>


namespace BusinessLayer {

/**
 * @brief Прокси модель для отображения структуры текстового документа в навигаторе
 */
class SimpleTextStructureModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit SimpleTextStructureModel(QObject* _parent = nullptr);
    ~SimpleTextStructureModel() override;

    /**
     * @brief Переопределяем, чтобы отлавливать модель текста и сохранять её для дальнейшей работы
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
