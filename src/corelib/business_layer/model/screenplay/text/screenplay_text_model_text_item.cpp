#include "screenplay_text_model_text_item.h"

#include "screenplay_text_model.h"

#include <business_layer/chronometry/chronometer.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <utils/helpers/string_helper.h>
#include <utils/helpers/text_helper.h>

#include <QColor>
#include <QLocale>
#include <QVariant>
#include <QXmlStreamReader>


namespace BusinessLayer {

class ScreenplayTextModelTextItem::Implementation
{
public:
    /**
     * @brief Длительность сцены
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{ 0 };
};

ScreenplayTextModelTextItem::ScreenplayTextModelTextItem(const ScreenplayTextModel* _model)
    : TextModelTextItem(_model)
    , d(new Implementation)
{
}

ScreenplayTextModelTextItem::~ScreenplayTextModelTextItem() = default;

std::chrono::milliseconds ScreenplayTextModelTextItem::duration() const
{
    return d->duration;
}

void ScreenplayTextModelTextItem::updateDuration()
{
    //
    // Не учитываем хронометраж некоторых блококв
    //
    if (paragraphType() == TextParagraphType::BeatHeading) {
        return;
    }

    const auto screenplayModel = qobject_cast<const ScreenplayTextModel*>(model());
    Q_ASSERT(screenplayModel);
    const auto duration = Chronometer::duration(paragraphType(), text(),
                                                screenplayModel->informationModel()->templateId());
    if (d->duration == duration) {
        return;
    }

    d->duration = duration;

    //
    // Помещаем изменённым для пересчёта хронометража в родительском элементе
    //
    markChanged();
}

QVariant ScreenplayTextModelTextItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return paragraphType() == TextParagraphType::Shot ? u8"\U000F0332" : u8"\U000F09A8";
    }

    default: {
        return TextModelTextItem::data(_role);
    }
    }
}

void ScreenplayTextModelTextItem::handleChange()
{
    updateDuration();
}

} // namespace BusinessLayer
