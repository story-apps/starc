#include "screenplay_series_episodes_model_item.h"

#include <QVariant>


namespace BusinessLayer {

ScreenplaySeriesEpisodesModelItem::ScreenplaySeriesEpisodesModelItem()
    : AbstractModelItem()
{
}

ScreenplaySeriesEpisodesModelItemType ScreenplaySeriesEpisodesModelItem::type() const
{
    return ScreenplaySeriesEpisodesModelItemType::Root;
}

QVariant ScreenplaySeriesEpisodesModelItem::data(int _role) const
{
    Q_UNUSED(_role)
    return {};
}

ScreenplaySeriesEpisodesModelItem* ScreenplaySeriesEpisodesModelItem::parent() const
{
    return static_cast<ScreenplaySeriesEpisodesModelItem*>(AbstractModelItem::parent());
}

} // namespace BusinessLayer
