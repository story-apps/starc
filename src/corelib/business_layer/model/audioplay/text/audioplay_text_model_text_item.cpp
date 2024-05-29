#include "audioplay_text_model_text_item.h"

#include "audioplay_text_model.h"

#include <business_layer/chronometry/chronometer.h>
#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/templates/audioplay_template.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/run_once.h>


namespace BusinessLayer {

class AudioplayTextModelTextItem::Implementation
{
public:
    /**
     * @brief Длительность блока
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{ 0 };
};


// ****


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

void AudioplayTextModelTextItem::updateCounters(bool _force)
{
    auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    //
    // Если это не принудительное оновление счётчиков и элемент не менялся после последнего подсчёта
    // счётчиков, не делаем лишнюю работу
    //
    if (!_force
        && (wordsCount() != 0 || charactersCount() != QPair<int, int>()
            || d->duration != std::chrono::milliseconds{ 0 })
        && !isChanged()) {
        return;
    }

    const auto audioplayModel = qobject_cast<const AudioplayTextModel*>(model());
    Q_ASSERT(audioplayModel);

    //
    // Считаем
    //
    const auto duration = AudioplayChronometer::duration(
        paragraphType(), text(), audioplayModel->informationModel()->templateId());
    const auto currentWordsCount = TextHelper::wordsCount(text());
    //
    const auto charactersCountFirst = text().length() - text().count(' ');
    const auto charactersCountSecond
        = text().length() + 1; // всегда добавляем единичку за перенос строки

    //
    // Если не было изменений, то и ладно, выходим тогда
    //
    if (d->duration == duration && wordsCount() == currentWordsCount
        && charactersCount() == QPair<int, int>(charactersCountFirst, charactersCountSecond)) {
        return;
    }

    d->duration = duration;
    setWordsCount(currentWordsCount);
    setCharactersCount(QPair<int, int>(charactersCountFirst, charactersCountSecond));

    //
    // Помечаем изменённым для пересчёта счетчиков в родительском элементе
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

} // namespace BusinessLayer
