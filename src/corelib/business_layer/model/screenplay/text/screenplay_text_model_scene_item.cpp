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

    /**
     * @brief Ресурсы сцены
     */
    QVector<BreakdownSceneResource> resources;

    //
    // Ридонли свойства, которые формируются по ходу работы со сценарием
    //

    /**
     * @brief Количество слов
     */
    int wordsCount = 0;

    /**
     * @brief Количество символов
     */
    QPair<int, int> charactersCount;

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
}

ScreenplayTextModelSceneItem::~ScreenplayTextModelSceneItem() = default;

QVector<BreakdownSceneResource> ScreenplayTextModelSceneItem::resources() const
{
    return d->resources;
}

void ScreenplayTextModelSceneItem::storeResource(const QUuid& _uuid, int _qty,
                                                 const QString& _descriptionForScene)
{
    bool updated = false;
    for (auto& resource : d->resources) {
        if (resource.uuid == _uuid) {
            resource.qty = _qty;
            resource.description = _descriptionForScene;
            updated = true;
            break;
        }
    }
    if (!updated) {
        d->resources.append({ _uuid, _qty, _descriptionForScene });
    }

    setChanged(true);
}

void ScreenplayTextModelSceneItem::removeResource(const QUuid& _uuid)
{
    for (int index = 0; index < d->resources.size(); ++index) {
        if (d->resources.at(index).uuid != _uuid) {
            continue;
        }

        //
        // Удаляем привязку ресурса к тексту, если таковая имеется
        //
        std::function<void(BusinessLayer::TextModelItem*)> removeResourcesFromText;
        removeResourcesFromText = [&removeResourcesFromText,
                                   _uuid](BusinessLayer::TextModelItem* _groupItem) {
            for (int childIndex = 0; childIndex < _groupItem->childCount(); ++childIndex) {
                auto childItem = _groupItem->childAt(childIndex);
                if (childItem->type() != BusinessLayer::TextModelItemType::Text) {
                    removeResourcesFromText(childItem);
                    return;
                }

                auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(childItem);
                auto resources = textItem->resourceMarks();
                for (int resourceIndex = 0; resourceIndex < resources.size(); ++resourceIndex) {
                    if (resources.at(resourceIndex).uuid == _uuid) {
                        resources.removeAt(resourceIndex);
                        --resourceIndex;
                    }
                }
                if (textItem->resourceMarks().size() > resources.size()) {
                    textItem->setResourceMarks(resources);
                }
            }
        };
        removeResourcesFromText(this);

        d->resources.removeAt(index);
        setChanged(true);
        return;
    }
}

int ScreenplayTextModelSceneItem::wordsCount() const
{
    return d->wordsCount;
}

QPair<int, int> ScreenplayTextModelSceneItem::charactersCount() const
{
    return d->charactersCount;
}

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

QString ScreenplayTextModelSceneItem::description(const QString& _separator) const
{
    QString description;
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        if (child->type() != TextModelItemType::Group) {
            continue;
        }

        auto group = static_cast<TextModelGroupItem*>(child);
        if (group->groupType() != TextGroupType::Beat || group->heading().isEmpty()) {
            continue;
        }

        if (!description.isEmpty()) {
            description.append(_separator);
        }
        description.append(group->heading());
    }
    return description;
}

QVariant ScreenplayTextModelSceneItem::data(int _role) const
{
    switch (_role) {
    case SceneDurationRole: {
        const int duration = std::chrono::duration_cast<std::chrono::seconds>(d->duration).count();
        return duration;
    }

    case SceneDescriptionRole: {
        return description();
    }

    default: {
        return TextModelGroupItem::data(_role);
    }
    }
}

void ScreenplayTextModelSceneItem::copyFrom(TextModelItem* _item)
{
    if (_item == nullptr || type() != _item->type() || subtype() != _item->subtype()) {
        Q_ASSERT(false);
        return;
    }

    auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(_item);
    d->plannedDuration = sceneItem->d->plannedDuration;

    TextModelGroupItem::copyFrom(_item);
}

bool ScreenplayTextModelSceneItem::isEqual(TextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type() || subtype() != _item->subtype()) {
        return false;
    }

    const auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(_item);
    return TextModelGroupItem::isEqual(_item)
        && d->plannedDuration == sceneItem->d->plannedDuration;
}

