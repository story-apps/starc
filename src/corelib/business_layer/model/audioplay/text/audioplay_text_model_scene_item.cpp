#include "audioplay_text_model_scene_item.h"

#include "audioplay_text_model.h"
#include "audioplay_text_model_text_item.h"

#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/audioplay_template.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

class AudioplayTextModelSceneItem::Implementation
{
public:
    //
    // Ридонли свойства, которые формируются по ходу работы со сценарием
    //

    /**
     * @brief Длительность сцены
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{ 0 };
};


// ****


AudioplayTextModelSceneItem::AudioplayTextModelSceneItem(const AudioplayTextModel* _model)
    : TextModelGroupItem(_model)
    , d(new Implementation)
{
    setGroupType(TextGroupType::Scene);
    setLevel(0);
}

AudioplayTextModelSceneItem::~AudioplayTextModelSceneItem() = default;

std::chrono::milliseconds AudioplayTextModelSceneItem::duration() const
{
    return d->duration;
}

QVariant AudioplayTextModelSceneItem::data(int _role) const
{
    switch (_role) {
    case SceneDurationRole: {
        const int duration = std::chrono::duration_cast<std::chrono::seconds>(d->duration).count();
        return duration;
    }

    default: {
        return TextModelGroupItem::data(_role);
    }
    }
}

QStringRef AudioplayTextModelSceneItem::readCustomContent(QXmlStreamReader& _contentReader)
{
    return _contentReader.name();
}

QByteArray AudioplayTextModelSceneItem::customContent() const
{
    return {};
}

void AudioplayTextModelSceneItem::handleChange()
{
    QString heading;
    QString text;
    int inlineNotesSize = 0;
    int reviewMarksSize = 0;
    d->duration = std::chrono::seconds{ 0 };

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        const auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Text: {
            auto childTextItem = static_cast<AudioplayTextModelTextItem*>(child);

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
                text.append(childTextItem->text() + " ");
                reviewMarksSize += std::count_if(
                    childTextItem->reviewMarks().begin(), childTextItem->reviewMarks().end(),
                    [](const AudioplayTextModelTextItem::ReviewMark& _reviewMark) {
                        return !_reviewMark.isDone;
                    });
                break;
            }
            }

            //
            // Собираем хронометраж
            //
            d->duration += childTextItem->duration();
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
    setReviewMarksSize(reviewMarksSize);
}

} // namespace BusinessLayer
