#include "simple_text_document.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/model/simple_text/simple_text_model_chapter_item.h>


namespace BusinessLayer {

SimpleTextDocument::SimpleTextDocument(QObject* _parent)
    : TextDocument(_parent)
{
}

QString SimpleTextDocument::chapterNumber(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    auto blockData = static_cast<TextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    auto itemParent = blockData->item()->parent();
    if (itemParent == nullptr || itemParent->type() != TextModelItemType::Group) {
        return {};
    }

    auto itemScene = static_cast<SimpleTextModelChapterItem*>(itemParent);
    return itemScene->number().value_or(TextModelGroupItem::Number()).text;
}

} // namespace BusinessLayer
