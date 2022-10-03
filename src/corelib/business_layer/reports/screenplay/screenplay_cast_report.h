#pragma once

#include <business_layer/reports/abstract_report.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Отчёт по персонажам сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayCastReport : public AbstractReport
{
public:
    ScreenplayCastReport();
    ~ScreenplayCastReport() override;

    /**
     * @brief Сформировать отчёт из модели
     */
    void build(QAbstractItemModel* _model) override;

    /**
     * @brief Получить информацию о персонажах
     */
    QAbstractItemModel* castModel() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
