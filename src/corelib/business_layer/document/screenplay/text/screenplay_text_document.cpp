#include "screenplay_text_document.h"

#include "screenplay_text_corrector.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/templates/screenplay_template.h>


namespace BusinessLayer {

class ScreenplayTextDocument::Implementation
{
public:
    explicit Implementation(ScreenplayTextDocument* _q);


    /**
     * @brief Владелец
     */
    ScreenplayTextDocument* q = nullptr;

    /**
     * @brief Отображать ли элементы поэпизодника (true) или текста сценария (false)
     */
    bool isTreatmentDocument = false;

    /**
     * @brief Отображать ли биты (только для режима сценария, когда isTreatmentDocument == false)
     */
    bool isBeatsVisible = false;
};

ScreenplayTextDocument::Implementation::Implementation(ScreenplayTextDocument* _q)
    : q(_q)
{
}


// ****


ScreenplayTextDocument::ScreenplayTextDocument(QObject* _parent)
    : TextDocument(_parent)
    , d(new Implementation(this))
{
    setCorrector(new ScreenplayTextCorrector(this));

    connect(this, &ScreenplayTextDocument::contentsChanged, this, [this] {
        if (!canChangeModel()) {
            return;
        }

        auto screenplayModel = qobject_cast<ScreenplayTextModel*>(model());
        if (screenplayModel == nullptr) {
            return;
        }

        if (d->isTreatmentDocument) {
            screenplayModel->setTreatmentPageCount(pageCount());

            //
            // Если включён режим отображения поэпизодника, то обработаем кейс, когда в документе
            // есть только один блок и он невидим в поэпизоднике
            //
            if (blockCount() == 1
                && !visibleBlocksTypes().contains(TextBlockStyle::forBlock(begin()))) {
                setParagraphType(TextParagraphType::SceneHeading, TextCursor(this));
            }
        } else {
            screenplayModel->setScriptPageCount(pageCount());
        }
    });
}

ScreenplayTextDocument::~ScreenplayTextDocument() = default;

bool ScreenplayTextDocument::isTreatmentDocument() const
{
    return d->isTreatmentDocument;
}

void ScreenplayTextDocument::setTreatmentDocument(bool _treatment)
{
    if (d->isTreatmentDocument == _treatment) {
        return;
    }

    d->isTreatmentDocument = _treatment;
}

bool ScreenplayTextDocument::isBeatsVisible() const
{
    return d->isBeatsVisible;
}

void ScreenplayTextDocument::setBeatsVisible(bool _visible)
{
    if (d->isBeatsVisible == _visible) {
        return;
    }

    d->isBeatsVisible = _visible;

    emit contentsChange(0, -1, -1);
    emit contentsChanged();
}

QSet<TextParagraphType> ScreenplayTextDocument::visibleBlocksTypes() const
{
    if (d->isTreatmentDocument) {
        return {
            TextParagraphType::SceneHeading,
            TextParagraphType::SceneHeadingShadowTreatment,
            TextParagraphType::SceneCharacters,
            TextParagraphType::BeatHeading,
            TextParagraphType::BeatHeadingShadowTreatment,
            TextParagraphType::ActHeading,
            TextParagraphType::ActHeadingShadowTreatment,
            TextParagraphType::ActFooter,
            TextParagraphType::SequenceHeading,
            TextParagraphType::SequenceHeadingShadowTreatment,
            TextParagraphType::SequenceFooter,
        };
    }

    if (d->isBeatsVisible) {
        return {
            TextParagraphType::SceneHeading,
            TextParagraphType::SceneHeadingShadow,
            TextParagraphType::SceneCharacters,
            TextParagraphType::BeatHeading,
            TextParagraphType::BeatHeadingShadow,
            TextParagraphType::Action,
            TextParagraphType::Character,
            TextParagraphType::Parenthetical,
            TextParagraphType::Dialogue,
            TextParagraphType::Lyrics,
            TextParagraphType::Shot,
            TextParagraphType::Transition,
            TextParagraphType::InlineNote,
            TextParagraphType::UnformattedText,
            TextParagraphType::ActHeading,
            TextParagraphType::ActHeadingShadow,
            TextParagraphType::ActFooter,
            TextParagraphType::SequenceHeading,
            TextParagraphType::SequenceHeadingShadow,
            TextParagraphType::SequenceFooter,
            TextParagraphType::PageSplitter,
        };
    }

    return {
        TextParagraphType::SceneHeading,
        TextParagraphType::SceneHeadingShadow,
        TextParagraphType::SceneCharacters,
        TextParagraphType::Action,
        TextParagraphType::Character,
        TextParagraphType::Parenthetical,
        TextParagraphType::Dialogue,
        TextParagraphType::Lyrics,
        TextParagraphType::Shot,
        TextParagraphType::Transition,
        TextParagraphType::InlineNote,
        TextParagraphType::UnformattedText,
        TextParagraphType::ActHeading,
        TextParagraphType::ActHeadingShadow,
        TextParagraphType::ActFooter,
        TextParagraphType::SequenceHeading,
        TextParagraphType::SequenceHeadingShadow,
        TextParagraphType::SequenceFooter,
        TextParagraphType::PageSplitter,
    };
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
    if (blockData == nullptr || blockData->item() == nullptr) {
        return {};
    }

    const auto itemParent = blockData->item()->parent();
    if (itemParent == nullptr || itemParent->type() != TextModelItemType::Group) {
        return {};
    }

    const auto sceneItem = static_cast<const ScreenplayTextModelSceneItem*>(itemParent);
    return sceneItem->number().value_or(TextModelGroupItem::Number()).text;
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

    const auto textItem = static_cast<const TextModelTextItem*>(item);
    return textItem->number().value_or(TextModelTextItem::Number()).text;
}

QVector<TextModelTextItem::ResourceMark> ScreenplayTextDocument::resourceMarks(
    const QTextBlock& _forBlock)
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
    return textItem->resourceMarks();
}

} // namespace BusinessLayer
