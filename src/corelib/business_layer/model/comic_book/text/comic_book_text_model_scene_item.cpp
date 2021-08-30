#include "comic_book_text_model_scene_item.h"

#include "comic_book_text_model_splitter_item.h"
#include "comic_book_text_model_text_item.h"
#include "comic_book_text_model_xml.h"
#include "comic_book_text_model_xml_writer.h"

#include <business_layer/templates/screenplay_template.h>
#include <utils/helpers/text_helper.h>

#include <QLocale>
#include <QUuid>
#include <QVariant>
#include <QXmlStreamReader>

#include <optional>


namespace BusinessLayer {

class ComicBookTextModelSceneItem::Implementation
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
     * @brief Цвет сцены
     */
    QColor color;

    //    /**
    //     * @brief Тэги сцены
    //     */
    //    QVector<Tag> tags;

    //    /**
    //     * @brief Сюжетные линии сцены
    //     */
    //    QVector<StoryLine> storyLines;

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


bool ComicBookTextModelSceneItem::Number::operator==(
    const ComicBookTextModelSceneItem::Number& _other) const
{
    return value == _other.value;
}

ComicBookTextModelSceneItem::ComicBookTextModelSceneItem()
    : ComicBookTextModelItem(ComicBookTextModelItemType::Scene)
    , d(new Implementation)
{
    d->uuid = QUuid::createUuid();
}

ComicBookTextModelSceneItem::ComicBookTextModelSceneItem(QXmlStreamReader& _contentReader)
    : ComicBookTextModelItem(ComicBookTextModelItemType::Scene)
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

    if (currentTag == xml::kColorTag) {
        d->color = xml::readContent(_contentReader).toString();
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
                appendItem(new ComicBookTextModelSceneItem(_contentReader));
            } else if (currentTag == xml::kSplitterTag) {
                appendItem(new ComicBookTextModelSplitterItem(_contentReader));
            } else {
                appendItem(new ComicBookTextModelTextItem(_contentReader));
            }
        } while (!_contentReader.atEnd());
    }

    //
    // Соберём заголовок, текст сцены и прочие параметры
    //
    handleChange();
}

ComicBookTextModelSceneItem::~ComicBookTextModelSceneItem() = default;

ComicBookTextModelSceneItem::Number ComicBookTextModelSceneItem::number() const
{
    if (!d->number.has_value()) {
        return {};
    }

    return *d->number;
}

bool ComicBookTextModelSceneItem::setNumber(int _number, const QString& _prefix)
{
    if (childCount() == 0) {
        return false;
    }

    bool hasContent = false;
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        const auto child = childAt(childIndex);
        if (child->type() != ComicBookTextModelItemType::Text) {
            continue;
        }

        const auto textItemChild = static_cast<const ComicBookTextModelTextItem*>(child);
        if (!textItemChild->isCorrection()) {
            hasContent = true;
            break;
        }
    }
    if (!hasContent) {
        return false;
    }

    const auto newNumber = QString(QLocale().textDirection() == Qt::LeftToRight ? "%1%2." : ".%2%1")
                               .arg(_prefix, QString::number(_number));
    if (d->number.has_value() && d->number->value == newNumber) {
        return true;
    }

    d->number = { newNumber };
    //
    // Т.к. пока мы не сохраняем номера, в указании, что произошли изменения нет смысла
    //
    //    setChanged(true);

    return true;
}

QColor ComicBookTextModelSceneItem::color() const
{
    return d->color;
}

void ComicBookTextModelSceneItem::setColor(const QColor& _color)
{
    if (d->color == _color) {
        return;
    }

    d->color = _color;
    setChanged(true);
}

std::chrono::milliseconds ComicBookTextModelSceneItem::duration() const
{
    return d->duration;
}

QVariant ComicBookTextModelSceneItem::data(int _role) const
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

    case SceneColorRole: {
        return d->color;
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
        return ComicBookTextModelItem::data(_role);
    }
    }
}

QByteArray ComicBookTextModelSceneItem::toXml() const
{
    return toXml(nullptr, 0, nullptr, 0, false);
}

QByteArray ComicBookTextModelSceneItem::toXml(ComicBookTextModelItem* _from, int _fromPosition,
                                              ComicBookTextModelItem* _to, int _toPosition,
                                              bool _clearUuid) const
{
    xml::ComicBookTextModelXmlWriter xml;
    xml += xmlHeader(_clearUuid);
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);

        //
        // Нетекстовые блоки, просто добавляем к общему xml
        //
        if (child->type() != ComicBookTextModelItemType::Text) {
            xml += child;
            continue;
        }

        //
        // Текстовые блоки, в зависимости от необходимости вставить блок целиком, или его часть
        //
        auto textItem = static_cast<ComicBookTextModelTextItem*>(child);
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

QByteArray ComicBookTextModelSceneItem::xmlHeader(bool _clearUuid) const
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
    if (d->color.isValid()) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(xml::kColorTag, d->color.name()).toUtf8();
    }
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

void ComicBookTextModelSceneItem::copyFrom(ComicBookTextModelItem* _item)
{
    if (_item->type() != ComicBookTextModelItemType::Scene) {
        Q_ASSERT(false);
        return;
    }

    auto sceneItem = static_cast<ComicBookTextModelSceneItem*>(_item);
    d->uuid = sceneItem->d->uuid;
    d->isOmited = sceneItem->d->isOmited;
    d->number = sceneItem->d->number;
    d->color = sceneItem->d->color;
    d->stamp = sceneItem->d->stamp;
    d->plannedDuration = sceneItem->d->plannedDuration;
}

bool ComicBookTextModelSceneItem::isEqual(ComicBookTextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto sceneItem = static_cast<ComicBookTextModelSceneItem*>(_item);
    return d->uuid == sceneItem->d->uuid
        && d->isOmited == sceneItem->d->isOmited
        //
        // TODO: тут нужно сравнивать, только когда номера зафиксированы
        //
        //            && d->number == sceneItem->d->number
        && d->color == sceneItem->d->color && d->stamp == sceneItem->d->stamp
        && d->plannedDuration == sceneItem->d->plannedDuration;
}

void ComicBookTextModelSceneItem::handleChange()
{
    d->heading.clear();
    d->text.clear();
    d->inlineNotesSize = 0;
    d->reviewMarksSize = 0;
    d->duration = std::chrono::seconds{ 0 };

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        if (child->type() != ComicBookTextModelItemType::Text) {
            continue;
        }

        auto childTextItem = static_cast<ComicBookTextModelTextItem*>(child);

        //
        // Собираем текст
        //
        switch (childTextItem->paragraphType()) {
            //        case ComicBookParagraphType::SceneHeading: {
            //            d->heading = TextHelper::smartToUpper(childTextItem->text());
            //            break;
            //        }

            //        case ComicBookParagraphType::InlineNote: {
            //            ++d->inlineNotesSize;
            //            break;
            //        }

        default: {
            d->text.append(childTextItem->text() + " ");
            d->reviewMarksSize += std::count_if(
                childTextItem->reviewMarks().begin(), childTextItem->reviewMarks().end(),
                [](const ComicBookTextModelTextItem::ReviewMark& _reviewMark) {
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
