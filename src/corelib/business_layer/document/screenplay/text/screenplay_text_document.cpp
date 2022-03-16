#include "screenplay_text_document.h"

#include "screenplay_text_corrector.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>


namespace BusinessLayer {

ScreenplayTextDocument::ScreenplayTextDocument(QObject* _parent)
    : TextDocument(_parent)
{
    setCorrector(new ScreenplayTextCorrector(this));
}

void ScreenplayTextDocument::setCorrectionOptions(bool _needToCorrectCharactersNames,
                                                  bool _needToCorrectPageBreaks)
{
    QStringList correctionOptions;
    if (_needToCorrectCharactersNames) {
        correctionOptions.append("correct-characters-names");
    }
    if (_needToCorrectPageBreaks) {
        correctionOptions.append("correct-page-breaks");
    }
    TextDocument::setCorrectionOptions(correctionOptions);
}

QString ScreenplayTextDocument::sceneNumber(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<TextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    const auto itemParent = blockData->item()->parent();
    if (itemParent == nullptr || itemParent->type() != TextModelItemType::Group) {
        return {};
    }

    const auto sceneItem = static_cast<const ScreenplayTextModelSceneItem*>(itemParent);
    return sceneItem->number()->text;
}

QString ScreenplayTextDocument::dialogueNumber(const QTextBlock& _forBlock) const
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

    const auto sceneItem = static_cast<const ScreenplayTextModelTextItem*>(item);
    return sceneItem->number().value_or(ScreenplayTextModelTextItem::Number()).text;
}

} // namespace BusinessLayer
