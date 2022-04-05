#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

class AudioplaySummaryReport;
class AudioplayTextModel;

/**
 * @brief Модель статистики по аудиопостановке
 */
class CORE_LIBRARY_EXPORT AudioplayStatisticsModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit AudioplayStatisticsModel(QObject* _parent = nullptr);
    ~AudioplayStatisticsModel() override;

    /**
     * @brief Задать модель текста сценария
     */
    void setAudioplayTextModel(AudioplayTextModel* _model);

    /**
     * @brief Перестроить отчёты
     */
    void updateReports();

    /**
     * @brief Суммарный отчёт по сценарию
     */
    const AudioplaySummaryReport& summaryReport() const;

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
