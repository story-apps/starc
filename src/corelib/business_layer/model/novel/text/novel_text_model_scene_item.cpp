#include "novel_text_model_scene_item.h"

#include "novel_text_model.h"
#include "novel_text_model_beat_item.h"
#include "novel_text_model_text_item.h"

#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/novel_template.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

class NovelTextModelSceneItem::Implementation
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
     * @brief Количество слов
     */
    int wordsCount = 0;

    /**
     * @brief Количество символов
     */
    QPair<int, int> charactersCount;
};


// ****


NovelTextModelSceneItem::NovelTextModelSceneItem(const NovelTextModel* _model)
    : TextModelGroupItem(_model)
    , d(new Implementation)
{
    setGroupType(TextGroupType::Scene);
}

NovelTextModelSceneItem::~NovelTextModelSceneItem() = default;

int NovelTextModelSceneItem::wordsCount() const
{
    return d->wordsCount;
}

QPair<int, int> NovelTextModelSceneItem::charactersCount() const
{
    return d->charactersCount;
}

QVector<QString> NovelTextModelSceneItem::beats() const
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

QString NovelTextModelSceneItem::description(const QString& _separator) const
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

QVariant NovelTextModelSceneItem::data(int _role) const
{
    switch (_role) {
    case SceneWordCountRole: {
        return d->wordsCount;
    }

    case SceneDescriptionRole: {
        return description();
    }

    default: {
        return TextModelGroupItem::data(_role);
    }
    }
}

void NovelTextModelSceneItem::copyFrom(TextModelItem* _item)
{
    if (_item == nullptr || type() != _item->type() || subtype() != _item->subtype()) {
        Q_ASSERT(false);
        return;
    }

    auto sceneItem = static_cast<NovelTextModelSceneItem*>(_item);
    d->plannedDuration = sceneItem->d->plannedDuration;

    TextModelGroupItem::copyFrom(_item);
}

bool NovelTextModelSceneItem::isEqual(TextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type() || subtype() != _item->subtype()) {
        return false;
    }

    const auto sceneItem = static_cast<NovelTextModelSceneItem*>(_item);
    return TextModelGroupItem::isEqual(_item)
        && d->plannedDuration == sceneItem->d->plannedDuration;
}

bool NovelTextModelSceneItem::isFilterAccepted(const QString& _text, bool _isCaseSensitive,
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

QStringRef NovelTextModelSceneItem::readCustomContent(QXmlStreamReader& _contentReader)
{
    auto currentTag = _contentReader.name();

    if (currentTag == xml::kPlannedDurationTag) {
        d->plannedDuration = xml::readContent(_contentReader).toInt();
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    return currentTag;
}

QByteArray NovelTextModelSceneItem::customContent() const
{
    QByteArray xml;

    if (d->plannedDuration.has_value()) {
        xml += QString("<%1>%2</%1>\n")
                   .arg(xml::kPlannedDurationTag, QString::number(*d->plannedDuration))
                   .toUtf8();
    }

    return xml;
}

void NovelTextModelSceneItem::handleChange()
{
    QString heading;
    QString text;
    int inlineNotesSize = 0;
    int childGroupsReviewMarksSize = 0;
    QVector<TextModelTextItem::ReviewMark> reviewMarks;
    d->wordsCount = 0;
    d->charactersCount = {};

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        const auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Group: {
            auto childGroupItem = static_cast<NovelTextModelBeatItem*>(child);
            text += childGroupItem->text() + " ";
            inlineNotesSize += childGroupItem->inlineNotesSize();
            childGroupsReviewMarksSize += childGroupItem->reviewMarksSize();
            d->wordsCount += childGroupItem->wordsCount();
            d->charactersCount.first += childGroupItem->charactersCount().first;
            d->charactersCount.second += childGroupItem->charactersCount().second;
            break;
        }

        case TextModelItemType::Text: {
            auto childTextItem = static_cast<NovelTextModelTextItem*>(child);

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
