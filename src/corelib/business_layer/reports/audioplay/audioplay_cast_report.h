#pragma once

#include <business_layer/reports/abstract_report.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Отчёт по персонажам сценария
 */
class CORE_LIBRARY_EXPORT AudioplayCastReport : public AbstractReport
{
public:
    AudioplayCastReport();
    ~AudioplayCastReport() override;

    /**
     * @brief Валиден ли отчёт
     */
    bool isValid() const override;

    /**
     * @brief Сформировать отчёт из модели
     */
    void build(QAbstractItemModel* _model) override;

    /**
     * @brief Задать параметры отчёта
     */
    void setParameters(int _sortBy);

    /**
     * @brief Получить информацию о персонажах
     */
    QAbstractItemModel* castModel() const;

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
