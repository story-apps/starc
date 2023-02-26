#include "text_model_folder_item.h"

#include "text_model.h"
#include "text_model_group_item.h"
#include "text_model_splitter_item.h"
#include "text_model_text_item.h"
#include "text_model_xml.h"
#include "text_model_xml_writer.h"

#include <business_layer/templates/text_template.h>
#include <utils/helpers/text_helper.h>

#include <QUuid>
#include <QVariant>
#include <QXmlStreamReader>


namespace BusinessLayer {

class TextModelFolderItem::Implementation
{
public:
    /**
     * @brief Тип папки
     */
    TextFolderType folderType = TextFolderType::Undefined;

    /**
     * @brief Идентификатор папки
     */
    QUuid uuid = QUuid::createUuid();

    /**
     * @brief Цвет папки
     */
    QColor color;

    /**
     * @brief Описание папки
     */
    QString description;

    /**
     * @brief Штамп
     */
    QString stamp;

    //
    // Ридонли свойства, которые формируются по ходу работы с текстом
    //

    /**
     * @brief Название папки
     */
    QString heading;
};


// ****


TextModelFolderItem::TextModelFolderItem(const TextModel* _model)
    : TextModelItem(TextModelItemType::Folder, _model)
    , d(new Implementation)
{
}

TextModelFolderItem::~TextModelFolderItem() = default;

int TextModelFolderItem::subtype() const
{
    return static_cast<int>(folderType());
}

const TextFolderType& TextModelFolderItem::folderType() const
{
    return d->folderType;
}

void TextModelFolderItem::setFolderType(TextFolderType _type)
{
    if (d->folderType == _type) {
        return;
    }

    d->folderType = _type;
    setChanged(true);
}

QColor TextModelFolderItem::color() const
{
    return d->color;
}

void TextModelFolderItem::setColor(const QColor& _color)
{
    if (d->color == _color) {
        return;
    }

    d->color = _color;
    setChanged(true);
}

QString TextModelFolderItem::description() const
{
    return d->description;
}

void TextModelFolderItem::setDescription(const QString& _description)
{
    if (d->description == _description) {
        return;
    }
    d->description = _description;
    setChanged(true);
}

QString TextModelFolderItem::stamp() const
{
    return d->stamp;
}

void TextModelFolderItem::setStamp(const QString& _stamp)
{
    if (d->stamp == _stamp) {
        return;
    }
    d->stamp = _stamp;
    setChanged(true);
}

QString TextModelFolderItem::heading() const
{
    return d->heading;
}

QVariant TextModelFolderItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return !customIcon().isEmpty()
            ? customIcon()
            : (d->folderType == TextFolderType::Act || d->folderType == TextFolderType::Part
                   ? u8"\U000F0253"
                   : u8"\U000f024b");
    }

    case Qt::DisplayRole:
    case FolderHeadingRole: {
        return d->heading;
    }

    case FolderColorRole: {
        return d->color;
    }

    case FolderDescriptionRole: {
        return d->description;
    }

    case FolderStampRole: {
        return d->stamp;
    }

    default: {
        return TextModelItem::data(_role);
    }
    }
}

void TextModelFolderItem::readContent(QXmlStreamReader& _contentReader)
{
    d->folderType = textFolderTypeFromString(_contentReader.name().toString());
    Q_ASSERT(d->folderType != TextFolderType::Undefined);

    if (_contentReader.attributes().hasAttribute(xml::kUuidAttribute)) {
        d->uuid
            = QUuid::fromString(_contentReader.attributes().value(xml::kUuidAttribute).toString());
    }
    auto currentTag = xml::readNextElement(_contentReader); // next

    if (currentTag == xml::kColorTag) {
        d->color = xml::readContent(_contentReader).toString();
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kDescriptionTag) {
        d->description = TextHelper::fromHtmlEscaped(xml::readContent(_contentReader).toString());
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kStampTag) {
        d->stamp = TextHelper::fromHtmlEscaped(xml::readContent(_contentReader).toString());
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
            // Если дошли до конца папки, выходим из обработки
            //
            else if (textFolderTypeFromString(currentTag.toString()) != TextFolderType::Undefined
                     && _contentReader.isEndElement()) {
                xml::readNextElement(_contentReader);
                break;
            }
            //
            // Считываем вложенный контент
            //
            else if (textFolderTypeFromString(currentTag.toString()) != TextFolderType::Undefined) {
                appendItem(model()->createFolderItem(_contentReader));
            } else if (textGroupTypeFromString(currentTag.toString()) != TextGroupType::Undefined) {
                appendItem(model()->createGroupItem(_contentReader));
            } else if (currentTag == xml::kSplitterTag) {
                appendItem(model()->createSplitterItem(_contentReader));
            } else {
                appendItem(model()->createTextItem(_contentReader));
            }
        } while (!_contentReader.atEnd());
    }

    //
    // Определим название
    //
    handleChange();
}

