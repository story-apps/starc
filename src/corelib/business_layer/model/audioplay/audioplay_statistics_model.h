#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

class AudioplayCastReport;
class AudioplayDialoguesReport;
class AudioplayGenderReport;
class AudioplayLocationReport;
class AudioplaySceneReport;
class AudioplaySummaryReport;
//
class AudioplayCharactersActivityPlot;
class AudioplayStructureAnalysisPlot;
//
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
     * @brief Название документа
     */
    QString documentName() const override;

    /**
     * @brief Задать модель текста сценария
     */
    AudioplayTextModel* textModel() const;
    void setAudioplayTextModel(AudioplayTextModel* _model);

    /**
     * @brief Перестроить отчёты
     */
    void updateReports();

    /**
     * @brief Суммарный отчёт по сценарию
     */
    const AudioplaySummaryReport& summaryReport() const;

    /**
     * @brief Отчёт по сценам
     */
    const AudioplaySceneReport& sceneReport() const;
    void setSceneReportParameters(int _sortBy);

    /**
     * @brief Отчёт по локациям
     */
    const AudioplayLocationReport& locationReport() const;
    void setLocationReportParameters(int _sortBy);

    /**
     * @brief Отчёт по персонажам
     */
    const AudioplayCastReport& castReport() const;
    void setCastReportParameters(int _sortBy);

    /**
     * @brief Отчёт по репликам
     */
    const AudioplayDialoguesReport& dialoguesReport() const;
    void setDialoguesReportParameters(const QVector<QString>& _visibleCharacters);

    /**
     * @brief Гнедерный анализ
     */
    const AudioplayGenderReport& genderReport() const;

    /**
     * @brief График структурного анализа
     */
    const AudioplayStructureAnalysisPlot& structureAnalysisPlot() const;
    void setStructureAnalysisPlotParameters(bool _sceneDuration, bool _actionDuration,
                                            bool _dialoguesDuration, bool _charactersCount,
                                            bool _dialoguesCount);

    /**
     * @brief График активности персонажей
     */
    const AudioplayCharactersActivityPlot& charactersActivityPlot() const;
    void setCharactersActivityPlotParameters(const QVector<QString>& _visibleCharacters);


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
