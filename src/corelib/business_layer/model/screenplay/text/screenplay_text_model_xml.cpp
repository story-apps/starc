#include "screenplay_text_model_xml.h"

namespace BusinessLayer
{
namespace xml
{

QString prepareXml(const QString& _xml)
{
    //
    // Балансируем xml
    //
    auto readParentTag = [_xml] (int _index) {
        Q_ASSERT(_index != -1);
        _index += kContentTag.length() + 2; // закрывающий тэг знак и перенос строки
        Q_ASSERT(_xml[_index] == '<');
        ++_index;
        Q_ASSERT(_xml[_index] == '/');
        ++_index;
        QString parentTag;
        while (_xml[_index] != '>'
               && _index < _xml.length()) {
            parentTag += _xml[_index];
            ++_index;
        }
        return parentTag;
    };
    QString preparedXml = _xml;
    const QString tagTemplate = QLatin1String("<%1>\n");
    auto contentTagIndex = _xml.indexOf(kContentTag);
    int openedGroupsCount = 0;
    while (contentTagIndex != -1) {
        const auto index = contentTagIndex - 1;

        //
        // Если закрытие вложенного блока
        //
        if (_xml[index] == '/') {
            //
            // ... если есть открытые группы, то декрементируем счётчик открытых групп
            //
            if (openedGroupsCount > 0) {
                --openedGroupsCount;
            }
            //
            // ... а если открытых групп нет, то нужно дополнить xml текущим блоком
            //
            else {
                const auto parentTag = readParentTag(contentTagIndex);
                preparedXml.prepend(tagTemplate.arg(kContentTag));
                preparedXml.prepend(tagTemplate.arg(parentTag));
            }
        }
        //
        // Если открытие вложенного блока, то инкрементируем счётчик открытых групп
        //
        else {
            ++openedGroupsCount;
        }

        //
        // Переходим к следующей группе
        //
        contentTagIndex = _xml.indexOf(kContentTag, contentTagIndex + 1);
    }

    //
    // Добавляем стандартные тэги, чтобы получился более менее нормальный xml :)
    //
    const QString xmlHeader = "<?xml version=\"1.0\"?>\n";
    const QString documentHeaderOpener = QString("<%1").arg(kDocumentTag);
    const QString documentHeader = QString("%1>\n").arg(documentHeaderOpener);
    const QString documentFooter = QString("</%1>").arg(kDocumentTag);
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
