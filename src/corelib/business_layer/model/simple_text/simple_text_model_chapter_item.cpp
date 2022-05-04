#include "simple_text_model_chapter_item.h"

#include "simple_text_model.h"

#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/simple_text_template.h>
#include <utils/helpers/text_helper.h>

#include <QXmlStreamReader>

namespace BusinessLayer {

class SimpleTextModelChapterItem::Implementation
{
public:
    //
    // Ридонли свойства, которые формируются по ходу работы со сценарием
    //

    /**
     * @brief Количество слов главы
     */
    int wordsCount = 0;
};


// ****


SimpleTextModelChapterItem::SimpleTextModelChapterItem(const SimpleTextModel* _model,
                                                       TextGroupType _type)
    : TextModelGroupItem(_model)
    , d(new Implementation)
{
    setGroupType(_type);
    setLevel(static_cast<int>(_type));
}

SimpleTextModelChapterItem::~SimpleTextModelChapterItem() = default;

int SimpleTextModelChapterItem::wordsCount() const
{
    return d->wordsCount;
}

QVariant SimpleTextModelChapterItem::data(int _role) const
{
    switch (_role) {
    case ChapterWordsCountRole: {
        return d->wordsCount;
    }

    default: {
        return TextModelGroupItem::data(_role);
    }
    }
}

QStringRef SimpleTextModelChapterItem::readCustomContent(QXmlStreamReader& _contentReader)
{
    return _contentReader.name();
}

QByteArray SimpleTextModelChapterItem::customContent() const
{
    return {};
}

void SimpleTextModelChapterItem::handleChange()
{
    QString heading;
    QString text;
    int inlineNotesSize = 0;
    int childGroupsReviewMarksSize = 0;
    QVector<TextModelTextItem::ReviewMark> reviewMarks;
    d->wordsCount = 0;

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Group: {
            auto childChapterItem = static_cast<SimpleTextModelChapterItem*>(child);
            const int maxTextLength = 1000;
            if (text.length() < maxTextLength) {
                text.append(childChapterItem->heading() + " " + childChapterItem->text());
            } else if (text.length() > maxTextLength) {
                text = text.left(maxTextLength);
            }
            childGroupsReviewMarksSize += childChapterItem->reviewMarksSize();
            d->wordsCount += childChapterItem->wordsCount();
            break;
        }

        case TextModelItemType::Text: {
            auto childTextItem = static_cast<TextModelTextItem*>(child);
            switch (childTextItem->paragraphType()) {
            case TextParagraphType::ChapterHeading1:
            case TextParagraphType::ChapterHeading2:
            case TextParagraphType::ChapterHeading3:
            case TextParagraphType::ChapterHeading4:
            case TextParagraphType::ChapterHeading5:
            case TextParagraphType::ChapterHeading6: {
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
            // Собираем количество слов
            //
            d->wordsCount += TextHelper::wordsCount(childTextItem->text());

            break;
        }

        default: {
            break;
        }
        }
    }

    setHeading(heading);
    setText(text);
    setInlineNotesSize(inlineNotesSize);
    setReviewMarksSize(childGroupsReviewMarksSize
                       + std::count_if(reviewMarks.begin(), reviewMarks.end(),
                                       [](const TextModelTextItem::ReviewMark& _reviewMark) {
                                           return !_reviewMark.isDone;
                                       }));
}

} // namespace BusinessLayer
