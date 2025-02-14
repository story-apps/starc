#pragma once

#include "../../abstract_model.h"


namespace BusinessLayer {

class ScreenplaySeriesInformationModel;
class ScreenplayTextModel;

/**
 * @brief Модель статистики сериала
 */
class CORE_LIBRARY_EXPORT ScreenplaySeriesEpisodesModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ScreenplaySeriesEpisodesModel(QObject* _parent = nullptr);
    ~ScreenplaySeriesEpisodesModel() override;

    /**
     * @brief Задать модель информации о сериале
     */
    void setInformationModel(ScreenplaySeriesInformationModel* _model);
    ScreenplaySeriesInformationModel* informationModel() const;

    /**
     * @brief Список серий сериала
     */
    QVector<ScreenplayTextModel*> episodes() const;
    void setEpisodes(const QVector<ScreenplayTextModel*>& _episodes);
    Q_SIGNAL void episodesChanged(const QVector<ScreenplayTextModel*>& _episodes);

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
