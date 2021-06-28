#include "screenplay_text_model_scene_item.h"

#include "screenplay_text_model_splitter_item.h"
#include "screenplay_text_model_text_item.h"
#include "screenplay_text_model_xml.h"
#include "screenplay_text_model_xml_writer.h"

#include <business_layer/templates/screenplay_template.h>
#include <utils/helpers/text_helper.h>

#include <QLocale>
#include <QUuid>
#include <QVariant>
#include <QXmlStreamReader>

#include <optional>


namespace BusinessLayer {

class ScreenplayTextModelSceneItem::Implementation
{
public:
    /**
     * @brief Идентификатор сцены
     */
    QUuid uuid;

    /**
     * @brief Пропущена ли сцена
     */
    bool isOmited = false;

    /**
     * @brief Номер сцены
     */
    std::optional<Number> number;

    /**
     * @brief Штамп на сцене
     */
    QString stamp;

    /**
     * @brief Запланированная длительность сцены
     */
    std::optional<int> plannedDuration;

    //
    // Ридонли свойства, которые формируются по ходу работы со сценарием
    //

    /**
     * @brief Заголовок сцены
     */
    QString heading;

    /**
     * @brief Текст сцены
     */
    QString text;

    /**
     * @brief Количество заметок по тексту
     */
    int inlineNotesSize = 0;

    /**
     * @brief Количество редакторских заметок
     */
    int reviewMarksSize = 0;

