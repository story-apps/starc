#include "novel_text_model_beat_item.h"

#include "novel_text_model.h"
#include "novel_text_model_text_item.h"

#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/novel_template.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

class NovelTextModelBeatItem::Implementation
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


// ****


NovelTextModelBeatItem::NovelTextModelBeatItem(const NovelTextModel* _model)
    : TextModelGroupItem(_model)
    , d(new Implementation)
{
    setGroupType(TextGroupType::Beat);
}

NovelTextModelBeatItem::~NovelTextModelBeatItem() = default;

int NovelTextModelBeatItem::wordsCount() const
{
    return d->wordsCount;
}

QPair<int, int> NovelTextModelBeatItem::charactersCount() const
{
    return d->charactersCount;
}

QVariant NovelTextModelBeatItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return u8"\U000F0B77";
    }

    default: {
        return TextModelGroupItem::data(_role);
    }
    }
}

void NovelTextModelBeatItem::handleChange()
{
    QString heading;
    QString text;
    int inlineNotesSize = 0;
    QVector<TextModelTextItem::ReviewMark> reviewMarks;
    d->wordsCount = 0;
    d->charactersCount = {};

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        if (child->type() != TextModelItemType::Text) {
            continue;
        }

        auto childTextItem = static_cast<NovelTextModelTextItem*>(child);

        //
        // Собираем текст
        //
        switch (childTextItem->paragraphType()) {
        case TextParagraphType::BeatHeading: {
            heading = childTextItem->text();
            break;
        }

        case TextParagraphType::InlineNote: {
            ++inlineNotesSize;
            break;
        }

        default: {
            if (!text.isEmpty() && !childTextItem->text().isEmpty()) {
                text.append(" ");
            }
            text.append(childTextItem->text());
            break;
        }
        }

        //
        // Собираем редакторские заметки
        //
        if (!reviewMarks.isEmpty() && !childTextItem->reviewMarks().isEmpty()
            && reviewMarks.constLast().isPartiallyEqual(
                childTextItem->reviewMarks().constFirst())) {
            reviewMarks.removeLast();
        }
        reviewMarks.append(childTextItem->reviewMarks());

        //
        // Собираем счётчики
        //
        d->wordsCount += childTextItem->wordsCount();
        d->charactersCount.first += childTextItem->charactersCount().first;
        d->charactersCount.second += childTextItem->charactersCount().second;
    }

    setHeading(heading);
    setText(text);
    setInlineNotesSize(inlineNotesSize);
    setReviewMarksSize(std::count_if(
        reviewMarks.begin(), reviewMarks.end(),
        [](const TextModelTextItem::ReviewMark& _reviewMark) { return !_reviewMark.isDone; }));
}

} // namespace BusinessLayer
