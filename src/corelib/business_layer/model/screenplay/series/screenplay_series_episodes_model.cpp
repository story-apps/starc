#include "screenplay_series_episodes_model.h"

#include "../screenplay_information_model.h"
#include "../text/screenplay_text_model.h"
#include "screenplay_series_information_model.h"


namespace BusinessLayer {

class ScreenplaySeriesEpisodesModel::Implementation
{
public:
    /**
     * @brief Обновить параметры эпизодов
     */
    void updateEpisodesSettings();


    /**
     * @brief Модель информации о проекте
     */
    ScreenplaySeriesInformationModel* informationModel = nullptr;

    /**
     * @brief Список серий
     */
    QVector<ScreenplayTextModel*> episodes;
};

void ScreenplaySeriesEpisodesModel::Implementation::updateEpisodesSettings()
{
    Q_ASSERT(informationModel);

    for (const auto& episode : std::as_const(episodes)) {
        auto episodeInformationModel = episode->informationModel();
        episodeInformationModel->setCanCommonSettingsBeOverridden(false);
        episodeInformationModel->setOverrideCommonSettings(
            informationModel->overrideCommonSettings());
        episodeInformationModel->setTemplateId(informationModel->templateId());
        episodeInformationModel->setShowSceneNumbers(informationModel->showSceneNumbers());
        episodeInformationModel->setShowSceneNumbersOnLeft(
            informationModel->showSceneNumbersOnLeft());
        episodeInformationModel->setShowSceneNumbersOnRight(
            informationModel->showSceneNumbersOnRight());
        episodeInformationModel->setShowDialoguesNumbers(informationModel->showDialoguesNumbers());
    }
}


// ****


ScreenplaySeriesEpisodesModel::ScreenplaySeriesEpisodesModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

ScreenplaySeriesEpisodesModel::~ScreenplaySeriesEpisodesModel() = default;

void ScreenplaySeriesEpisodesModel::setInformationModel(ScreenplaySeriesInformationModel* _model)
{
    if (d->informationModel == _model) {
        return;
    }

    if (d->informationModel) {
        d->informationModel->disconnect(this);
    }

    d->informationModel = _model;

    if (d->informationModel) {
        connect(d->informationModel,
                &ScreenplaySeriesInformationModel::overrideCommonSettingsChanged, this,
                [this] { d->updateEpisodesSettings(); });
        connect(d->informationModel, &ScreenplaySeriesInformationModel::templateIdChanged, this,
                [this] { d->updateEpisodesSettings(); });
        connect(d->informationModel, &ScreenplaySeriesInformationModel::showSceneNumbersChanged,
                this, [this] { d->updateEpisodesSettings(); });
        connect(d->informationModel,
                &ScreenplaySeriesInformationModel::showSceneNumbersOnLeftChanged, this,
                [this] { d->updateEpisodesSettings(); });
        connect(d->informationModel,
                &ScreenplaySeriesInformationModel::showSceneNumbersOnRightChanged, this,
                [this] { d->updateEpisodesSettings(); });
        connect(d->informationModel, &ScreenplaySeriesInformationModel::showDialoguesNumbersChanged,
                this, [this] { d->updateEpisodesSettings(); });
    }
}

ScreenplaySeriesInformationModel* ScreenplaySeriesEpisodesModel::informationModel() const
{
    return d->informationModel;
}

QVector<ScreenplayTextModel*> ScreenplaySeriesEpisodesModel::episodes() const
{
    return d->episodes;
}

void ScreenplaySeriesEpisodesModel::setEpisodes(const QVector<ScreenplayTextModel*>& _episodes)
{
    if (d->episodes == _episodes) {
        return;
    }

    //
    // Снимаем запрет на переопределение параметров для сценариев, которые больше не в сериале
    //
    for (const auto& episode : std::as_const(d->episodes)) {
        if (!_episodes.contains(episode)) {
            episode->informationModel()->setCanCommonSettingsBeOverridden(true);
        }
    }

    //
    // Для сценариев, которые были добавлены в сериал включаем запрет на переопределение и
    // синхронизируем значения параметров с параметрами сериала
    //
    d->episodes = _episodes;
    d->updateEpisodesSettings();

    emit episodesChanged(_episodes);
}

void ScreenplaySeriesEpisodesModel::initDocument()
{
}

void ScreenplaySeriesEpisodesModel::clearDocument()
{
}

QByteArray ScreenplaySeriesEpisodesModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer
