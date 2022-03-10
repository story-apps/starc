#include "abstract_text_model_folder_item.h"

#include "abstract_text_model_group_item.h"
#include "abstract_text_model_splitter_item.h"
#include "abstract_text_model_text_item.h"
#include "abstract_text_model_xml.h"
#include "abstract_text_model_xml_writer.h"

#include <business_layer/templates/text_template.h>
#include <utils/helpers/text_helper.h>

#include <QUuid>
#include <QVariant>
#include <QXmlStreamReader>


namespace BusinessLayer {

class AbstractTextModelFolderItem::Implementation
{
public:
    /**
     * @brief Тип папки
     */
    TextFolderType groupType = TextFolderType::Undefined;

    /**
     * @brief Идентификатор папки
     */
    QUuid uuid;

    /**
     * @brief Цвет папки
     */
    QColor color;

    //
    // Ридонли свойства, которые формируются по ходу работы с текстом
    //

    /**
     * @brief Название папки
     */
    QString name;
};


// ****


AbstractTextModelFolderItem::AbstractTextModelFolderItem(const AbstractTextModel* _model)
    : AbstractTextModelItem(AbstractTextModelItemType::Folder, _model)
    , d(new Implementation)
{
    d->uuid = QUuid::createUuid();
}

AbstractTextModelFolderItem::AbstractTextModelFolderItem(const AbstractTextModel* _model,
                                                         QXmlStreamReader& _contentReader)
    : AbstractTextModelItem(AbstractTextModelItemType::Folder, _model)
    , d(new Implementation)
{
    d->groupType = textFolderTypeFromString(_contentReader.name().toString());
    Q_ASSERT(d->groupType != TextFolderType::Undefined);

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
            else if (textFolderTypeFromString(currentTag.toString())
                         != TextFolderType::Undefined
                     && _contentReader.isEndElement()) {
                xml::readNextElement(_contentReader);
                break;
            }
            //
            // Считываем вложенный контент
            //
            else if (textFolderTypeFromString(currentTag.toString())
                     != TextFolderType::Undefined) {
                appendItem(new AbstractTextModelFolderItem(model(), _contentReader));
            } else if (textGroupTypeFromString(currentTag.toString())
                       != TextGroupType::Undefined) {
                appendItem(new AbstractTextModelGroupItem(model(), _contentReader));
            } else if (currentTag == xml::kSplitterTag) {
                appendItem(new AbstractTextModelSplitterItem(model(), _contentReader));
            } else {
                appendItem(new AbstractTextModelTextItem(model(), _contentReader));
            }
        } while (!_contentReader.atEnd());
    }

    //
    // Определим название
    //
    handleChange();
}

AbstractTextModelFolderItem::~AbstractTextModelFolderItem() = default;

QColor AbstractTextModelFolderItem::color() const
{
    return d->color;
}

void AbstractTextModelFolderItem::setColor(const QColor& _color)
{
    if (d->color == _color) {
        return;
    }

    d->color = _color;
    setChanged(true);
}

QVariant AbstractTextModelFolderItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return u8"\U000f024b";
    }

    case FolderNameRole: {
        return d->name;
    }

    case FolderColorRole: {
        return d->color;
    }

    default: {
        return AbstractTextModelItem::data(_role);
    }
    }
}

QByteArray AbstractTextModelFolderItem::toXml() const
{
    return toXml(nullptr, 0, nullptr, 0, false);
}

QByteArray AbstractTextModelFolderItem::toXml(AbstractTextModelItem* _from, int _fromPosition,
                                              AbstractTextModelItem* _to, int _toPosition,
                                              bool _clearUuid) const
{
    auto folderFooterXml = [this] {
        AbstractTextModelTextItem item(model());
        item.setParagraphType(d->groupType == TextFolderType::Act
                                  ? TextParagraphType::ActFooter
                                  : TextParagraphType::FolderFooter);
        return item.toXml();
    };

    xml::AbstractTextModelXmlWriter xml;
    xml += xmlHeader(_clearUuid);
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);

        //
        // Нетекстовые блоки, просто добавляем к общему xml
        //
        if (child->type() == AbstractTextModelItemType::Splitter) {
            xml += child;
            continue;
        }
        //
        // Папки и сцены проверяем на наличие в них завершающего элемента
        //
        else if (child->type() != AbstractTextModelItemType::Text) {
            //
            // Если конечный элемент содержится в дите, то сохраняем его и завершаем формирование
            //
            const bool recursively = true;
            if (child->hasChild(_to, recursively)) {
                if (child->type() == AbstractTextModelItemType::Folder) {
                    auto folder = static_cast<AbstractTextModelFolderItem*>(child);
                    xml += folder->toXml(_from, _fromPosition, _to, _toPosition, _clearUuid);
                } else if (child->type() == AbstractTextModelItemType::Group) {
                    auto scene = static_cast<AbstractTextModelGroupItem*>(child);
                    xml += scene->toXml(_from, _fromPosition, _to, _toPosition, _clearUuid);
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
        auto textItem = static_cast<AbstractTextModelTextItem*>(child);
        if (textItem == _to) {
            if (textItem == _from) {
                xml += { textItem, _fromPosition, _toPosition - _fromPosition };
            } else {
                xml += { textItem, 0, _toPosition };
            }

            //
            // Если папка не была закрыта, добавим корректное завершение для неё
            //
            if (textItem->paragraphType() != TextParagraphType::FolderFooter) {
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
    xml += QString("</%1>\n").arg(toString(d->groupType)).toUtf8();

    return xml.data();
}

QByteArray AbstractTextModelFolderItem::xmlHeader(bool _clearUuid) const
{
    QByteArray xml;
    xml += QString("<%1 %2=\"%3\">\n")
               .arg(toString(d->groupType), xml::kUuidAttribute,
                    _clearUuid ? QUuid::createUuid().toString() : d->uuid.toString())
               .toUtf8();
    if (d->color.isValid()) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(xml::kColorTag, d->color.name()).toUtf8();
    }
    xml += QString("<%1>\n").arg(xml::kContentTag).toUtf8();

    return xml;
}

void AbstractTextModelFolderItem::copyFrom(AbstractTextModelItem* _item)
{
    if (_item->type() != AbstractTextModelItemType::Folder) {
        Q_ASSERT(false);
        return;
    }

    auto folderItem = static_cast<AbstractTextModelFolderItem*>(_item);
    d->uuid = folderItem->d->uuid;
    d->color = folderItem->d->color;
}

bool AbstractTextModelFolderItem::isEqual(AbstractTextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto folderItem = static_cast<AbstractTextModelFolderItem*>(_item);
    return d->uuid == folderItem->d->uuid && d->color == folderItem->d->color;
}

void AbstractTextModelFolderItem::handleChange()
{
    d->name.clear();

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case AbstractTextModelItemType::Text: {
            auto childItem = static_cast<AbstractTextModelTextItem*>(child);
            if (childItem->paragraphType() == TextParagraphType::FolderHeader) {
                d->name = TextHelper::smartToUpper(childItem->text());
            }
            break;
        }

        default:
            break;
        }
    }
}

} // namespace BusinessLayer
