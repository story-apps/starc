#include "screenplay_text_model_xml_writer.h"

#include "screenplay_text_model_text_item.h"

#include <QString>

namespace BusinessLayer {
namespace xml {

class ScreenplayTextModelXmlWriter::Implementation
{
public:
    QByteArray data;
};


// ****


ScreenplayTextModelXmlWriter::ScreenplayTextModelXmlWriter(bool _addHeader)
    : d(new Implementation)
{
    if (_addHeader) {
        d->data = "<?xml version=\"1.0\"?>\n";
    }
}

void ScreenplayTextModelXmlWriter::operator+=(const char* _data)
{
    d->data += _data;
}

void ScreenplayTextModelXmlWriter::operator+=(const QByteArray& _data)
{
    d->data += _data;
}

void ScreenplayTextModelXmlWriter::operator+=(const QString& _data)
{
    d->data += _data.toUtf8();
}

void ScreenplayTextModelXmlWriter::operator+=(ScreenplayTextModelItem* _data)
{
    d->data += _data->toXml();
}

void ScreenplayTextModelXmlWriter::operator+=(const TextItemData& _data)
{
    d->data += _data.item->toXml(_data.fromPosition, _data.toPosition);
}

QByteArray ScreenplayTextModelXmlWriter::data() const
{
    return d->data;
}

ScreenplayTextModelXmlWriter::~ScreenplayTextModelXmlWriter() = default;


} // namespace xml
} // namespace BusinessLayer
