#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

class ScreenplayCastReport;
class ScreenplayDialoguesReport;
class ScreenplayGenderReport;
class ScreenplayLocationReport;
class ScreenplaySceneReport;
class ScreenplaySummaryReport;
//
class ScreenplayCharactersActivityPlot;
class ScreenplayStructureAnalysisPlot;
//
class ScreenplayTextModel;

/**
 * @brief Модель статистики по сценарию
 */
class CORE_LIBRARY_EXPORT ScreenplayStatisticsModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ScreenplayStatisticsModel(QObject* _parent = nullptr);
    ~ScreenplayStatisticsModel() override;

    /**
     * @brief Название документа
     */
    QString documentName() const override;

    /**
     * @brief Модель текста сценария
     */
    ScreenplayTextModel* textModel() const;
    void setScreenplayTextModel(ScreenplayTextModel* _model);

    /**
     * @brief Перестроить отчёты
     */
    void updateReports();

    /**
     * @brief Суммарный отчёт по сценарию
     */
    const ScreenplaySummaryReport& summaryReport() const;

    /**
     * @brief Отчёт по сценам
     */
    const ScreenplaySceneReport& sceneReport() const;
    void setSceneReportParameters(int _sortBy);

    /**
     * @brief Отчёт по локациям
     */
    const ScreenplayLocationReport& locationReport() const;
    void setLocationReportParameters(int _sortBy);

    /**
     * @brief Отчёт по персонажам
     */
    const ScreenplayCastReport& castReport() const;
    void setCastReportParameters(int _sortBy);

    /**
     * @brief Отчёт по репликам
     */
    const ScreenplayDialoguesReport& dialoguesReport() const;
    void setDialoguesReportParameters(int _sortBy);

    /**
     * @brief Гнедерный анализ
     */
    const ScreenplayGenderReport& genderReport() const;

    /**
     * @brief График структурного анализа
     */
    const ScreenplayStructureAnalysisPlot& structureAnalysisPlot() const;
    void setStructureAnalysisPlotParameters(bool _sceneDuration, bool _actionDuration,
                                            bool _dialoguesDuration, bool _charactersCount,
                                            bool _dialoguesCount);

    /**
     * @brief График активности персонажей
     */
    const ScreenplayCharactersActivityPlot& charactersActivityPlot() const;
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
