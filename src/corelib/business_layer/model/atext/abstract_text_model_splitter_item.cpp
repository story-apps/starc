#include "abstract_text_model_splitter_item.h"

#include "abstract_text_model_xml.h"

#include <QVariant>
#include <QXmlStreamReader>


namespace BusinessLayer {

namespace {
const QHash<AbstractTextModelSplitterItemType, QString> kSplitterTypeToString = {
    { AbstractTextModelSplitterItemType::Undefined, QLatin1String("undefined") },
    { AbstractTextModelSplitterItemType::Start, QLatin1String("start") },
    { AbstractTextModelSplitterItemType::End, QLatin1String("end") },
};
}

class AbstractTextModelSplitterItem::Implementation
{
public:
    Implementation() = default;
    explicit Implementation(AbstractTextModelSplitterItemType _type);

    AbstractTextModelSplitterItemType splitterType = AbstractTextModelSplitterItemType::Undefined;
};

AbstractTextModelSplitterItem::Implementation::Implementation(
    AbstractTextModelSplitterItemType _type)
    : splitterType(_type)
{
}


// ****


AbstractTextModelSplitterItem::AbstractTextModelSplitterItem(
    const AbstractTextModel* _model, AbstractTextModelSplitterItemType _type)
    : AbstractTextModelItem(AbstractTextModelItemType::Splitter, _model)
    , d(new Implementation(_type))
{
}

AbstractTextModelSplitterItem::AbstractTextModelSplitterItem(const AbstractTextModel* _model,
                                                             QXmlStreamReader& _contentReader)
    : AbstractTextModelItem(AbstractTextModelItemType::Splitter, _model)
    , d(new Implementation)
{
    Q_ASSERT(_contentReader.name() == xml::kSplitterTag);

    d->splitterType = kSplitterTypeToString.key(
        _contentReader.attributes().value(xml::kTypeAttribute).toString());

    xml::readNextElement(_contentReader); // end
    xml::readNextElement(_contentReader); // next
}

AbstractTextModelSplitterItem::~AbstractTextModelSplitterItem() = default;

AbstractTextModelSplitterItemType AbstractTextModelSplitterItem::splitterType() const
{
    return d->splitterType;
}

QByteArray AbstractTextModelSplitterItem::toXml() const
{
    if (d->splitterType == AbstractTextModelSplitterItemType::Undefined) {
        Q_ASSERT(0);
        return {};
    }

    return QString("<%1 %2=\"%3\"/>\n")
        .arg(xml::kSplitterTag, xml::kTypeAttribute, kSplitterTypeToString.value(d->splitterType))
        .toUtf8();
}

void AbstractTextModelSplitterItem::copyFrom(AbstractTextModelItem* _item)
{
    if (_item->type() != AbstractTextModelItemType::Splitter) {
        Q_ASSERT(false);
        return;
    }
}

bool AbstractTextModelSplitterItem::isEqual(AbstractTextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto splitterItem = static_cast<AbstractTextModelSplitterItem*>(_item);
    return d->splitterType == splitterItem->d->splitterType;
}

} // namespace BusinessLayer
