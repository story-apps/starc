#include "novel_text_document.h"

#include "novel_text_corrector.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/novel/text/novel_text_model.h>
#include <business_layer/model/novel/text/novel_text_model_scene_item.h>
#include <business_layer/templates/novel_template.h>


namespace BusinessLayer {

class NovelTextDocument::Implementation
{
public:
    explicit Implementation(NovelTextDocument* _q);


    /**
     * @brief Владелец
     */
    NovelTextDocument* q = nullptr;

    /**
     * @brief Отображать ли элементы поэпизодника (true) или текста сценария (false)
     */
    bool isOutlineDocument = false;

    /**
     * @brief Отображать ли биты (только для режима сценария, когда isTreatmentDocument == false)
     */
    bool isBeatsVisible = false;
};

NovelTextDocument::Implementation::Implementation(NovelTextDocument* _q)
    : q(_q)
{
}


// ****


NovelTextDocument::NovelTextDocument(QObject* _parent)
    : TextDocument(_parent)
    , d(new Implementation(this))
{
    setCorrector(new NovelTextCorrector(this));

    connect(this, &NovelTextDocument::contentsChanged, this, [this] {
        if (!canChangeModel()) {
            return;
        }

        auto novelModel = qobject_cast<NovelTextModel*>(model());
        if (novelModel == nullptr) {
            return;
        }

        if (d->isOutlineDocument) {
            novelModel->setOutlinePageCount(pageCount());

            //
            // Если включён режим отображения поэпизодника, то обработаем кейс, когда в документе
            // есть только один блок и он невидим в поэпизоднике
            //
            if (blockCount() == 1
                && !visibleBlocksTypes().contains(TextBlockStyle::forBlock(begin()))) {
                setParagraphType(TextParagraphType::SceneHeading, TextCursor(this));
            }
        } else {
            novelModel->setTextPageCount(pageCount());
        }
    });
}

NovelTextDocument::~NovelTextDocument() = default;

bool NovelTextDocument::isOutlineDocument() const
{
    return d->isOutlineDocument;
}

void NovelTextDocument::setOutlineDocument(bool _treatment)
{
    if (d->isOutlineDocument == _treatment) {
        return;
    }

    d->isOutlineDocument = _treatment;
}

bool NovelTextDocument::isBeatsVisible() const
{
    return d->isBeatsVisible;
}

void NovelTextDocument::setBeatsVisible(bool _visible)
{
    if (d->isBeatsVisible == _visible) {
        return;
    }

    d->isBeatsVisible = _visible;

    emit contentsChange(0, 0, 0);
    emit contentsChanged();
}

QSet<TextParagraphType> NovelTextDocument::visibleBlocksTypes() const
{
    if (d->isOutlineDocument) {
        return {
            TextParagraphType::SceneHeading,   TextParagraphType::SceneHeadingShadowTreatment,
            TextParagraphType::BeatHeading,    TextParagraphType::BeatHeadingShadowTreatment,
            TextParagraphType::PartHeading,    TextParagraphType::PartFooter,
            TextParagraphType::ChapterHeading, TextParagraphType::ChapterFooter,
        };
    }

    if (d->isBeatsVisible) {
        return {
            TextParagraphType::SceneHeading,
            TextParagraphType::SceneHeadingShadow,
            TextParagraphType::BeatHeading,
            TextParagraphType::BeatHeadingShadow,
            TextParagraphType::Text,
            TextParagraphType::InlineNote,
            TextParagraphType::UnformattedText,
            TextParagraphType::PartHeading,
            TextParagraphType::PartFooter,
            TextParagraphType::ChapterHeading,
            TextParagraphType::ChapterFooter,
        };
    }

    return {
        TextParagraphType::SceneHeading,
        TextParagraphType::SceneHeadingShadow,
        TextParagraphType::Text,
        TextParagraphType::InlineNote,
        TextParagraphType::UnformattedText,
        TextParagraphType::PartHeading,
        TextParagraphType::PartFooter,
        TextParagraphType::ChapterHeading,
        TextParagraphType::ChapterFooter,
        TextParagraphType::PageSplitter,
    };
}

void NovelTextDocument::setCorrectionOptions(bool _needToCorrectPageBreaks)
{
    QStringList correctionOptions;
    if (_needToCorrectPageBreaks) {
        correctionOptions.append("correct-page-breaks");
    }
    TextDocument::setCorrectionOptions(correctionOptions);
}

QString NovelTextDocument::sceneNumber(const QTextBlock& _forBlock) const
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

    const auto sceneItem = static_cast<const NovelTextModelSceneItem*>(itemParent);
    return sceneItem->number().value_or(TextModelGroupItem::Number()).text;
}

QString NovelTextDocument::dialogueNumber(const QTextBlock& _forBlock) const
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
