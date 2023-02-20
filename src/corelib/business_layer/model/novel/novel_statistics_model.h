#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

class NovelSummaryReport;
class NovelTextModel;

/**
 * @brief Модель статистики по пьесе
 */
class CORE_LIBRARY_EXPORT NovelStatisticsModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit NovelStatisticsModel(QObject* _parent = nullptr);
    ~NovelStatisticsModel() override;

    /**
     * @brief Название документа
     */
    QString documentName() const override;

    /**
     * @brief Задать модель
     */
    void setNovelTextModel(NovelTextModel* _model);

    /**
     * @brief Перестроить отчёты
     */
    void updateReports();

    /**
     * @brief Суммарный отчёт по сценарию
     */
    const NovelSummaryReport& summaryReport() const;

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
