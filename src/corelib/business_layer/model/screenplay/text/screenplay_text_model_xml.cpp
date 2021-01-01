#include "screenplay_text_model_xml.h"

namespace BusinessLayer
{
namespace xml
{

QStringRef readContent(QXmlStreamReader& _reader)
{
    _reader.readNext();
    return _reader.text();
}

QStringRef readNextElement(QXmlStreamReader& _reader)
{
    do {
        _reader.readNext();
    } while (!_reader.isStartElement()
             && !_reader.isEndElement());
    return _reader.name();
}

} // namespace xml
} // namespace BusinessLayer
