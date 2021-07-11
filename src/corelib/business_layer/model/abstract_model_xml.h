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

//
// Содержимое группирующих элементов
//
const QString kContentTag = QLatin1String("content");

/**
 * @brief Привести xml в читаемый парсером вид
 */
QString prepareXml(const QString& _xml);

//
// Вспомогательные методы для чтения контента из потока
//
QStringRef readContent(QXmlStreamReader& _reader);
QStringRef readNextElement(QXmlStreamReader& _reader);

} // namespace xml
} // namespace BusinessLayer
