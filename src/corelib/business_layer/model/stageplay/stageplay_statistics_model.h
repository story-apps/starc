#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

class StageplaySummaryReport;
class StageplayTextModel;

/**
 * @brief Модель статистики по пьесе
 */
class CORE_LIBRARY_EXPORT StageplayStatisticsModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit StageplayStatisticsModel(QObject* _parent = nullptr);
    ~StageplayStatisticsModel() override;

    /**
     * @brief Задать модель
     */
    void setStageplayTextModel(StageplayTextModel* _model);

    /**
     * @brief Перестроить отчёты
     */
    void updateReports();

    /**
     * @brief Суммарный отчёт по сценарию
     */
    const StageplaySummaryReport& summaryReport() const;

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
