#pragma once

#include <business_layer/reports/abstract_report.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Отчёт по локациям сценария
 */
class CORE_LIBRARY_EXPORT AudioplayLocationReport : public AbstractReport
{
public:
    AudioplayLocationReport();
    ~AudioplayLocationReport() override;

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
    void setParameters(bool _extendedView, int _sortBy);

    /**
     * @brief Получить информацию о локациях
     */
    QAbstractItemModel* locationModel() const;

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
