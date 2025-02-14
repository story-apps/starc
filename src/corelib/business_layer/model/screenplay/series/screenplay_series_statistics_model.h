#pragma once

#include "../../abstract_model.h"


namespace BusinessLayer {

class ScreenplaySeriesCastReport;
class ScreenplaySeriesDialoguesReport;
class ScreenplaySeriesLocationReport;
class ScreenplaySeriesSceneReport;
class ScreenplaySeriesSummaryReport;
//
class ScreenplaySeriesCharactersActivityPlot;
//
class ScreenplaySeriesEpisodesModel;

/**
 * @brief Модель статистики сериала
 */
class CORE_LIBRARY_EXPORT ScreenplaySeriesStatisticsModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ScreenplaySeriesStatisticsModel(QObject* _parent = nullptr);
    ~ScreenplaySeriesStatisticsModel() override;

    /**
     * @brief Название документа
     */
    QString documentName() const override;

    /**
     * @brief Модель серий сценария
     */
    ScreenplaySeriesEpisodesModel* episodesModel() const;
    void setEpisodesModel(ScreenplaySeriesEpisodesModel* _model);

    /**
     * @brief Перестроить отчёты
     */
    void updateReports();

    /**
     * @brief Суммарный отчёт по сценарию
     */
    const ScreenplaySeriesSummaryReport& summaryReport() const;

    /**
     * @brief Отчёт по сценам
     */
    const ScreenplaySeriesSceneReport& sceneReport() const;
    void setSceneReportParameters(bool _showCharacters, int _sortBy);

    /**
     * @brief Отчёт по локациям
     */
    const ScreenplaySeriesLocationReport& locationReport() const;
    void setLocationReportParameters(bool _extendedView, int _sortBy);

    /**
     * @brief Отчёт по персонажам
     */
    const ScreenplaySeriesCastReport& castReport() const;
    void setCastReportParameters(bool _showDetails, bool _showWords, int _sortBy);

    /**
     * @brief Отчёт по репликам
     */
    const ScreenplaySeriesDialoguesReport& dialoguesReport() const;
    void setDialoguesReportParameters(const QVector<QString>& _visibleCharacters);

    /**
     * @brief График активности персонажей
     */
    const ScreenplaySeriesCharactersActivityPlot& charactersActivityPlot() const;
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
