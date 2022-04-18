#include "comic_book_text_document.h"

#include "comic_book_text_corrector.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_page_item.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_panel_item.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/comic_book_template.h>

namespace BusinessLayer {

ComicBookTextDocument::ComicBookTextDocument(QObject* _parent)
    : TextDocument(_parent)
{
    setCorrector(new ComicBookTextCorrector(this));
}

void ComicBookTextDocument::setCorrectionOptions(bool _needToCorrectCharactersNames,
                                                 bool _needToCorrectPageBreaks)
{
    QStringList correctionOptions;
    if (_needToCorrectCharactersNames) {
        correctionOptions.append("correct-characters-names");
    }
    correctionOptions.append("correct-blocks-numbers");
    correctionOptions.append("bold-panel-title");
    if (_needToCorrectPageBreaks) {
        correctionOptions.append("correct-page-breaks");
    }
    TextDocument::setCorrectionOptions(correctionOptions);
}

QString ComicBookTextDocument::pageNumber(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<TextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    const auto itemParent = blockData->item()->parent();
    if (itemParent == nullptr || itemParent->type() != TextModelItemType::Group
        || static_cast<TextModelGroupItem*>(itemParent)->groupType() != TextGroupType::Page) {
        return {};
    }

    const auto panelItem = static_cast<const ComicBookTextModelPageItem*>(itemParent);
    return panelItem->pageNumber()->text;
}

QString ComicBookTextDocument::panelNumber(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<TextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    const auto itemParent = blockData->item()->parent();
    if (itemParent == nullptr || itemParent->type() != TextModelItemType::Group
        || static_cast<TextModelGroupItem*>(itemParent)->groupType() != TextGroupType::Panel) {
        return {};
    }

    const auto panelItem = static_cast<const ComicBookTextModelPanelItem*>(itemParent);
    return panelItem->number()->text;
}

QString ComicBookTextDocument::dialogueNumber(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<TextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    const auto item = blockData->item();
    if (item == nullptr || item->type() != TextModelItemType::Text) {
        return {};
    }

    const auto textItem = static_cast<const TextModelTextItem*>(item);
    return textItem->number().value_or(TextModelTextItem::Number()).text;
}

TextModelTextItem::Bookmark ComicBookTextDocument::bookmark(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<TextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    const auto item = blockData->item();
    if (item == nullptr || item->type() != TextModelItemType::Text) {
        return {};
    }

    const auto textItem = static_cast<const TextModelTextItem*>(item);
    return textItem->bookmark().value_or(TextModelTextItem::Bookmark());
}

} // namespace BusinessLayer
