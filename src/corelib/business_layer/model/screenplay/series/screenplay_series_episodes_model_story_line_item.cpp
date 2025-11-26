#include "screenplay_series_episodes_model_story_line_item.h"

#include <QColor>


namespace BusinessLayer {

class ScreenplaySeriesEpisodesModelStoryLineItem::Implementation
{
public:
    bool isTitle = false;
    QString name;
    QColor color;
};


// **


ScreenplaySeriesEpisodesModelStoryLineItem::ScreenplaySeriesEpisodesModelStoryLineItem()
    : ScreenplaySeriesEpisodesModelItem()
    , d(new Implementation)
{
}

ScreenplaySeriesEpisodesModelStoryLineItem::ScreenplaySeriesEpisodesModelStoryLineItem(
    const ScreenplaySeriesEpisodesModelStoryLineItem& _other)
    : ScreenplaySeriesEpisodesModelItem()
    , d(new Implementation(*_other.d))
{
}

ScreenplaySeriesEpisodesModelStoryLineItem::~ScreenplaySeriesEpisodesModelStoryLineItem() = default;

const ScreenplaySeriesEpisodesModelStoryLineItem& ScreenplaySeriesEpisodesModelStoryLineItem::
operator=(const ScreenplaySeriesEpisodesModelStoryLineItem& _other)
{
    d.reset(new Implementation(*_other.d));
    return *this;
}

ScreenplaySeriesEpisodesModelItemType ScreenplaySeriesEpisodesModelStoryLineItem::type() const
{
    return ScreenplaySeriesEpisodesModelItemType::StoryLine;
}

bool ScreenplaySeriesEpisodesModelStoryLineItem::isStoryLineTitle() const
{
    return d->isTitle;
}

void ScreenplaySeriesEpisodesModelStoryLineItem::setStoryLineTitle(bool _title)
{
    if (d->isTitle == _title) {
        return;
    }

    d->isTitle = _title;
    setChanged(true);
}

QString ScreenplaySeriesEpisodesModelStoryLineItem::name() const
{
    return d->name;
}

void ScreenplaySeriesEpisodesModelStoryLineItem::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    setChanged(true);
}

QColor ScreenplaySeriesEpisodesModelStoryLineItem::color() const
{
    return d->color;
}

void ScreenplaySeriesEpisodesModelStoryLineItem::setColor(const QColor& _color)
{
    if (d->color == _color) {
        return;
    }

    d->color = _color;
    setChanged(true);
}

bool ScreenplaySeriesEpisodesModelStoryLineItem::isFilterAccepted(const QString& _text,
                                                                  bool _isCaseSensitive,
                                                                  int _filterType) const
{
    Q_UNUSED(_filterType)

    if (d->isTitle) {
        return true;
    }

    return d->name.contains(_text, _isCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
}

bool operator==(const ScreenplaySeriesEpisodesModelStoryLineItem& _lhs,
                const ScreenplaySeriesEpisodesModelStoryLineItem& _rhs)
{
    return _lhs.name() == _rhs.name();
}

} // namespace BusinessLayer
