#pragma once

#include <business_layer/reports/abstract_report.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Сводный отчёт по сериалу
 */
class ScreenplaySeriesSceneReport : public AbstractReport
{
public:
    ScreenplaySeriesSceneReport();
    ~ScreenplaySeriesSceneReport() override;

    /**
     * @brief Сформировать отчёт из модели
     */
    void build(QAbstractItemModel* _model) override;

    /**
     * @brief Задать параметры отчёта
     */
    void setParameters(bool _showCharacters, int _sortBy);

    /**
     * @brief Получить информацию о сценах
     */
    QAbstractItemModel* sceneModel() const;

protected:
    /**
     * @brief Сохранить отчёт в файл
     */
    void saveToPdf(const QString& _fileName) const override;
    void saveToXlsx(const QString& _fileName) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
