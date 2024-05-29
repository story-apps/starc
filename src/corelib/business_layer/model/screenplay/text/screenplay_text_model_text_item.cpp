#include "screenplay_text_model_text_item.h"

#include "screenplay_text_model.h"

#include <business_layer/chronometry/chronometer.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/run_once.h>


namespace BusinessLayer {

class ScreenplayTextModelTextItem::Implementation
{
public:
    //
    // Ридонли свойства, которые формируются по ходу работы
    //

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

void ScreenplayTextModelTextItem::updateCounters(bool _force)
{
    auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    //
    // Не учитываем биты
    //
    if (paragraphType() == TextParagraphType::BeatHeading) {
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

    //
    // Считаем
    //
    const auto currentWordsCount = TextHelper::wordsCount(text());
    //
    const auto charactersCountFirst = text().length() - text().count(' ');
    const auto charactersCountSecond
        = text().length() + 1; // всегда добавляем единичку за перенос строки
    //
    const auto screenplayModel = qobject_cast<const ScreenplayTextModel*>(model());
    Q_ASSERT(screenplayModel);
    const auto duration = ScreenplayChronometer::duration(
        paragraphType(), text(), screenplayModel->informationModel()->templateId());

    //
    // Если не было изменений, то и ладно, выходим тогда
    //
    if (wordsCount() == currentWordsCount
        && charactersCount() == QPair<int, int>(charactersCountFirst, charactersCountSecond)
        && d->duration == duration) {
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

} // namespace BusinessLayer
