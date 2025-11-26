#include "screenplay_series_episodes_model_episode_item.h"

#include <QColor>


namespace BusinessLayer {

class ScreenplaySeriesEpisodesModelEpisodeItem::Implementation
{
public:
    bool isStoryLines = false;
    QString name;
};


// ****


ScreenplaySeriesEpisodesModelEpisodeItem::ScreenplaySeriesEpisodesModelEpisodeItem()
    : d(new Implementation)
{
}

ScreenplaySeriesEpisodesModelEpisodeItem::~ScreenplaySeriesEpisodesModelEpisodeItem() = default;

ScreenplaySeriesEpisodesModelItemType ScreenplaySeriesEpisodesModelEpisodeItem::type() const
{
    return ScreenplaySeriesEpisodesModelItemType::Episode;
}

bool ScreenplaySeriesEpisodesModelEpisodeItem::isStoryLinesContainer() const
{
    return d->isStoryLines;
}

void ScreenplaySeriesEpisodesModelEpisodeItem::setStoryLinesContainer(bool _container)
{
    if (d->isStoryLines == _container) {
        return;
    }

    d->isStoryLines = _container;
    setChanged(true);
}

QString ScreenplaySeriesEpisodesModelEpisodeItem::name() const
{
    return d->name;
}

void ScreenplaySeriesEpisodesModelEpisodeItem::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    setChanged(true);
}

QColor ScreenplaySeriesEpisodesModelEpisodeItem::color() const
{
    return {};
}

void ScreenplaySeriesEpisodesModelEpisodeItem::setColor(const QColor& _color)
{
    Q_UNUSED(_color)
}

bool ScreenplaySeriesEpisodesModelEpisodeItem::isFilterAccepted(const QString& _text,
                                                                bool _isCaseSensitive,
                                                                int _filterType) const
{
    Q_UNUSED(_text)
    Q_UNUSED(_isCaseSensitive)
    Q_UNUSED(_filterType)

    return true;
}

} // namespace BusinessLayer
