#include "novel_text_model_text_item.h"

#include "novel_text_model.h"

#include <business_layer/chronometry/chronometer.h>
#include <business_layer/model/novel/novel_information_model.h>
#include <business_layer/templates/novel_template.h>
#include <business_layer/templates/templates_facade.h>
#include <utils/helpers/string_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/run_once.h>

#include <QColor>
#include <QLocale>
#include <QVariant>
#include <QXmlStreamReader>


namespace BusinessLayer {

class NovelTextModelTextItem::Implementation
{
public:
    //
    // Ридонли свойства, которые формируются по ходу работы со сценарием
    //

    /**
     * @brief Количество слов
     */
    int wordsCount = 0;

    /**
     * @brief Количество символов
     */
    QPair<int, int> charactersCount;
};

NovelTextModelTextItem::NovelTextModelTextItem(const NovelTextModel* _model)
    : TextModelTextItem(_model)
    , d(new Implementation)
{
}

NovelTextModelTextItem::~NovelTextModelTextItem() = default;

int NovelTextModelTextItem::wordsCount() const
{
    return d->wordsCount;
}

QPair<int, int> NovelTextModelTextItem::charactersCount() const
{
    return d->charactersCount;
}

void NovelTextModelTextItem::updateCounters()
{
    auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    //
    // Не учитываем некоторые блоки
    //
    if (paragraphType() == TextParagraphType::BeatHeading) {
        return;
    }

    //
    // Если элемент не менялся после последнего подсчёта счётчиков, не делаем лишнюю работу
    //
    if ((d->wordsCount != 0 || d->charactersCount != QPair<int, int>()) && !isChanged()) {
        return;
    }

    //
    // Считаем
    //
    const auto wordsCount = TextHelper::wordsCount(text());
    //
    const auto charactersCountFirst = text().length() - text().count(' ');
    const auto charactersCountSecond
        = text().length() + 1; // всегда добавляем единичку за перенос строки

    //
    // Если не было изменений, то и ладно, выходим тогда
    //
    if (d->wordsCount == wordsCount
        && d->charactersCount == QPair<int, int>(charactersCountFirst, charactersCountSecond)) {
        return;
    }

    d->wordsCount = wordsCount;
    d->charactersCount.first = charactersCountFirst;
    d->charactersCount.second = charactersCountSecond;

    //
    // Помещаем изменённым для пересчёта хронометража в родительском элементе
    //
    markChanged();
}

QVariant NovelTextModelTextItem::data(int _role) const
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

void NovelTextModelTextItem::handleChange()
{
    updateCounters();
}

} // namespace BusinessLayer
