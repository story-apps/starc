#include "screenplay_text_model_folder_item.h"

#include "screenplay_text_model_scene_item.h"
#include "screenplay_text_model_text_item.h"

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

    QString name;
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

    d->uuid = _node.attribute(kUuidAttribute);

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

QString ScreenplayTextModelFolderItem::toXml() const
{
    QByteArray xml;
    xml += QString("<%1 %2=\"%3\">")
           .arg(kFolderTag,
                kUuidAttribute, d->uuid.toString());
    xml.append(QString("<%1>").arg(kContentTag));
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        xml += childAt(childIndex)->toXml();
    }
    xml.append(QString("</%1>\n").arg(kContentTag));
    xml.append(QString("</%1>\n").arg(kFolderTag));

    return xml;
}

void ScreenplayTextModelFolderItem::handleChange()
{
    d->name.clear();

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        if (child->type() != ScreenplayTextModelItemType::Text) {
            continue;
        }

        auto childTextItem = static_cast<ScreenplayTextModelTextItem*>(child);
        d->name = TextHelper::smartToUpper(childTextItem->text());
        break;
    }
}

} // namespace BusinessLayer
