#pragma once

#include <business_layer/plots/abstract_plot.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief График структурного анализа сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayStructureAnalysisPlot : public AbstractPlot
{
public:
    ScreenplayStructureAnalysisPlot();
    ~ScreenplayStructureAnalysisPlot() override;

    /**
     * @brief Сформировать график из заданной модели
     */
    void build(QAbstractItemModel* _model) const override;

    /**
     * @brief Получить данные графика
     */
    Plot plot() const override;

    /**
     * @brief Задать параметры графика
     */
    void setParameters(bool _sceneDuration, bool _actionDuration, bool _dialoguesDuration,
                       bool _charactersCount, bool _dialoguesCount);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
