#pragma once

#include <QString>
#include <QXmlStreamReader>

class QXmlStreamReader;

namespace BusinessLayer {
namespace xml {

//
// Документ
//
const QString kDocumentTag = QLatin1String("document");
const QString kMimeTypeAttribute = QLatin1String("mime-type");
const QString kVersionAttribute = QLatin1String("version");
const QString kHeaderTag = "header";

//
// Содержимое группирующих элементов
//
const QString kContentTag = QLatin1String("content");

/**
 * @brief Привести xml в читаемый парсером вид
 */
QByteArray prepareXml(const QString& _xml);

//
// Вспомогательные методы для чтения контента из потока
//
#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
QStringView readContent(QXmlStreamReader& _reader);
QStringView readNextElement(QXmlStreamReader& _reader);
#else
QStringRef readContent(QXmlStreamReader& _reader);
QStringRef readNextElement(QXmlStreamReader& _reader);
#endif

} // namespace xml
} // namespace BusinessLayer
