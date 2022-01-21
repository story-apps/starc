#include "comic_book_text_model_folder_item.h"

#include "comic_book_text_model_page_item.h"
#include "comic_book_text_model_panel_item.h"
#include "comic_book_text_model_splitter_item.h"
#include "comic_book_text_model_text_item.h"
#include "comic_book_text_model_xml.h"
#include "comic_book_text_model_xml_writer.h"

#include <business_layer/templates/comic_book_template.h>
#include <utils/helpers/text_helper.h>

#include <QUuid>
#include <QVariant>
#include <QXmlStreamReader>


namespace BusinessLayer {

class ComicBookTextModelFolderItem::Implementation
{
public:
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


ComicBookTextModelFolderItem::ComicBookTextModelFolderItem()
    : ComicBookTextModelItem(ComicBookTextModelItemType::Folder)
    , d(new Implementation)
{
    d->uuid = QUuid::createUuid();
}

ComicBookTextModelFolderItem::ComicBookTextModelFolderItem(QXmlStreamReader& _contentReader)
    : ComicBookTextModelItem(ComicBookTextModelItemType::Folder)
    , d(new Implementation)
{
    Q_ASSERT(_contentReader.name() == xml::kFolderTag);

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
            else if (currentTag == xml::kFolderTag && _contentReader.isEndElement()) {
                xml::readNextElement(_contentReader);
                break;
            }
            //
            // Считываем вложенный контент
            //
            else if (currentTag == xml::kFolderTag) {
                appendItem(new ComicBookTextModelFolderItem(_contentReader));
            } else if (currentTag == xml::kPageTag) {
                appendItem(new ComicBookTextModelPageItem(_contentReader));
            } else if (currentTag == xml::kPanelTag) {
                appendItem(new ComicBookTextModelPanelItem(_contentReader));
            } else if (currentTag == xml::kSplitterTag) {
                appendItem(new ComicBookTextModelSplitterItem(_contentReader));
            } else {
                appendItem(new ComicBookTextModelTextItem(_contentReader));
            }
        } while (!_contentReader.atEnd());
    }

    //
    // Определим название
    //
    handleChange();
}

ComicBookTextModelFolderItem::~ComicBookTextModelFolderItem() = default;

QColor ComicBookTextModelFolderItem::color() const
{
    return d->color;
}

void ComicBookTextModelFolderItem::setColor(const QColor& _color)
{
    if (d->color == _color) {
        return;
    }

    d->color = _color;
    setChanged(true);
}

QVariant ComicBookTextModelFolderItem::data(int _role) const
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
        return ComicBookTextModelItem::data(_role);
    }
    }
}

QByteArray ComicBookTextModelFolderItem::toXml() const
{
    return toXml(nullptr, 0, nullptr, 0, false);
}

QByteArray ComicBookTextModelFolderItem::toXml(ComicBookTextModelItem* _from, int _fromPosition,
                                               ComicBookTextModelItem* _to, int _toPosition,
                                               bool _clearUuid) const
{
    auto folderFooterXml = [] {
        ComicBookTextModelTextItem item;
        item.setParagraphType(ComicBookParagraphType::FolderFooter);
        return item.toXml();
    };

    xml::ComicBookTextModelXmlWriter xml;
    xml += xmlHeader(_clearUuid);
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);

        //
        // Нетекстовые блоки, просто добавляем к общему xml
        //
        if (child->type() == ComicBookTextModelItemType::Splitter) {
            xml += child;
            continue;
        }
        //
        // Папки и сцены проверяем на наличие в них завершающего элемента
        //
        else if (child->type() != ComicBookTextModelItemType::Text) {
            //
            // Если конечный элемент содержится в дите, то сохраняем его и завершаем формирование
            //
            const bool recursively = true;
            if (child->hasChild(_to, recursively)) {
                if (child->type() == ComicBookTextModelItemType::Folder) {
                    auto folder = static_cast<ComicBookTextModelFolderItem*>(child);
                    xml += folder->toXml(_from, _fromPosition, _to, _toPosition, _clearUuid);
                } else if (child->type() == ComicBookTextModelItemType::Page) {
                    auto page = static_cast<ComicBookTextModelPageItem*>(child);
                    xml += page->toXml(_from, _fromPosition, _to, _toPosition, _clearUuid);
                } else if (child->type() == ComicBookTextModelItemType::Panel) {
                    auto panel = static_cast<ComicBookTextModelPanelItem*>(child);
                    xml += panel->toXml(_from, _fromPosition, _to, _toPosition, _clearUuid);
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
        auto textItem = static_cast<ComicBookTextModelTextItem*>(child);
        if (textItem == _to) {
            if (textItem == _from) {
                xml += { textItem, _fromPosition, _toPosition - _fromPosition };
            } else {
                xml += { textItem, 0, _toPosition };
            }

            //
            // Если папка не была закрыта, добавим корректное завершение для неё
            //
            if (textItem->paragraphType() != ComicBookParagraphType::FolderFooter) {
                xml += folderFooterXml();
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
    xml += QString("</%1>\n").arg(xml::kFolderTag).toUtf8();

    return xml.data();
}

QByteArray ComicBookTextModelFolderItem::xmlHeader(bool _clearUuid) const
{
    QByteArray xml;
    xml += QString("<%1 %2=\"%3\">\n")
               .arg(xml::kFolderTag, xml::kUuidAttribute,
                    _clearUuid ? QUuid::createUuid().toString() : d->uuid.toString())
               .toUtf8();
    if (d->color.isValid()) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(xml::kColorTag, d->color.name()).toUtf8();
    }
    xml += QString("<%1>\n").arg(xml::kContentTag).toUtf8();

    return xml;
}

void ComicBookTextModelFolderItem::copyFrom(ComicBookTextModelItem* _item)
{
    if (_item->type() != ComicBookTextModelItemType::Folder) {
        Q_ASSERT(false);
        return;
    }

    auto folderItem = static_cast<ComicBookTextModelFolderItem*>(_item);
    d->uuid = folderItem->d->uuid;
    d->color = folderItem->d->color;
}

bool ComicBookTextModelFolderItem::isEqual(ComicBookTextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto folderItem = static_cast<ComicBookTextModelFolderItem*>(_item);
    return d->uuid == folderItem->d->uuid && d->color == folderItem->d->color;
}

void ComicBookTextModelFolderItem::handleChange()
{
    d->name.clear();

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case ComicBookTextModelItemType::Text: {
            auto childItem = static_cast<ComicBookTextModelTextItem*>(child);
            if (childItem->paragraphType() == ComicBookParagraphType::FolderHeader) {
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
