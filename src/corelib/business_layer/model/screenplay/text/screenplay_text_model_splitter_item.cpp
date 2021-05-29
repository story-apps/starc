#include "screenplay_text_model_splitter_item.h"

#include "screenplay_text_model_xml.h"

#include <QVariant>
#include <QXmlStreamReader>


namespace BusinessLayer
{

namespace {
    const QHash<ScreenplayTextModelSplitterItemType, QString> kSplitterTypeToString
        = {{ ScreenplayTextModelSplitterItemType::Undefined, {} },
           { ScreenplayTextModelSplitterItemType::Start, QStringLiteral("start") },
           { ScreenplayTextModelSplitterItemType::End, QStringLiteral("end") }};
}

class ScreenplayTextModelSplitterItem::Implementation
{
public:
    Implementation() = default;
    explicit Implementation(ScreenplayTextModelSplitterItemType _type);

    ScreenplayTextModelSplitterItemType type = ScreenplayTextModelSplitterItemType::Undefined;
};

ScreenplayTextModelSplitterItem::Implementation::Implementation(ScreenplayTextModelSplitterItemType _type)
    : type(_type)
{
}


// ****


ScreenplayTextModelSplitterItem::ScreenplayTextModelSplitterItem(ScreenplayTextModelSplitterItemType _type)
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Splitter),
      d(new Implementation(_type))
{
}

ScreenplayTextModelSplitterItem::ScreenplayTextModelSplitterItem(QXmlStreamReader& _contentReader)
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Splitter),
      d(new Implementation)
{
    Q_ASSERT(_contentReader.name() == xml::kSplitterTag);

    d->type = kSplitterTypeToString.key(_contentReader.attributes().value(xml::kTypeAttribute).toString());

    xml::readNextElement(_contentReader); // end
    xml::readNextElement(_contentReader); // next
}

ScreenplayTextModelSplitterItem::~ScreenplayTextModelSplitterItem() = default;

ScreenplayTextModelSplitterItemType ScreenplayTextModelSplitterItem::splitterType() const
{
    return d->type;
}

QByteArray ScreenplayTextModelSplitterItem::toXml() const
{
    if (d->type == ScreenplayTextModelSplitterItemType::Undefined) {
        Q_ASSERT(0);
        return {};
    }

    return QString("<%1 %2=\"%3\"/>\n")
            .arg(xml::kSplitterTag, xml::kTypeAttribute, kSplitterTypeToString.value(d->type)).toUtf8();
}

void ScreenplayTextModelSplitterItem::copyFrom(ScreenplayTextModelItem* _item)
{
    if (_item->type() != ScreenplayTextModelItemType::Splitter) {
        Q_ASSERT(false);
        return;
    }
}

bool ScreenplayTextModelSplitterItem::isEqual(ScreenplayTextModelItem* _item) const
{
    if (_item == nullptr
        || type() != _item->type()) {
        return false;
    }

    const auto splitterItem = static_cast<ScreenplayTextModelSplitterItem*>(_item);
    return d->type == splitterItem->d->type;
}

} // namespace BusinessLayer
