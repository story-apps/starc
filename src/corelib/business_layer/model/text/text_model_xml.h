#pragma once

#include <QStringRef>
#include <QXmlStreamReader>

namespace BusinessLayer
{
namespace xml
{

//
// Документ
//
const QString kDocumentTag = QLatin1String("document");
const QString kMimeTypeAttribute = QLatin1String("mime-type");
const QString kVersionAttribute = QLatin1String("version");

//
// Текст
//
const QString kParagraphTag = QLatin1String("text");
const QString kValueTag = QLatin1String("v");
const QString kFormatsTag = QLatin1String("fms");
const QString kFormatTag = QLatin1String("fm");
const QString kFromAttribute = QLatin1String("from");
const QString kLengthAttribute = QLatin1String("length");
const QString kBoldAttribute = QLatin1String("bold");
const QString kItalicAttribute = QLatin1String("italic");
const QString kUnderlineAttribute = QLatin1String("underline");
const QString kAlignAttribute = QLatin1String("align");

//
// Вспомогательные методы для чтения контента из потока
//
QStringRef readContent(QXmlStreamReader& _reader);
QStringRef readNextElement(QXmlStreamReader& _reader);

} // namespace xml
} // namespace BusinessLayer
