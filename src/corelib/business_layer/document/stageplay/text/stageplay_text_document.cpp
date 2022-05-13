#include "stageplay_text_document.h"

#include "stageplay_text_corrector.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/stageplay_template.h>


namespace BusinessLayer {

StageplayTextDocument::StageplayTextDocument(QObject* _parent)
    : TextDocument(_parent)
{
    setCorrector(new StageplayTextCorrector(this));
}

StageplayTextDocument::~StageplayTextDocument() = default;

void StageplayTextDocument::setCorrectionOptions(bool _needToCorrectPageBreaks)
{
    QStringList correctionOptions;
    if (_needToCorrectPageBreaks) {
        correctionOptions.append("correct-page-breaks");
    }
    TextDocument::setCorrectionOptions(correctionOptions);
}

QString StageplayTextDocument::blockNumber(const QTextBlock& _forBlock) const
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

} // namespace BusinessLayer