QByteArray TextModelFolderItem::toXml() const
{
    return toXml(nullptr, 0, nullptr, 0, false);
}

QByteArray TextModelFolderItem::toXml(TextModelItem* _from, int _fromPosition, TextModelItem* _to,
                                      int _toPosition, bool _clearUuid) const
{
    auto folderFooterXml = [this] {
        TextModelTextItem item(model());
        item.setParagraphType([type = d->folderType] {
            switch (type) {
            default:
            case TextFolderType::Act: {
                return TextParagraphType::ActFooter;
            }
            case TextFolderType::Sequence: {
                return TextParagraphType::SequenceFooter;
            }
            case TextFolderType::Part: {
                return TextParagraphType::PartFooter;
            }
            case TextFolderType::Chapter: {
                return TextParagraphType::ChapterFooter;
            }
            }
        }());
        return item.toXml();
    };

    xml::TextModelXmlWriter xml;
    xml += xmlHeader(_clearUuid);
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);

        //
        // Нетекстовые блоки, просто добавляем к общему xml
        //
        if (child->type() == TextModelItemType::Splitter) {
            xml += child;
            continue;
        }
        //
        // Папки и сцены проверяем на наличие в них завершающего элемента
        //
        else if (child->type() != TextModelItemType::Text) {
            //
            // Если конечный элемент содержится в дите, то сохраняем его и завершаем формирование
            //
            const bool recursively = true;
            if (child->hasChild(_to, recursively)) {
                if (child->type() == TextModelItemType::Folder) {
                    auto folder = static_cast<TextModelFolderItem*>(child);
                    xml += folder->toXml(_from, _fromPosition, _to, _toPosition, _clearUuid);
                } else if (child->type() == TextModelItemType::Group) {
                    auto group = static_cast<TextModelGroupItem*>(child);
                    xml += group->toXml(_from, _fromPosition, _to, _toPosition, _clearUuid);
                } else {
                    Q_ASSERT(false);
                }

                //
                // Не забываем завершить папку
                //
                xml += folderFooterXml();
                break;
            }
            //
            // В противном случае просто дополняем xml
            //
            else {
                xml += child;
                continue;
            }
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

            //
            // Если папка не была закрыта, добавим корректное завершение для неё
            //
            if (textItem->paragraphType() != TextParagraphType::ActFooter
                && textItem->paragraphType() != TextParagraphType::SequenceFooter
                && textItem->paragraphType() != TextParagraphType::PartFooter
                && textItem->paragraphType() != TextParagraphType::ChapterFooter) {
                xml += folderFooterXml();
            }
            break;
        }
        //
        else if (textItem == _from) {
            xml += { textItem, _fromPosition,
                     static_cast<int>(textItem->text().length()) - _fromPosition };
        } else {
            xml += textItem;
        }
    }
    xml += QString("</%1>\n").arg(xml::kContentTag).toUtf8();
    xml += QString("</%1>\n").arg(toString(d->folderType)).toUtf8();

    return xml.data();
}

QByteArray TextModelFolderItem::xmlHeader(bool _clearUuid) const
{
    QByteArray xml;
    xml += QString("<%1 %2=\"%3\">\n")
               .arg(toString(d->folderType), xml::kUuidAttribute,
                    _clearUuid ? QUuid::createUuid().toString() : d->uuid.toString())
               .toUtf8();
    if (d->color.isValid()) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(xml::kColorTag, d->color.name()).toUtf8();
    }
    if (!d->description.isEmpty()) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n")
                   .arg(xml::kDescriptionTag, TextHelper::toHtmlEscaped(d->description))
                   .toUtf8();
    }
    if (!d->stamp.isEmpty()) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n")
                   .arg(xml::kStampTag, TextHelper::toHtmlEscaped(d->stamp))
                   .toUtf8();
    }
    xml += QString("<%1>\n").arg(xml::kContentTag).toUtf8();

    return xml;
}

void TextModelFolderItem::copyFrom(TextModelItem* _item)
{
    if (_item == nullptr || type() != _item->type() || subtype() != _item->subtype()) {
        Q_ASSERT(false);
        return;
    }

    auto folderItem = static_cast<TextModelFolderItem*>(_item);
    d->uuid = folderItem->d->uuid;
    d->color = folderItem->d->color;
    d->description = folderItem->d->description;
    d->stamp = folderItem->d->stamp;
}

bool TextModelFolderItem::isEqual(TextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type() || subtype() != _item->subtype()) {
        return false;
    }

    const auto folderItem = static_cast<TextModelFolderItem*>(_item);
    return d->uuid == folderItem->d->uuid && d->color == folderItem->d->color
        && d->description == folderItem->d->description && d->stamp == folderItem->d->stamp;
}

void TextModelFolderItem::setHeading(const QString& _heading)
{
    d->heading = _heading;
}

} // namespace BusinessLayer
