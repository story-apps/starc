#include "screenplay_text_model_xml.h"

namespace BusinessLayer
{
namespace xml
{

QString prepareXml(const QString& _xml)
{
    QString preparedXml = _xml;

    //
    // Добавляем открывающие тэги, если есть брошенные закрывающие
    //
    const auto firstContentTagIndex = preparedXml.indexOf(kContentTag);
    if (firstContentTagIndex != -1) {
        const QString tagTemplate = QLatin1String("<%1>\n");
        auto readParentTag = [preparedXml] (int _index) {
            Q_ASSERT(_index != -1);
            _index += kContentTag.length() + 2; // закрывающий тэг знак и перенос строки
            Q_ASSERT(preparedXml[_index] == '<');
            ++_index;
            Q_ASSERT(preparedXml[_index] == '/');
            ++_index;
            QString parentTag;
            while (preparedXml[_index] != '>'
                   && _index < preparedXml.length()) {
                parentTag += preparedXml[_index];
                ++_index;
            }
            return parentTag;
        };
        auto index = firstContentTagIndex - 1;
        //
        // Если первым идёт закрывающий, то нужно добавить соответствующие открывающие тэги
        //
        if (preparedXml[index] == '/') {
            const auto parentTag = readParentTag(firstContentTagIndex);
            preparedXml.prepend(tagTemplate.arg(kContentTag));
            preparedXml.prepend(tagTemplate.arg(parentTag));
        }
        //
        // Если первым идёт открывающий и он в начале xml, то нужно добавить тэг сцены или папки
        //
        else if (index == 0) {
            index = preparedXml.indexOf(kContentTag, firstContentTagIndex);
            const auto parentTag = readParentTag(index);
            preparedXml.prepend(tagTemplate.arg(parentTag));
        }
    }

    //
    // Добавляем стандартные тэги, чтобы получился более менее нормальный xml :)
    //
    const QString xmlHeader = "<?xml version=\"1.0\"?>\n";
    const QString documentHeaderOpener = QString("<%1").arg(kDocumentTag);
    const QString documentHeader = QString("%1>\n").arg(documentHeaderOpener);
    const QString documentFooter = QString("</%1>\n").arg(kDocumentTag);
    if (!preparedXml.startsWith(xmlHeader)) {
        if (!preparedXml.startsWith(documentHeaderOpener)) {
            preparedXml.prepend(documentHeader);
        }
        preparedXml.prepend(xmlHeader);
    }
    if (!preparedXml.endsWith(documentFooter)) {
        if (!preparedXml.endsWith('\n')) {
            preparedXml.append('\n');
        }
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
