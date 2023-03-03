#include "screenplay_breakdown_structure_model_item.h"

#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>


namespace BusinessLayer {
ScreenplayBreakdownStructureModelItem::ScreenplayBreakdownStructureModelItem(
    const QString& _name, const QModelIndex& _screenplayItemIndex)
    : name(_name)
    , screenplayItemIndex(_screenplayItemIndex)
{
    if (screenplayItemIndex.isValid()) {
        duration = std::chrono::seconds{
            _screenplayItemIndex.data(ScreenplayTextModelSceneItem::SceneDurationRole).toInt()
        };
    }
}

ScreenplayBreakdownStructureModelItem* ScreenplayBreakdownStructureModelItem::parent() const
{
    return static_cast<ScreenplayBreakdownStructureModelItem*>(AbstractModelItem::parent());
}

ScreenplayBreakdownStructureModelItem* ScreenplayBreakdownStructureModelItem::childAt(
    int _index) const
{
    return static_cast<ScreenplayBreakdownStructureModelItem*>(AbstractModelItem::childAt(_index));
}

QVariant ScreenplayBreakdownStructureModelItem::data(int _role) const
{
    switch (_role) {
    case Qt::DisplayRole: {
        return name;
    }

    case Qt::DecorationRole: {
        return screenplayItemIndex.isValid() ? screenplayItemIndex.data(_role) : u8"\U000f02dc";
    }

    case DurationRole: {
        const int duration
            = std::chrono::duration_cast<std::chrono::seconds>(this->duration).count();
        return duration;
    }

    case HighlightedRole: {
        return isHighlighted;
    }

    case ScreenplayIndexRole: {
        return screenplayItemIndex;
    }

    default: {
        return {};
    }
    }
}

void ScreenplayBreakdownStructureModelItem::appendItem(ScreenplayBreakdownStructureModelItem* _item)
{
    AbstractModelItem::appendItem(_item);
    duration += _item->duration;

    if (parent() != nullptr) {
        parent()->updateDuration();
    }
}

bool ScreenplayBreakdownStructureModelItem::isScene() const
{
    return screenplayItemIndex.isValid();
}

void ScreenplayBreakdownStructureModelItem::updateDuration()
{
    duration = {};
    for (auto childIndex = 0; childIndex < childCount(); ++childIndex) {
        duration += childAt(childIndex)->duration;
    }

    if (parent() != nullptr) {
        parent()->updateDuration();
    }
}

void ScreenplayBreakdownStructureModelItem::setHighlighted(bool _highlighted)
{
    if (isHighlighted == _highlighted) {
        return;
    }

    isHighlighted = _highlighted;
    if (parent() != nullptr) {
        parent()->setHighlighted(isHighlighted);
    }
}

} // namespace BusinessLayer
