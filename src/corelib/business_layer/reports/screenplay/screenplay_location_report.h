#pragma once

#include <business_layer/reports/abstract_report.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Отчёт по локациям сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayLocationReport : public AbstractReport
{
public:
    ScreenplayLocationReport();
    ~ScreenplayLocationReport() override;

    /**
     * @brief Сформировать отчёт из модели
     */
    void build(QAbstractItemModel* _model) override;

    /**
     * @brief Сохранить отчёт в файл
     */
    void saveToXlsx(const QString& _fileName) const override;

    /**
     * @brief Задать параметры отчёта
     */
    void setParameters(int _sortBy);

    /**
     * @brief Получить информацию о локациях
     */
    QAbstractItemModel* locationModel() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
