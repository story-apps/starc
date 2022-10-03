#include "text_model_group_item.h"

#include "text_model.h"
#include "text_model_splitter_item.h"
#include "text_model_text_item.h"
#include "text_model_xml.h"
#include "text_model_xml_writer.h"

#include <business_layer/templates/screenplay_template.h>
#include <utils/helpers/text_helper.h>

#include <QLocale>
#include <QUuid>
#include <QVariant>
#include <QXmlStreamReader>


namespace BusinessLayer {

class TextModelGroupItem::Implementation
{
public:
    /**
     * @brief Тип группы
     */
    TextGroupType groupType = TextGroupType::Undefined;

    /**
     * @brief Идентификатор группы
     */
    QUuid uuid = QUuid::createUuid();

    /**
     * @brief Уровень группы (чем больше число, тем ниже уровень)
     */
    int level = 0;

    /**
     * @brief Пропущена ли группа
     */
    bool isOmited = false;

    /**
     * @brief Номер группы
     */
    std::optional<Number> number;

    /**
     * @brief Цвет группы
     */
    QColor color;

    //    /**
    //     * @brief Тэги группы
    //     */
    //    QVector<Tag> tags;

    //    /**
    //     * @brief Сюжетные линии группы
    //     */
    //    QVector<StoryLine> storyLines;

    /**
     * @brief Название группы
     */
    QString title;

    /**
     * @brief Штамп на группе
     */
    QString stamp;

    //
    // Ридонли свойства, которые формируются по ходу работы со сценарием
    //

    /**
     * @brief Заголовок группы
     */
    QString heading;

