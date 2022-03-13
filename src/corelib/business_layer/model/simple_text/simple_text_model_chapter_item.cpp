#include "simple_text_model_chapter_item.h"

#include "simple_text_model.h"
#include "simple_text_model_text_item.h"

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


SimpleTextModelChapterItem::SimpleTextModelChapterItem(const SimpleTextModel* _model)
    : TextModelGroupItem(_model)
    , d(new Implementation)
{
    setGroupType(TextGroupType::Chapter);
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
    int level = 0;
    QString heading;
    QString text;
    int inlineNotesSize = 0;
    int reviewMarksSize = 0;

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
            d->wordsCount += childChapterItem->wordsCount();
            break;
        }

        case TextModelItemType::Text: {
            auto childTextItem = static_cast<SimpleTextModelTextItem*>(child);
            switch (childTextItem->paragraphType()) {
            case TextParagraphType::Heading1:
            case TextParagraphType::Heading2:
            case TextParagraphType::Heading3:
            case TextParagraphType::Heading4:
            case TextParagraphType::Heading5:
            case TextParagraphType::Heading6: {
                level = static_cast<int>(childTextItem->paragraphType());
                heading = childTextItem->text();
                d->wordsCount += TextHelper::wordsCount(childTextItem->text());
                break;
            }

            case TextParagraphType::InlineNote: {
                ++inlineNotesSize;
                break;
            }

            default: {
                text.append(childTextItem->text() + " ");
                d->wordsCount += TextHelper::wordsCount(childTextItem->text());
                reviewMarksSize += std::count_if(
                    childTextItem->reviewMarks().begin(), childTextItem->reviewMarks().end(),
                    [](const SimpleTextModelTextItem::ReviewMark& _reviewMark) {
                        return !_reviewMark.isDone;
                    });
                break;
            }
            }

            break;
        }

        default: {
            break;
        }
        }
    }

    setLevel(level);
    setHeading(heading);
    setText(text);
    setInlineNotesSize(inlineNotesSize);
    setReviewMarksSize(reviewMarksSize);
}

} // namespace BusinessLayer
