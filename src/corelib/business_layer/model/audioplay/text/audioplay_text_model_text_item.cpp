#include "audioplay_text_model_text_item.h"

#include "audioplay_text_model.h"

#include <business_layer/chronometry/chronometer.h>
#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/templates_facade.h>

#include <QVariant>


namespace BusinessLayer {

class AudioplayTextModelTextItem::Implementation
{
public:
    /**
     * @brief Длительность блока
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{ 0 };
};

AudioplayTextModelTextItem::AudioplayTextModelTextItem(const AudioplayTextModel* _model)
    : TextModelTextItem(_model)
    , d(new Implementation)
{
}

AudioplayTextModelTextItem::~AudioplayTextModelTextItem() = default;

std::chrono::milliseconds AudioplayTextModelTextItem::duration() const
{
    return d->duration;
}

void AudioplayTextModelTextItem::updateDuration()
{
    //
    // Не учитываем хронометраж некоторых блококв
    //
    if (paragraphType() == TextParagraphType::BeatHeading) {
        return;
    }

    const auto audioplayModel = qobject_cast<const AudioplayTextModel*>(model());
    Q_ASSERT(audioplayModel);
    const auto duration = AudioplayChronometer::duration(
        paragraphType(), text(), audioplayModel->informationModel()->templateId());
    if (d->duration == duration) {
        return;
    }

    d->duration = duration;

    //
    // Помещаем изменённым для пересчёта хронометража в родительском элементе
    //
    markChanged();
}

QVariant AudioplayTextModelTextItem::data(int _role) const
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

void AudioplayTextModelTextItem::handleChange()
{
    updateDuration();
}

} // namespace BusinessLayer