    /**
     * @brief Текст группы
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
};


// ****


bool TextModelGroupItem::Number::operator==(const TextModelGroupItem::Number& _other) const
{
    return text == _other.text;
}

TextModelGroupItem::TextModelGroupItem(const TextModel* _model)
    : TextModelItem(TextModelItemType::Group, _model)
    , d(new Implementation)
{
}

TextModelGroupItem::~TextModelGroupItem() = default;

int TextModelGroupItem::subtype() const
{
    return static_cast<int>(groupType());
}

const TextGroupType& TextModelGroupItem::groupType() const
{
    return d->groupType;
}

void TextModelGroupItem::setGroupType(TextGroupType _type)
{
    if (d->groupType == _type) {
        return;
    }

    d->groupType = _type;
    setChanged(true);
}

QUuid TextModelGroupItem::uuid() const
{
    return d->uuid;
}

int TextModelGroupItem::level() const
{
    return d->level;
}

void TextModelGroupItem::setLevel(int _level)
{
    if (d->level == _level) {
        return;
    }

    d->level = _level;
    setChanged(true);
}

std::optional<TextModelGroupItem::Number> TextModelGroupItem::number() const
{
    return d->number;
}

bool TextModelGroupItem::setNumber(int _number, const QString& _prefix)
{
    if (childCount() == 0) {
        return false;
    }

    bool hasContent = false;
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        const auto child = childAt(childIndex);
        if (child->type() != TextModelItemType::Text) {
            continue;
        }

        const auto textItemChild = static_cast<const TextModelTextItem*>(child);
        if (!textItemChild->isCorrection()) {
            hasContent = true;
            break;
        }
    }
    if (!hasContent) {
        return false;
    }

    const auto newNumberText = QString("%1%2.").arg(_prefix, QString::number(_number));
    if (d->number.has_value() && d->number->text == newNumberText) {
        return true;
    }

    d->number = { _number, newNumberText };
    //
    // Т.к. пока мы не сохраняем номера, в указании, что произошли изменения нет смысла
    //
    //    setChanged(true);

    return true;
}

QColor TextModelGroupItem::color() const
{
    return d->color;
}

void TextModelGroupItem::setColor(const QColor& _color)
{
    if (d->color == _color) {
        return;
    }

    d->color = _color;
    setChanged(true);
}

QString TextModelGroupItem::title() const
{
    return d->title;
}

void TextModelGroupItem::setTitle(const QString& _title)
{
    if (d->title == _title) {
        return;
    }

    d->title = _title;
    setChanged(true);
}

QString TextModelGroupItem::stamp() const
{
    return d->stamp;
}

void TextModelGroupItem::setStamp(const QString& _stamp)
{
    if (d->stamp == _stamp) {
        return;
    }

    d->stamp = _stamp;
    setChanged(true);
}

QString TextModelGroupItem::heading() const
{
    return d->heading;
}

QString TextModelGroupItem::text() const
{
    return d->text;
}

int TextModelGroupItem::length() const
{
    return heading().length() + text().length() + (childCount() > 1 ? 1 : 0);
}

int TextModelGroupItem::inlineNotesSize() const
{
    return d->inlineNotesSize;
}

int TextModelGroupItem::reviewMarksSize() const
{
    return d->reviewMarksSize;
}

QVariant TextModelGroupItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return u8"\U000f021a";
    }

    case GroupTypeRole: {
        return static_cast<int>(d->groupType);
    }

    case GroupNumberRole: {
        if (d->number.has_value()) {
            return d->number->text;
        }
        return {};
    }

    case GroupColorRole: {
        return d->color;
    }

    case GroupStampRole: {
        return d->stamp;
    }

    case GroupHeadingRole: {
        return d->title.isEmpty() ? d->heading : d->title;
    }

    case GroupTextRole: {
        return d->text;
    }

    case GroupInlineNotesSizeRole: {
        return d->inlineNotesSize;
    }

    case GroupReviewMarksSizeRole: {
        return d->reviewMarksSize;
    }

    default: {
        return TextModelItem::data(_role);
    }
    }
}

void TextModelGroupItem::readContent(QXmlStreamReader& _contentReader)
{
    d->groupType = textGroupTypeFromString(_contentReader.name().toString());
    Q_ASSERT(d->groupType != TextGroupType::Undefined);

    const auto attributes = _contentReader.attributes();
    if (attributes.hasAttribute(xml::kUuidAttribute)) {
        d->uuid = QUuid::fromString(attributes.value(xml::kUuidAttribute).toString());
    }

    //
    // TODO: plots
    //
    d->isOmited = attributes.hasAttribute(xml::kOmitedAttribute);
    xml::readNextElement(_contentReader);

    auto currentTag = _contentReader.name();
    if (currentTag == xml::kNumberTag) {
        //        d->number = {
        //        _contentReader.attributes().value(xml::kNumberValueAttribute).toString() };
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kColorTag) {
        d->color = xml::readContent(_contentReader).toString();
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kTitleTag) {
        d->title = TextHelper::fromHtmlEscaped(xml::readContent(_contentReader).toString());
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kStampTag) {
        d->stamp = TextHelper::fromHtmlEscaped(xml::readContent(_contentReader).toString());
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    currentTag = readCustomContent(_contentReader);

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
            // Если дошли до конца группы, выходим из обработки
            //
            else if (textGroupTypeFromString(currentTag.toString()) != TextGroupType::Undefined
                     && _contentReader.isEndElement()) {
                xml::readNextElement(_contentReader);
                break;
            }
            //
            // Считываем вложенный контент
            //
            else if (textGroupTypeFromString(currentTag.toString()) != TextGroupType::Undefined) {
                auto item = model()->createGroupItem(_contentReader);
                appendItem(item);
            } else if (currentTag == xml::kSplitterTag) {
                auto item = model()->createSplitterItem(_contentReader);
                appendItem(item);
            } else {
                auto item = model()->createTextItem(_contentReader);
                appendItem(item);
            }
        } while (!_contentReader.atEnd());
    }

    //
    // Соберём заголовок, текст группы и прочие параметры
    //
    handleChange();
}

QByteArray TextModelGroupItem::toXml() const
{
    return toXml(nullptr, 0, nullptr, 0, false);
}

QByteArray TextModelGroupItem::toXml(TextModelItem* _from, int _fromPosition, TextModelItem* _to,
                                     int _toPosition, bool _clearUuid) const
{
    xml::TextModelXmlWriter xml;
    xml += xmlHeader(_clearUuid);
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);

        //
        // Нетекстовые блоки, просто добавляем к общему xml
        //
        if (child->type() != TextModelItemType::Text) {
            xml += child;
            continue;
        }

        //
        // Текстовые блоки, в зависимости от необходимости вставить блок целиком, или его часть
        //
        auto textItem = static_cast<TextModelTextItem*>(child);
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
    xml += QString("</%1>\n").arg(toString(d->groupType)).toUtf8();

    return xml.data();
}

QByteArray TextModelGroupItem::xmlHeader(bool _clearUuid) const
{
    QByteArray xml;
    //
    // TODO: plots
    //
    xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6>\n")
               .arg(toString(d->groupType), xml::kUuidAttribute,
                    _clearUuid ? QUuid::createUuid().toString() : d->uuid.toString(),
                    xml::kPlotsAttribute, QString(),
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
    if (!d->title.isEmpty()) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n")
                   .arg(xml::kTitleTag, TextHelper::toHtmlEscaped(d->title))
                   .toUtf8();
    }
    if (!d->stamp.isEmpty()) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n")
                   .arg(xml::kStampTag, TextHelper::toHtmlEscaped(d->stamp))
                   .toUtf8();
    }
    xml += customContent();
    xml += QString("<%1>\n").arg(xml::kContentTag).toUtf8();

    return xml;
}

void TextModelGroupItem::copyFrom(TextModelItem* _item)
{
    if (_item == nullptr || type() != _item->type()) {
        Q_ASSERT(false);
        return;
    }

    auto sceneItem = static_cast<TextModelGroupItem*>(_item);
    d->uuid = sceneItem->d->uuid;
    d->isOmited = sceneItem->d->isOmited;
    d->number = sceneItem->d->number;
    d->color = sceneItem->d->color;
    d->title = sceneItem->title();
    d->stamp = sceneItem->d->stamp;
}

bool TextModelGroupItem::isEqual(TextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto sceneItem = static_cast<TextModelGroupItem*>(_item);
    return d->uuid == sceneItem->d->uuid
        && d->isOmited == sceneItem->d->isOmited
        //
        // TODO: тут нужно сравнивать, только когда номера зафиксированы
        //
        //            && d->number == sceneItem->d->number
        && d->color == sceneItem->d->color && d->title == sceneItem->d->title
        && d->stamp == sceneItem->d->stamp;
}

void TextModelGroupItem::setHeading(const QString& _heading)
{
    d->heading = _heading;
}

void TextModelGroupItem::setText(const QString& _text)
{
    d->text = _text;
}

void TextModelGroupItem::setInlineNotesSize(int _size)
{
    d->inlineNotesSize = _size;
}

void TextModelGroupItem::setReviewMarksSize(int _size)
{
    d->reviewMarksSize = _size;
}

} // namespace BusinessLayer