    /**
     * @brief Длительность сцены
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{ 0 };
};


// ****


bool ScreenplayTextModelSceneItem::Number::operator==(
    const ScreenplayTextModelSceneItem::Number& _other) const
{
    return value == _other.value;
}

ScreenplayTextModelSceneItem::ScreenplayTextModelSceneItem()
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Scene)
    , d(new Implementation)
{
    d->uuid = QUuid::createUuid();
}

ScreenplayTextModelSceneItem::ScreenplayTextModelSceneItem(QXmlStreamReader& _contentReader)
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Scene)
    , d(new Implementation)
{
    Q_ASSERT(_contentReader.name() == xml::kSceneTag);

    const auto attributes = _contentReader.attributes();
    if (attributes.hasAttribute(xml::kUuidAttribute)) {
        d->uuid = attributes.value(xml::kUuidAttribute).toString();
    }

    //
    // TODO: plots
    //
    d->isOmited = attributes.hasAttribute(xml::kOmitedAttribute);
    xml::readNextElement(_contentReader);

    auto currentTag = _contentReader.name();
    if (currentTag == xml::kNumberTag) {
        d->number = { _contentReader.attributes().value(xml::kNumberValueAttribute).toString() };
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kStampTag) {
        d->stamp = TextHelper::fromHtmlEscaped(xml::readContent(_contentReader).toString());
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kPlannedDurationTag) {
        d->plannedDuration = xml::readContent(_contentReader).toInt();
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kContentTag) {
        xml::readNextElement(_contentReader); // next item
        do {
            currentTag = _contentReader.name();

            //
            // Проглатываем закрывающий контентный тэг
            //
            if (currentTag == xml::kContentTag && _contentReader.isEndElement()) {
                xml::readNextElement(_contentReader);
                continue;
            }
            //
            // Если дошли до конца сцены, выходим из обработки
            //
            else if (currentTag == xml::kSceneTag && _contentReader.isEndElement()) {
                xml::readNextElement(_contentReader);
                break;
            }
            //
            // Считываем вложенный контент
            //
            else if (currentTag == xml::kSceneTag) {
                appendItem(new ScreenplayTextModelSceneItem(_contentReader));
            } else if (currentTag == xml::kSplitterTag) {
                appendItem(new ScreenplayTextModelSplitterItem(_contentReader));
            } else {
                appendItem(new ScreenplayTextModelTextItem(_contentReader));
            }
        } while (!_contentReader.atEnd());
    }

    //
    // Соберём заголовок, текст сцены и прочие параметры
    //
    handleChange();
}

ScreenplayTextModelSceneItem::~ScreenplayTextModelSceneItem() = default;

ScreenplayTextModelSceneItem::Number ScreenplayTextModelSceneItem::number() const
{
    if (!d->number.has_value()) {
        return {};
    }

    return *d->number;
}

void ScreenplayTextModelSceneItem::setNumber(int _number, const QString& _prefix)
{
    const auto newNumber = QString(QLocale().textDirection() == Qt::LeftToRight ? "%1%2." : ".%2%1")
                               .arg(_prefix, QString::number(_number));
    if (d->number.has_value() && d->number->value == newNumber) {
        return;
    }

    d->number = { newNumber };
    //
    // Т.к. пока мы не сохраняем номера, в указании, что произошли изменения нет смысла
    //
    //    setChanged(true);
}

std::chrono::milliseconds ScreenplayTextModelSceneItem::duration() const
{
    return d->duration;
}

QVariant ScreenplayTextModelSceneItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return u8"\U000f021a";
    }

    case SceneNumberRole: {
        if (d->number.has_value()) {
            return d->number->value;
        }
        return {};
    }

    case SceneHeadingRole: {
        return d->heading;
    }

    case SceneTextRole: {
        return d->text;
    }

    case SceneInlineNotesSizeRole: {
        return d->inlineNotesSize;
    }

    case SceneReviewMarksSizeRole: {
        return d->reviewMarksSize;
    }

    case SceneDurationRole: {
        const int duration = std::chrono::duration_cast<std::chrono::seconds>(d->duration).count();
        return duration;
    }

    default: {
        return ScreenplayTextModelItem::data(_role);
    }
    }
}

QByteArray ScreenplayTextModelSceneItem::toXml() const
{
    return toXml(nullptr, 0, nullptr, 0, false);
}

QByteArray ScreenplayTextModelSceneItem::toXml(ScreenplayTextModelItem* _from, int _fromPosition,
                                               ScreenplayTextModelItem* _to, int _toPosition,
                                               bool _clearUuid) const
{
    xml::ScreenplayTextModelXmlWriter xml;
    xml += xmlHeader(_clearUuid);
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);

        //
        // Нетекстовые блоки, просто добавляем к общему xml
        //
        if (child->type() != ScreenplayTextModelItemType::Text) {
            xml += child;
            continue;
        }

        //
        // Текстовые блоки, в зависимости от необходимости вставить блок целиком, или его часть
        //
        auto textItem = static_cast<ScreenplayTextModelTextItem*>(child);
        if (textItem == _to) {
            if (textItem == _from) {
                xml += { textItem, _fromPosition, _toPosition - _fromPosition };
            } else {
                xml += { textItem, 0, _toPosition };
            }
            break;
        }
        //
        else if (textItem == _from) {
            xml += { textItem, _fromPosition, textItem->text().length() - _fromPosition };
        } else {
            xml += textItem;
        }
    }
    xml += QString("</%1>\n").arg(xml::kContentTag).toUtf8();
    xml += QString("</%1>\n").arg(xml::kSceneTag).toUtf8();

    return xml.data();
}

QByteArray ScreenplayTextModelSceneItem::xmlHeader(bool _clearUuid) const
{
    QByteArray xml;
    //
    // TODO: plots
    //
    xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6>\n")
               .arg(xml::kSceneTag, xml::kUuidAttribute,
                    _clearUuid ? QUuid::createUuid().toString() : d->uuid.toString(),
                    xml::kPlotsAttribute, {},
                    (d->isOmited ? QString("%1=\"true\"").arg(xml::kOmitedAttribute) : ""))
               .toUtf8();
    //
    // TODO: Номера будем сохранять только когда они кастомные или фиксированные
    //
    //    if (d->number.has_value()) {
    //        xml += QString("<%1 %2=\"%3\"/>\n")
    //               .arg(xml::kNumberTag, xml::kNumberValueAttribute, d->number->value).toUtf8();
    //    }
    if (!d->stamp.isEmpty()) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n")
                   .arg(xml::kStampTag, TextHelper::toHtmlEscaped(d->stamp))
                   .toUtf8();
    }
    if (d->plannedDuration.has_value()) {
        xml += QString("<%1>%2</%1>\n")
                   .arg(xml::kPlannedDurationTag, QString::number(*d->plannedDuration))
                   .toUtf8();
    }
    xml += QString("<%1>\n").arg(xml::kContentTag).toUtf8();

    return xml;
}

void ScreenplayTextModelSceneItem::copyFrom(ScreenplayTextModelItem* _item)
{
    if (_item->type() != ScreenplayTextModelItemType::Scene) {
        Q_ASSERT(false);
        return;
    }

    auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(_item);
    d->uuid = sceneItem->d->uuid;
    d->isOmited = sceneItem->d->isOmited;
    d->number = sceneItem->d->number;
    d->stamp = sceneItem->d->stamp;
    d->plannedDuration = sceneItem->d->plannedDuration;
}

bool ScreenplayTextModelSceneItem::isEqual(ScreenplayTextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(_item);
    return d->uuid == sceneItem->d->uuid
        && d->isOmited == sceneItem->d->isOmited
        //
        // TODO: тут нужно сравнивать, только когда номера зафиксированы
        //
        //            && d->number == sceneItem->d->number
        && d->stamp == sceneItem->d->stamp && d->plannedDuration == sceneItem->d->plannedDuration;
}

void ScreenplayTextModelSceneItem::handleChange()
{
    d->heading.clear();
    d->text.clear();
    d->inlineNotesSize = 0;
    d->reviewMarksSize = 0;
    d->duration = std::chrono::seconds{ 0 };

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        if (child->type() != ScreenplayTextModelItemType::Text) {
            continue;
        }

        auto childTextItem = static_cast<ScreenplayTextModelTextItem*>(child);

        //
        // Собираем текст
        //
        switch (childTextItem->paragraphType()) {
        case ScreenplayParagraphType::SceneHeading: {
            d->heading = TextHelper::smartToUpper(childTextItem->text());
            break;
        }

        case ScreenplayParagraphType::InlineNote: {
            ++d->inlineNotesSize;
            break;
        }

        default: {
            d->text.append(childTextItem->text() + " ");
            d->reviewMarksSize += std::count_if(
                childTextItem->reviewMarks().begin(), childTextItem->reviewMarks().end(),
                [](const ScreenplayTextModelTextItem::ReviewMark& _reviewMark) {
                    return !_reviewMark.isDone;
                });
            break;
        }
        }

        //
        // Собираем хронометраж
        //
        d->duration += childTextItem->duration();
    }
}

} // namespace BusinessLayer
