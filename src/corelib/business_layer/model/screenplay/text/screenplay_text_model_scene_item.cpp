#include "screenplay_text_model_scene_item.h"

#include "screenplay_text_model.h"
#include "screenplay_text_model_beat_item.h"
#include "screenplay_text_model_text_item.h"

#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

class ScreenplayTextModelSceneItem::Implementation
{
public:
    /**
     * @brief Запланированная длительность сцены
     */
    std::optional<int> plannedDuration;

    //
    // Ридонли свойства, которые формируются по ходу работы со сценарием
    //

    /**
     * @brief Длительность сцены
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{ 0 };
};


// ****


ScreenplayTextModelSceneItem::ScreenplayTextModelSceneItem(const ScreenplayTextModel* _model)
    : TextModelGroupItem(_model)
    , d(new Implementation)
{
    setGroupType(TextGroupType::Scene);
    setLevel(0);
}

ScreenplayTextModelSceneItem::~ScreenplayTextModelSceneItem() = default;

std::chrono::milliseconds ScreenplayTextModelSceneItem::duration() const
{
    return d->duration;
}

QVector<QString> ScreenplayTextModelSceneItem::beats() const
{
    QVector<QString> beats;
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        if (child->type() != TextModelItemType::Group) {
            continue;
        }

        auto group = static_cast<TextModelGroupItem*>(child);
        if (group->groupType() != TextGroupType::Beat) {
            continue;
        }

        beats.append(group->heading());
    }
    return beats;
}

QVariant ScreenplayTextModelSceneItem::data(int _role) const
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

void ScreenplayTextModelSceneItem::copyFrom(TextModelItem* _item)
{
    if (_item == nullptr || type() != _item->type()) {
        Q_ASSERT(false);
        return;
    }

    auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(_item);
    d->plannedDuration = sceneItem->d->plannedDuration;

    TextModelGroupItem::copyFrom(_item);
}

bool ScreenplayTextModelSceneItem::isEqual(TextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(_item);
    return TextModelGroupItem::isEqual(_item)
        && d->plannedDuration == sceneItem->d->plannedDuration;
}

QStringRef ScreenplayTextModelSceneItem::readCustomContent(QXmlStreamReader& _contentReader)
{
    auto currentTag = _contentReader.name();

    if (currentTag == xml::kPlannedDurationTag) {
        d->plannedDuration = xml::readContent(_contentReader).toInt();
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    return currentTag;
}

QByteArray ScreenplayTextModelSceneItem::customContent() const
{
    QByteArray xml;

    if (d->plannedDuration.has_value()) {
        xml += QString("<%1>%2</%1>\n")
                   .arg(xml::kPlannedDurationTag, QString::number(*d->plannedDuration))
                   .toUtf8();
    }

    return xml;
}

void ScreenplayTextModelSceneItem::handleChange()
{
    QString heading;
    QString text;
    int inlineNotesSize = 0;
    int childGroupsReviewMarksSize = 0;
    QVector<TextModelTextItem::ReviewMark> reviewMarks;
    d->duration = std::chrono::seconds{ 0 };

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        const auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Group: {
            auto childGroupItem = static_cast<ScreenplayTextModelBeatItem*>(child);
            text += childGroupItem->text() + " ";
            inlineNotesSize += childGroupItem->inlineNotesSize();
            childGroupsReviewMarksSize += childGroupItem->reviewMarksSize();
            d->duration += childGroupItem->duration();
            break;
        }

        case TextModelItemType::Text: {
            auto childTextItem = static_cast<ScreenplayTextModelTextItem*>(child);

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
    setReviewMarksSize(childGroupsReviewMarksSize
                       + std::count_if(reviewMarks.begin(), reviewMarks.end(),
                                       [](const TextModelTextItem::ReviewMark& _reviewMark) {
                                           return !_reviewMark.isDone;
                                       }));
}

} // namespace BusinessLayer
