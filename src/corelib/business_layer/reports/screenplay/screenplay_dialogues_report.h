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
    void saveToXlsx(const QString& _fileName) const override;

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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
