#pragma once

#include <business_layer/reports/abstract_report.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Отчёт по репликам персонажей сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayDialoguesReport : public AbstractReport
{
public:
    ScreenplayDialoguesReport();
    ~ScreenplayDialoguesReport() override;

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
     * @brief Получить информацию о репликах
     */
    QAbstractItemModel* dialoguesModel() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
