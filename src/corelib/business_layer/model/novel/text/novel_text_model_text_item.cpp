#include "novel_text_model_text_item.h"

#include "novel_text_model.h"

#include <business_layer/templates/novel_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/run_once.h>


namespace BusinessLayer {

NovelTextModelTextItem::NovelTextModelTextItem(const NovelTextModel* _model)
    : TextModelTextItem(_model)
{
}

NovelTextModelTextItem::~NovelTextModelTextItem() = default;

void NovelTextModelTextItem::updateCounters(bool _force)
{
    Q_UNUSED(_force)

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
    // Если элемент не менялся после последнего подсчёта счётчиков, не делаем лишнюю работу
    //
    if ((wordsCount() != 0 || charactersCount() != QPair<int, int>()) && !isChanged()) {
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
    // Если не было изменений, то и ладно, выходим тогда
    //
    if (wordsCount() == currentWordsCount
        && charactersCount() == QPair<int, int>(charactersCountFirst, charactersCountSecond)) {
        return;
    }

    setWordsCount(currentWordsCount);
    setCharactersCount({ charactersCountFirst, charactersCountSecond });

    //
    // Помечаем изменённым для пересчёта счетчиков в родительском элементе
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

} // namespace BusinessLayer