bool ScreenplayTextModelSceneItem::isFilterAccepted(const QString& _text, bool _isCaseSensitive,
                                                    int _filterType) const
{
    auto contains = [text = _text, cs = _isCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive](
                        const QString& _text) { return _text.contains(text, cs); };
    auto tagsString = [tags = this->tags()] {
        return std::accumulate(tags.begin(), tags.end(), QString(),
                               [](const QString& _lhs, const QPair<QString, QColor>& _rhs) {
                                   return _lhs + _rhs.first + " ";
                               });
    };
    switch (_filterType) {
    default:
    case 0: {
        return contains(title()) || contains(heading()) || contains(text()) || contains(stamp())
            || contains(tagsString());
    }

    case 1: {
        return contains(title()) || contains(heading());
    }

    case 2: {
        return contains(text());
    }

    case 3: {
        return contains(tagsString());
    }
    }
}

QStringRef ScreenplayTextModelSceneItem::readCustomContent(QXmlStreamReader& _contentReader)
{
    auto currentTag = _contentReader.name();

    if (currentTag == xml::kPlannedDurationTag) {
        d->plannedDuration = xml::readContent(_contentReader).toInt();
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kResourcesTag) {
        d->resources.clear();
        currentTag = xml::readNextElement(_contentReader); // next
        while (currentTag == xml::kResourceTag) {
            const QUuid resourceUuid
                = _contentReader.attributes().value(xml::kUuidAttribute).toString();
            const int resourceQty = _contentReader.attributes().value(xml::kQtyAttribute).toInt();
            const auto resourceDescription
                = TextHelper::fromHtmlEscaped(xml::readContent(_contentReader).toString());
            d->resources.append({ resourceUuid, resourceQty, resourceDescription });

            xml::readNextElement(_contentReader); // end
            currentTag = xml::readNextElement(_contentReader); // next resource or resources end
        }
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

    if (!d->resources.isEmpty()) {
        xml += QString("<%1>\n").arg(xml::kResourcesTag).toUtf8();
        for (const auto& resource : std::as_const(d->resources)) {
            xml += QString("<%1 %2=\"%3\" %4=\"%5\"><![CDATA[%6]]></%1>\n")
                       .arg(xml::kResourceTag, xml::kUuidAttribute, resource.uuid.toString(),
                            xml::kQtyAttribute, QString::number(resource.qty),
                            TextHelper::toHtmlEscaped(resource.description))
                       .toUtf8();
        }
        xml += QString("</%1>\n").arg(xml::kResourcesTag).toUtf8();
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
    d->wordsCount = 0;
    d->charactersCount = {};
    d->duration = std::chrono::seconds{ 0 };

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        const auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Group: {
            auto childGroupItem = static_cast<ScreenplayTextModelBeatItem*>(child);
            text += childGroupItem->text() + " ";
            inlineNotesSize += childGroupItem->inlineNotesSize();
            childGroupsReviewMarksSize += childGroupItem->reviewMarksSize();
            d->wordsCount += childGroupItem->wordsCount();
            d->charactersCount.first += childGroupItem->charactersCount().first;
            d->charactersCount.second += childGroupItem->charactersCount().second;
            d->duration += childGroupItem->duration();
            break;
        }

        case TextModelItemType::Text: {
            auto childTextItem = static_cast<ScreenplayTextModelTextItem*>(child);

            //
            // Собираем текст
            //
            QString childTextItemText = childTextItem->text();
            switch (childTextItem->paragraphType()) {
            case TextParagraphType::SceneHeading: {
                heading = childTextItemText;
                break;
            }

            case TextParagraphType::InlineNote: {
                ++inlineNotesSize;
                break;
            }

            case TextParagraphType::SceneCharacters:
            case TextParagraphType::Character:
            case TextParagraphType::Shot:
            case TextParagraphType::Transition:
            default: {
                if (!text.isEmpty() && !childTextItemText.isEmpty()) {
                    text.append(" ");
                }

                text.append(childTextItemText);
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
            // Собираем счётчики
            //
            d->wordsCount += childTextItem->wordsCount();
            d->charactersCount.first += childTextItem->charactersCount().first;
            d->charactersCount.second += childTextItem->charactersCount().second;
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
