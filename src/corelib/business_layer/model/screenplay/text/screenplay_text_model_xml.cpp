#include "screenplay_text_model_xml.h"

namespace BusinessLayer
{
namespace xml
{

QString prepareXml(const QString& _xml)
{
    const QString xmlHeader = "<?xml version=\"1.0\"?>\n";
    const QString documentHeaderOpener = QString("<%1").arg(kDocumentTag);
    const QString documentHeader = QString("%1>\n").arg(documentHeaderOpener);
    const QString documentFooter = QString("</%1>\n").arg(kDocumentTag);

    QString preparedXml = _xml;
    if (!preparedXml.startsWith(xmlHeader)) {
        if (!preparedXml.startsWith(documentHeaderOpener)) {
            preparedXml.prepend(documentHeader);
        }
        preparedXml.prepend(xmlHeader);
    }
    if (!preparedXml.endsWith(documentFooter)) {
        preparedXml.append(documentFooter);
    }
    return preparedXml;
}

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
             && !_reader.isEndElement()
             && !_reader.atEnd());
    return _reader.name();
}

} // namespace xml
} // namespace BusinessLayer
