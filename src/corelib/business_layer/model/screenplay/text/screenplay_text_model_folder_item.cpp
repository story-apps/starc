#include "screenplay_text_model_folder_item.h"

#include "screenplay_text_model_scene_item.h"
#include "screenplay_text_model_splitter_item.h"
#include "screenplay_text_model_text_item.h"
#include "screenplay_text_model_xml.h"
#include "screenplay_text_model_xml_writer.h"

#include <business_layer/templates/screenplay_template.h>

#include <utils/helpers/text_helper.h>

#include <QUuid>
#include <QVariant>
#include <QXmlStreamReader>


namespace BusinessLayer
{

class ScreenplayTextModelFolderItem::Implementation
{
public:
    /**
     * @brief Идентификатор папки
     */
    QUuid uuid;

    //
    // Ридонли свойства, которые формируются по ходу работы со сценарием
    //

    /**
     * @brief Название папки
     */
    QString name;

    /**
     * @brief Длительность папки
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{0};
};


// ****


ScreenplayTextModelFolderItem::ScreenplayTextModelFolderItem()
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Folder),
      d(new Implementation)
{
    d->uuid = QUuid::createUuid();
}

ScreenplayTextModelFolderItem::ScreenplayTextModelFolderItem(QXmlStreamReader& _contentReader)
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Folder),
      d(new Implementation)
{
    Q_ASSERT(_contentReader.name() == xml::kFolderTag);

    if (_contentReader.attributes().hasAttribute(xml::kUuidAttribute)) {
        d->uuid = _contentReader.attributes().value(xml::kUuidAttribute).toString();
    }
    auto currentTag = xml::readNextElement(_contentReader); // content

    if (currentTag == xml::kContentTag) {
        xml::readNextElement(_contentReader); // next item
        do {
            currentTag = _contentReader.name();

            //
            // Проглатываем закрывающий контентный тэг
            //
            if (currentTag == xml::kContentTag
                && _contentReader.isEndElement()) {
                xml::readNextElement(_contentReader);
                continue;
            }
            //
            // Если дошли до конца папки, выходим из обработки
            //
            else if (currentTag == xml::kFolderTag
                && _contentReader.isEndElement()) {
                xml::readNextElement(_contentReader);
                break;
            }
            //
            // Считываем вложенный контент
            //
            else if (currentTag == xml::kFolderTag) {
                appendItem(new ScreenplayTextModelFolderItem(_contentReader));
            } else if (currentTag == xml::kSceneTag) {
                appendItem(new ScreenplayTextModelSceneItem(_contentReader));
            } else if (currentTag == xml::kSplitterTag) {
                appendItem(new ScreenplayTextModelSplitterItem(_contentReader));
            } else {
                appendItem(new ScreenplayTextModelTextItem(_contentReader));
            }
        } while (!_contentReader.atEnd());
    }

    //
    // Определим название
    //
    handleChange();
}

ScreenplayTextModelFolderItem::~ScreenplayTextModelFolderItem() = default;

std::chrono::milliseconds ScreenplayTextModelFolderItem::duration() const
{
    return d->duration;
}

QVariant ScreenplayTextModelFolderItem::data(int _role) const
{
    switch (_role) {
        case Qt::DecorationRole: {
            return u8"\U000f024b";
        }

        case FolderNameRole: {
            return d->name;
        }

        case FolderDurationRole: {
            const int duration = std::chrono::duration_cast<std::chrono::seconds>(d->duration).count();
            return duration;
        }

        default: {
            return ScreenplayTextModelItem::data(_role);
        }
    }
}

QByteArray ScreenplayTextModelFolderItem::toXml() const
{
    return toXml(nullptr, 0, nullptr, 0, false);
}

QByteArray ScreenplayTextModelFolderItem::toXml(ScreenplayTextModelItem* _from, int _fromPosition, ScreenplayTextModelItem* _to, int _toPosition, bool _clearUuid) const
{
    auto folderFooterXml = [] {
        ScreenplayTextModelTextItem item;
        item.setParagraphType(ScreenplayParagraphType::FolderFooter);
        return item.toXml();
    };

    xml::ScreenplayTextModelXmlWriter xml;
    xml += xmlHeader(_clearUuid);
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);

        //
        // Нетекстовые блоки, просто добавляем к общему xml
        //
        if (child->type() == ScreenplayTextModelItemType::Splitter) {
            xml += child;
            continue;
        }
        //
        // Папки и сцены проверяем на наличие в них завершающего элемента
        //
        else if (child->type() != ScreenplayTextModelItemType::Text) {
            //
            // Если конечный элемент содержится в дите, то сохраняем его и завершаем формирование
            //
            const bool recursively = true;
            if (child->hasChild(_to, recursively)) {
                if (child->type() == ScreenplayTextModelItemType::Folder) {
                    auto folder = static_cast<ScreenplayTextModelFolderItem*>(child);
                    xml += folder->toXml(_from, _fromPosition, _to, _toPosition, _clearUuid);
                } else if (child->type() == ScreenplayTextModelItemType::Scene) {
                    auto scene = static_cast<ScreenplayTextModelSceneItem*>(child);
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
        auto textItem = static_cast<ScreenplayTextModelTextItem*>(child);
        if (textItem == _to) {
            if (textItem == _from) {
                xml += { textItem, _fromPosition, _toPosition - _fromPosition };
            } else {
                xml += { textItem, 0, _toPosition };
            }

            //
            // Если папка не была закрыта, добавим корректное завершение для неё
            //
            if (textItem->paragraphType() != ScreenplayParagraphType::FolderFooter) {
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

QByteArray ScreenplayTextModelFolderItem::xmlHeader(bool _clearUuid) const
{
    QByteArray xml;
    if (_clearUuid) {
        xml += QString("<%1>\n").arg(xml::kFolderTag).toUtf8();
    } else {
        xml += QString("<%1 %2=\"%3\">\n")
               .arg(xml::kFolderTag,
                    xml::kUuidAttribute, d->uuid.toString()).toUtf8();
    }
    xml += QString("<%1>\n").arg(xml::kContentTag).toUtf8();

    return xml;
}

void ScreenplayTextModelFolderItem::copyFrom(ScreenplayTextModelItem* _item)
{
    if (_item->type() != ScreenplayTextModelItemType::Folder) {
        Q_ASSERT(false);
        return;
    }

    auto folderItem = static_cast<ScreenplayTextModelFolderItem*>(_item);
    d->uuid = folderItem->d->uuid;
}

bool ScreenplayTextModelFolderItem::isEqual(ScreenplayTextModelItem* _item) const
{
    if (_item == nullptr
        || type() != _item->type()) {
        return false;
    }

    const auto folderItem = static_cast<ScreenplayTextModelFolderItem*>(_item);
    return d->uuid == folderItem->d->uuid;
}

void ScreenplayTextModelFolderItem::handleChange()
{
    d->name.clear();
    d->duration = std::chrono::seconds{0};

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
            case ScreenplayTextModelItemType::Folder: {
                auto childItem = static_cast<ScreenplayTextModelFolderItem*>(child);
                d->duration += childItem->duration();
                break;
            }

            case ScreenplayTextModelItemType::Scene: {
                auto childItem = static_cast<ScreenplayTextModelSceneItem*>(child);
                d->duration += childItem->duration();
                break;
            }

            case ScreenplayTextModelItemType::Text: {
                auto childItem = static_cast<ScreenplayTextModelTextItem*>(child);
                if (childItem->paragraphType() == ScreenplayParagraphType::FolderHeader) {
                    d->name = TextHelper::smartToUpper(childItem->text());
                }
                d->duration += childItem->duration();
                break;
            }

            default: break;
        }
    }
}

} // namespace BusinessLayer
