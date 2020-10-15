#include "screenplay_text_model_folder_item.h"

#include "screenplay_text_model_scene_item.h"
#include "screenplay_text_model_text_item.h"

#include <business_layer/templates/screenplay_template.h>

#include <utils/helpers/text_helper.h>

#include <QDomElement>
#include <QUuid>
#include <QVariant>


namespace BusinessLayer
{

namespace {
    const QString kFolderTag = QLatin1String("folder");
    const QString kSceneTag = QLatin1String("scene");
    const QString kUuidAttribute = QLatin1String("uuid");
    const QString kPlotsAttribute = QLatin1String("plots");
    const QString kOmitedAttribute = QLatin1String("omited");
    const QString kNumberTag = QLatin1String("number");
    const QString kNumberValueAttribute = QLatin1String("value");
    const QString kNumberGroupAttribute = QLatin1String("group");
    const QString kNumberGroupIndexAttribute = QLatin1String("group_index");
    const QString kStampTag = QLatin1String("stamp");
    const QString kPlannedDurationTag = QLatin1String("planned_duration");
    const QString kContentTag = QLatin1String("content");
}

class ScreenplayTextModelFolderItem::Implementation
{
public:
    Implementation();

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

ScreenplayTextModelFolderItem::Implementation::Implementation()
    : uuid(QUuid::createUuid())
{
}


// ****


ScreenplayTextModelFolderItem::ScreenplayTextModelFolderItem()
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Folder),
      d(new Implementation)
{
}

ScreenplayTextModelFolderItem::ScreenplayTextModelFolderItem(const QDomElement& _node)
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Folder),
      d(new Implementation)
{
    Q_ASSERT(_node.tagName() == kFolderTag);

    d->uuid = _node.hasAttribute(kUuidAttribute) ? _node.attribute(kUuidAttribute)
                                                 : QUuid::createUuid();

    auto childNode = _node.firstChildElement(kContentTag).firstChildElement();
    while (!childNode.isNull()) {
        if (childNode.tagName() == kFolderTag) {
            appendItem(new ScreenplayTextModelFolderItem(childNode));
        } else if (childNode.tagName() == kSceneTag) {
            appendItem(new ScreenplayTextModelSceneItem(childNode));
        } else {
            appendItem(new ScreenplayTextModelTextItem(childNode));
        }
        childNode = childNode.nextSiblingElement();
    }

    //
    // Определим название
    //
    handleChange();
}

std::chrono::milliseconds ScreenplayTextModelFolderItem::duration() const
{
    return d->duration;
}

ScreenplayTextModelFolderItem::~ScreenplayTextModelFolderItem() = default;

QVariant ScreenplayTextModelFolderItem::data(int _role) const
{
    switch (_role) {
        case Qt::DecorationRole: {
            return u8"\U000f024b";
        }

        case FolderNameRole: {
            return d->name;
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

    QByteArray xml;
    if (_clearUuid) {
        xml += QString("<%1>\n").arg(kFolderTag).toUtf8();
    } else {
        xml += QString("<%1 %2=\"%3\">\n")
               .arg(kFolderTag,
                    kUuidAttribute, d->uuid.toString()).toUtf8();
    }
    xml += QString("<%1>\n").arg(kContentTag).toUtf8();
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);

        //
        // Нетекстовые блоки, просто добавляем к общему xml
        //
        if (child->type() == ScreenplayTextModelItemType::Splitter) {
            xml += child->toXml();
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
                xml += child->toXml();
                continue;
            }
        }

        //
        // Текстовые блоки, в зависимости от необходимости вставить блок целиком, или его часть
        //
        auto textItem = static_cast<ScreenplayTextModelTextItem*>(child);
        if (textItem == _to) {
            if (textItem == _from) {
                xml += textItem->toXml(_fromPosition, _toPosition - _fromPosition);
            } else {
                xml += textItem->toXml(0, _toPosition);
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
            xml += textItem->toXml(_fromPosition, textItem->text().length() - _fromPosition);
        } else {
            xml += textItem->toXml();
        }
    }
    xml += QString("</%1>\n").arg(kContentTag).toUtf8();
    xml += QString("</%1>\n").arg(kFolderTag).toUtf8();

    return xml;
}

void ScreenplayTextModelFolderItem::handleChange()
{
    d->name.clear();
    d->duration = std::chrono::seconds{0};

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
            case ScreenplayTextModelItemType::Text: {
                auto childTextItem = static_cast<ScreenplayTextModelTextItem*>(child);
                if (childTextItem->paragraphType() == ScreenplayParagraphType::FolderHeader) {
                    d->name = TextHelper::smartToUpper(childTextItem->text());
                }
                d->duration += childTextItem->duration();
                break;
            }

            case ScreenplayTextModelItemType::Scene: {
                auto childSceneItem = static_cast<ScreenplayTextModelSceneItem*>(child);
                d->duration += childSceneItem->duration();
                break;
            }

            default: break;
        }
    }
}

} // namespace BusinessLayer
