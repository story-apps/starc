#include "screenplay_text_model_scene_item.h"

#include "screenplay_text_model_splitter_item.h"
#include "screenplay_text_model_text_item.h"

#include <utils/helpers/text_helper.h>

#include <QDomElement>
#include <QUuid>
#include <QVariant>

#include <optional>


namespace BusinessLayer
{

namespace {
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
    const QString kSplitterTag = QLatin1String("splitter");
}

class ScreenplayTextModelSceneItem::Implementation
{
public:
    Implementation();

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
    struct SceneNumber {
        QString value;
        int group = 0;
        int groupIndex = 0;
    };
    std::optional<SceneNumber> number;

    /**
     * @brief Штамп на сцене
     */
    QString stamp;

    /**
     * @brief Запланированная длительность сцены
     */
    std::optional<int> plannedDuration;
};

ScreenplayTextModelSceneItem::Implementation::Implementation()
    : uuid(QUuid::createUuid())
{
}


// ****


ScreenplayTextModelSceneItem::ScreenplayTextModelSceneItem()
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Scene),
      d(new Implementation)
{
}

ScreenplayTextModelSceneItem::ScreenplayTextModelSceneItem(const QDomElement& _node)
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Scene),
      d(new Implementation)
{
    Q_ASSERT(_node.tagName() == kSceneTag);

    d->uuid = _node.attribute(kUuidAttribute);
    //
    // TODO: plots
    //
    d->isOmited = _node.hasAttribute(kOmitedAttribute);
    const auto numberNode = _node.firstChildElement(kNumberTag);
    if (!numberNode.isNull()) {
        d->number = { numberNode.attribute(kNumberValueAttribute),
                      numberNode.attribute(kNumberGroupAttribute).toInt(),
                      numberNode.attribute(kNumberGroupIndexAttribute).toInt()};
    }
    const auto stampNode = _node.firstChildElement(kStampTag);
    if (!stampNode.isNull()) {
        d->stamp = TextHelper::fromHtmlEscaped(_node.text());
    }
    const auto plannedDurationNode = _node.firstChildElement(kPlannedDurationTag);
    if (!plannedDurationNode.isNull()) {
        d->plannedDuration = plannedDurationNode.text().toInt();
    }
    auto childNode = _node.firstChildElement(kContentTag).firstChildElement();
    while (!childNode.isNull()) {
        if (childNode.tagName() == kSceneTag) {
            appendItem(new ScreenplayTextModelSceneItem(childNode));
        } else if (childNode.tagName() == kSplitterTag) {
            appendItem(new ScreenplayTextModelSplitterItem(childNode));
        } else {
            appendItem(new ScreenplayTextModelTextItem(childNode));
        }
        childNode = childNode.nextSiblingElement();
    }
}

ScreenplayTextModelSceneItem::~ScreenplayTextModelSceneItem() = default;

QVariant ScreenplayTextModelSceneItem::data(int _role) const
{
    switch (_role) {
        case Qt::DisplayRole: {
            for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
                auto child = childAt(childIndex);
                if (child->type() != ScreenplayTextModelItemType::Text) {
                    continue;
                }

                auto childTextItem = static_cast<ScreenplayTextModelTextItem*>(child);
                return TextHelper::smartToUpper(childTextItem->text());
            }
            return {};
        }

        case Qt::DecorationRole: {
            return u8"\uf21a";
        }
    }

    return {};
}

QString ScreenplayTextModelSceneItem::toXml() const
{
    QByteArray xml;
    //
    // TODO: plots
    //
    xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6>")
           .arg(kSceneTag,
                kUuidAttribute, d->uuid.toString(),
                kPlotsAttribute, {},
                (d->isOmited ? QString("%1=\"true\"").arg(kOmitedAttribute) : ""));
    if (d->number.has_value()) {
        xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\"/>")
               .arg(kNumberTag,
                    kNumberValueAttribute, d->number->value,
                    kNumberGroupAttribute, QString::number(d->number->group),
                    kNumberGroupIndexAttribute, QString::number(d->number->groupIndex));
    }
    if (!d->stamp.isEmpty()) {
        xml += QString("<%1><![CDATA[%2]]></%1>").arg(kStampTag, TextHelper::toHtmlEscaped(d->stamp));
    }
    if (d->plannedDuration.has_value()) {
        xml += QString("<%1>%2</%1>").arg(kPlannedDurationTag, QString::number(*d->plannedDuration));
    }
    xml.append(QString("<%1>").arg(kContentTag));
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        xml += childAt(childIndex)->toXml();
    }
    xml.append(QString("</%1>\n").arg(kContentTag));
    xml.append(QString("</%1>\n").arg(kSceneTag));

    return xml;
}

} // namespace BusinessLayer
