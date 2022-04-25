#include "audioplay_text_document.h"

#include "audioplay_text_corrector.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/audioplay_template.h>


namespace BusinessLayer {

AudioplayTextDocument::AudioplayTextDocument(QObject* _parent)
    : TextDocument(_parent)
{
    setCorrector(new AudioplayTextCorrector(this));
}

AudioplayTextDocument::~AudioplayTextDocument() = default;

void AudioplayTextDocument::setCorrectionOptions(bool _needToCorrectPageBreaks)
{
    QStringList correctionOptions;
    if (_needToCorrectPageBreaks) {
        correctionOptions.append("correct-page-breaks");
    }
    TextDocument::setCorrectionOptions(correctionOptions);
}

QString AudioplayTextDocument::blockNumber(const QTextBlock& _forBlock) const
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
