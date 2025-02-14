#pragma once

#include <business_layer/reports/abstract_report.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Сводный отчёт по сериалу
 */
class ScreenplaySeriesDialoguesReport : public AbstractReport
{
public:
    ScreenplaySeriesDialoguesReport();
    ~ScreenplaySeriesDialoguesReport() override;

    /**
     * @brief Сформировать отчёт из модели
     */
    void build(QAbstractItemModel* _model) override;

    /**
     * @brief Задать параметры отчёта
     */
    void setParameters(const QVector<QString>& _characters);

    /**
     * @brief Получить информацию о репликах
     */
    QAbstractItemModel* dialoguesModel() const;

    /**
     * @brief Получить список персонажей из отчёта
     */
    QVector<QString> characters() const;

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
