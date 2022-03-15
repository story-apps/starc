#include "comic_book_text_model_panel_item.h"

#include "comic_book_text_block_parser.h"
#include "comic_book_text_model.h"

#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/comic_book_template.h>
#include <utils/helpers/text_helper.h>

#include <QXmlStreamReader>


namespace BusinessLayer {

class ComicBookTextModelPanelItem::Implementation
{
public:
    //
    // Ридонли свойства, которые формируются по ходу работы с текстом
    //

    /**
     * @brief Количество слов в диалогах
     */
    int dialoguesWordsCount = 0;
};


// ****


ComicBookTextModelPanelItem::ComicBookTextModelPanelItem(const ComicBookTextModel* _model)
    : TextModelGroupItem(_model)
    , d(new Implementation)
{
    setGroupType(TextGroupType::Panel);
    setLevel(1);
}

ComicBookTextModelPanelItem::~ComicBookTextModelPanelItem() = default;

int ComicBookTextModelPanelItem::dialoguesWordsCount() const
{
    return d->dialoguesWordsCount;
}

QVariant ComicBookTextModelPanelItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return u8"\U000F0B77";
    }

    case PanelDialoguesWordsSizeRole: {
        return d->dialoguesWordsCount;
    }

    default: {
        return TextModelGroupItem::data(_role);
    }
    }
}

QStringRef ComicBookTextModelPanelItem::readCustomContent(QXmlStreamReader& _contentReader)
{
    return _contentReader.name();
}

QByteArray ComicBookTextModelPanelItem::customContent() const
{
    return {};
}

void ComicBookTextModelPanelItem::handleChange()
{
    QString heading;
    QString text;
    int inlineNotesSize = 0;
    int reviewMarksSize = 0;
    d->dialoguesWordsCount = 0;

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        if (child->type() != TextModelItemType::Text) {
            continue;
        }

        auto childTextItem = static_cast<TextModelTextItem*>(child);

        //
        // Собираем текст
        //
        switch (childTextItem->paragraphType()) {
        case TextParagraphType::PanelHeading: {
            heading
                = TextHelper::smartToUpper(ComicBookPanelParser::panelTitle(childTextItem->text()));
            const auto panelDescription
                = ComicBookPanelParser::panelDescription(childTextItem->text());
            text = panelDescription;
            break;
        }

        case TextParagraphType::Dialogue: {
            d->dialoguesWordsCount += TextHelper::wordsCount(childTextItem->text());
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
            reviewMarksSize += std::count_if(childTextItem->reviewMarks().begin(),
                                             childTextItem->reviewMarks().end(),
                                             [](const TextModelTextItem::ReviewMark& _reviewMark) {
                                                 return !_reviewMark.isDone;
                                             });
            break;
        }
        }
    }

    setHeading(heading);
    setText(text);
    setInlineNotesSize(inlineNotesSize);
    setReviewMarksSize(reviewMarksSize);
}

} // namespace BusinessLayer
