#include "screenplay_text_document.h"

#include "screenplay_text_corrector.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/templates/screenplay_template.h>


namespace BusinessLayer {

class ScreenplayTextDocument::Implementation
{
public:
    explicit Implementation(ScreenplayTextDocument* _q);

    /**
     * @brief Получить список видимых блоков в зависимости от режима отображения поэпизодника или
     * сценария
     */
    QSet<TextParagraphType> visibleBlocksTypes() const;

    /**
     * @brief Обновить видимость блоков в заданном интервале
     */
    void updateBlocksVisibility(int _from, int _to);


    /**
     * @brief Владелец
     */
    ScreenplayTextDocument* q = nullptr;

    /**
     * @brief Отображать ли элементы поэпизодника (true) или текста сценария (false)
     */
    bool isTreatmentVisible = false;
};

ScreenplayTextDocument::Implementation::Implementation(ScreenplayTextDocument* _q)
    : q(_q)
{
}

QSet<TextParagraphType> ScreenplayTextDocument::Implementation::visibleBlocksTypes() const
{
    if (isTreatmentVisible) {
        return {
            TextParagraphType::SceneHeading,      TextParagraphType::SceneHeadingShadowTreatment,
            TextParagraphType::SceneCharacters,   TextParagraphType::BeatHeading,
            TextParagraphType::BeatHeadingShadow, TextParagraphType::ActHeading,
            TextParagraphType::ActFooter,         TextParagraphType::SequenceHeading,
            TextParagraphType::SequenceFooter,
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
        TextParagraphType::ActFooter,
        TextParagraphType::SequenceHeading,
        TextParagraphType::SequenceFooter,
        TextParagraphType::PageSplitter,
    };
}

void ScreenplayTextDocument::Implementation::updateBlocksVisibility(int _from, int _to)
{
    //
    // Сформируем список типов блоков для отображения
    //
    const auto visibleBlocksTypes = this->visibleBlocksTypes();

    //
    // Пробегаем документ и настраиваем видимые и невидимые блоки
    //
    TextCursor cursor(q);
    cursor.setPosition(_from);
    while (cursor.position() <= _to) {
        auto block = cursor.block();
        const auto blockType = TextBlockStyle::forBlock(block);

        //
        // В некоторых случаях, мы попадаем сюда, когда документ не до конца настроен, поэтому
        // когда обнаруживается такая ситация, завершаем выполнение
        //
        if (blockType == TextParagraphType::Undefined) {
            break;
        }

        //
        // ... уберём отступы у скрытых блоков, чтобы они не ломали компановку документа
        //
        block.setVisible(visibleBlocksTypes.contains(blockType));
        if (!block.isVisible()) {
            if (!cursor.isInEditBlock()) {
                cursor.beginEditBlock();
            }
            auto blockFormat = cursor.blockFormat();
            blockFormat.setTopMargin(0);
            blockFormat.setBottomMargin(0);
            cursor.setBlockFormat(blockFormat);
        }

        if (cursor.atEnd()) {
            break;
        }

        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::NextBlock);
    }

    if (cursor.isInEditBlock()) {
        cursor.endEditBlock();
    }
}


// ****


ScreenplayTextDocument::ScreenplayTextDocument(QObject* _parent)
    : TextDocument(_parent)
    , d(new Implementation(this))
{
    setCorrector(new ScreenplayTextCorrector(this));

    connect(this, &ScreenplayTextDocument::contentsChange, this,
            [this](int _position, int _removed, int _added) {
                d->updateBlocksVisibility(_position, _position + std::max(_removed, _added));
            });
}

ScreenplayTextDocument::~ScreenplayTextDocument() = default;

bool ScreenplayTextDocument::isTreatmentVisible() const
{
    return d->isTreatmentVisible;
}

void ScreenplayTextDocument::setTreatmentVisible(bool _visible)
{
    if (d->isTreatmentVisible == _visible) {
        return;
    }

    d->isTreatmentVisible = _visible;
    d->updateBlocksVisibility(0, characterCount());
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

} // namespace BusinessLayer
