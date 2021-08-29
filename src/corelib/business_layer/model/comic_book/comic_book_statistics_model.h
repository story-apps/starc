#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

class ComicBookSummaryReport;
class ComicBookTextModel;

/**
 * @brief Модель статистики по комиксу
 */
class CORE_LIBRARY_EXPORT ComicBookStatisticsModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ComicBookStatisticsModel(QObject* _parent = nullptr);
    ~ComicBookStatisticsModel() override;

    /**
     * @brief Задать мМодель текста сценария
     */
    void setComicBookTextModel(ComicBookTextModel* _model);

    /**
     * @brief Перестроить отчёты
     */
    void updateReports();

    /**
     * @brief Суммарный отчёт по сценарию
     */
    const ComicBookSummaryReport& summaryReport() const;

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
