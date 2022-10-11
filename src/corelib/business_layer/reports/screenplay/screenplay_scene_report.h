#pragma once

#include <business_layer/reports/abstract_report.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Отчёт по сценам сценария
 */
class CORE_LIBRARY_EXPORT ScreenplaySceneReport : public AbstractReport
{
public:
    ScreenplaySceneReport();
    ~ScreenplaySceneReport() override;

    /**
     * @brief Сформировать отчёт из модели
     */
    void build(QAbstractItemModel* _model) override;

    /**
     * @brief Сохранить отчёт в файл
     */
    void saveToFile(const QString& _fileName) const override;

    /**
     * @brief Задать параметры отчёта
     */
    void setParameters(int _sortBy);

    /**
     * @brief Получить информацию о сценах
     */
    QAbstractItemModel* sceneModel() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
