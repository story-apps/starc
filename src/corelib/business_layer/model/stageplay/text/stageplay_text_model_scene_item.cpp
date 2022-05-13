#include "stageplay_text_model_scene_item.h"

#include "stageplay_text_model.h"

#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/stageplay_template.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

StageplayTextModelSceneItem::StageplayTextModelSceneItem(const StageplayTextModel* _model)
    : TextModelGroupItem(_model)
{
    setGroupType(TextGroupType::Scene);
    setLevel(0);
}

StageplayTextModelSceneItem::~StageplayTextModelSceneItem() = default;

QStringRef StageplayTextModelSceneItem::readCustomContent(QXmlStreamReader& _contentReader)
{
    return _contentReader.name();
}

QByteArray StageplayTextModelSceneItem::customContent() const
{
    return {};
}

void StageplayTextModelSceneItem::handleChange()
{
    QString heading;
    QString text;
    int inlineNotesSize = 0;
    QVector<TextModelTextItem::ReviewMark> reviewMarks;

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        const auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Text: {
            auto childTextItem = static_cast<TextModelTextItem*>(child);

            //
            // Собираем текст
            //
            switch (childTextItem->paragraphType()) {
            case TextParagraphType::SceneHeading: {
                heading = TextHelper::smartToUpper(childTextItem->text());
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
                break;
            }
            }

            //
            // Собираем редакторские заметки
            //
            if (!reviewMarks.isEmpty() && !childTextItem->reviewMarks().isEmpty()
                && reviewMarks.constLast().isPartiallyEqual(
                    childTextItem->reviewMarks().constFirst())) {
                reviewMarks.removeLast();
            }
            reviewMarks.append(childTextItem->reviewMarks());
            break;
        }

        default: {
            break;
        }
        }
    }

    setHeading(heading);
    setText(text);
    setInlineNotesSize(inlineNotesSize);
    setReviewMarksSize(std::count_if(
        reviewMarks.begin(), reviewMarks.end(),
        [](const TextModelTextItem::ReviewMark& _reviewMark) { return !_reviewMark.isDone; }));
}

} // namespace BusinessLayer
