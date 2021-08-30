#include "comic_book_text_model_splitter_item.h"

#include "comic_book_text_model_xml.h"

#include <QVariant>
#include <QXmlStreamReader>


namespace BusinessLayer {

namespace {
const QHash<ComicBookTextModelSplitterItemType, QString> kSplitterTypeToString
    = { { ComicBookTextModelSplitterItemType::Undefined, {} },
        { ComicBookTextModelSplitterItemType::Start, QStringLiteral("start") },
        { ComicBookTextModelSplitterItemType::End, QStringLiteral("end") } };
}

class ComicBookTextModelSplitterItem::Implementation
{
public:
    Implementation() = default;
    explicit Implementation(ComicBookTextModelSplitterItemType _type);

    ComicBookTextModelSplitterItemType type = ComicBookTextModelSplitterItemType::Undefined;
};

ComicBookTextModelSplitterItem::Implementation::Implementation(
    ComicBookTextModelSplitterItemType _type)
    : type(_type)
{
}


// ****


ComicBookTextModelSplitterItem::ComicBookTextModelSplitterItem(
    ComicBookTextModelSplitterItemType _type)
    : ComicBookTextModelItem(ComicBookTextModelItemType::Splitter)
    , d(new Implementation(_type))
{
}

ComicBookTextModelSplitterItem::ComicBookTextModelSplitterItem(QXmlStreamReader& _contentReader)
    : ComicBookTextModelItem(ComicBookTextModelItemType::Splitter)
    , d(new Implementation)
{
    Q_ASSERT(_contentReader.name() == xml::kSplitterTag);

    d->type = kSplitterTypeToString.key(
        _contentReader.attributes().value(xml::kTypeAttribute).toString());

    xml::readNextElement(_contentReader); // end
    xml::readNextElement(_contentReader); // next
}

ComicBookTextModelSplitterItem::~ComicBookTextModelSplitterItem() = default;

ComicBookTextModelSplitterItemType ComicBookTextModelSplitterItem::splitterType() const
{
    return d->type;
}

QByteArray ComicBookTextModelSplitterItem::toXml() const
{
    if (d->type == ComicBookTextModelSplitterItemType::Undefined) {
        Q_ASSERT(0);
        return {};
    }

    return QString("<%1 %2=\"%3\"/>\n")
        .arg(xml::kSplitterTag, xml::kTypeAttribute, kSplitterTypeToString.value(d->type))
        .toUtf8();
}

void ComicBookTextModelSplitterItem::copyFrom(ComicBookTextModelItem* _item)
{
    if (_item->type() != ComicBookTextModelItemType::Splitter) {
        Q_ASSERT(false);
        return;
    }
}

bool ComicBookTextModelSplitterItem::isEqual(ComicBookTextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto splitterItem = static_cast<ComicBookTextModelSplitterItem*>(_item);
    return d->type == splitterItem->d->type;
}

} // namespace BusinessLayer
