#include "text_model_splitter_item.h"

#include "text_model_xml.h"

#include <QVariant>
#include <QXmlStreamReader>


namespace BusinessLayer {

namespace {
const QHash<TextModelSplitterItemType, QString> kSplitterTypeToString = {
    { TextModelSplitterItemType::Undefined, QLatin1String("undefined") },
    { TextModelSplitterItemType::Start, QLatin1String("start") },
    { TextModelSplitterItemType::End, QLatin1String("end") },
};
}

class TextModelSplitterItem::Implementation
{
public:
    Implementation() = default;
    explicit Implementation(TextModelSplitterItemType _type);

    TextModelSplitterItemType splitterType = TextModelSplitterItemType::Undefined;
};

TextModelSplitterItem::Implementation::Implementation(TextModelSplitterItemType _type)
    : splitterType(_type)
{
}


// ****


TextModelSplitterItem::TextModelSplitterItem(const TextModel* _model)
    : TextModelItem(TextModelItemType::Splitter, _model)
    , d(new Implementation)
{
}

TextModelSplitterItem::~TextModelSplitterItem() = default;

TextModelSplitterItemType TextModelSplitterItem::splitterType() const
{
    return d->splitterType;
}

void TextModelSplitterItem::setSplitterType(TextModelSplitterItemType _type)
{
    if (d->splitterType == _type) {
        return;
    }

    d->splitterType = _type;
    setChanged(true);
}

void TextModelSplitterItem::readContent(QXmlStreamReader& _contentReader)
{
    Q_ASSERT(_contentReader.name() == xml::kSplitterTag);

    d->splitterType = kSplitterTypeToString.key(
        _contentReader.attributes().value(xml::kTypeAttribute).toString());

    xml::readNextElement(_contentReader); // end
    xml::readNextElement(_contentReader); // next
}

QByteArray TextModelSplitterItem::toXml() const
{
    if (d->splitterType == TextModelSplitterItemType::Undefined) {
        Q_ASSERT(0);
        return {};
    }

    return QString("<%1 %2=\"%3\"/>\n")
        .arg(xml::kSplitterTag, xml::kTypeAttribute, kSplitterTypeToString.value(d->splitterType))
        .toUtf8();
}

void TextModelSplitterItem::copyFrom(TextModelItem* _item)
{
    if (_item == nullptr || type() != _item->type()) {
        Q_ASSERT(false);
        return;
    }
}

bool TextModelSplitterItem::isEqual(TextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto splitterItem = static_cast<TextModelSplitterItem*>(_item);
    return d->splitterType == splitterItem->d->splitterType;
}

} // namespace BusinessLayer
