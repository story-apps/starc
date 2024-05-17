#include "novel_markdown_importer.h"

#include <business_layer/import/import_options.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/simple_text_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QTextCursor>
#include <QXmlStreamWriter>

namespace BusinessLayer {

namespace {

/**
 * @brief Регулярные выражения для поиска типов выделения текста
 */
static const QRegularExpression kBoldChecker("(^|[^\\\\])(\\*\\*)");
static const QRegularExpression kBoldChecker2("(^|[^\\\\])(__)");
static const QRegularExpression kItalicChecker("(^|[^\\\\])(\\*)");
static const QRegularExpression kItalicChecker2("(^|[^\\\\])(_)");
static const QRegularExpression kStrikeoutChecker("(^|[^\\\\])(~~)");

/**
 * @brief Регулярное выражение для поиска символа экранирования
 */
static const QRegularExpression kEscapeingSymbolChecker("(^|[^\\\\])(\\\\)([^\\\\])");

/**
 * @brief Регулярное выражение заголовка
 */
static const QRegularExpression kHeadingChecker("^#{1,6}(\\s{1,}|$)");

/**
 * @brief Возможные типы выделения текста в markdown
 */
static const QVector<QPair<QRegularExpression, QLatin1String>> kMarkdownSelectionTypes({
    { kBoldChecker, xml::kBoldAttribute },
    { kBoldChecker2, xml::kBoldAttribute },
    { kItalicChecker, xml::kItalicAttribute },
    { kItalicChecker2, xml::kItalicAttribute },
    { kStrikeoutChecker, xml::kStrikethroughAttribute },
});

/**
 * @brief Информация о типах выделения в тексте
 */
struct SelectionTypeInText {
    QLatin1String attribute;
    int from = -1;
    int length = 0;
};


/**
 * @brief Очистить параграф от символов экранирования
 */
static void removeEscapeingSymbol(QString& _paragraphText)
{
    QRegularExpressionMatch match = kEscapeingSymbolChecker.match(_paragraphText);
    while (match.hasMatch()) {
        _paragraphText.remove(match.capturedStart(2), 1);
        match = kEscapeingSymbolChecker.match(_paragraphText);
    }
}

/**
 * @brief Найти выделения текста
 */
static int findTypeSelection(const QString& _paragraphText,
                             const QPair<QRegularExpression, QLatin1String>& _selectionType,
                             int& _typeStart, int& _typeEnd)
{
    _typeStart = -1;
    _typeEnd = -1;
    QRegularExpressionMatch match = _selectionType.first.match(_paragraphText);
    if (match.hasMatch()) {
        _typeStart = match.capturedStart(2);
        match = _selectionType.first.match(_paragraphText, _typeStart + 1);
        if (match.hasMatch()) {
            _typeEnd = match.capturedStart(2);
            return match.capturedLength(2);
        }
    }
    return 0;
};

} // namespace


NovelAbstractImporter::Document NovelMarkdownImporter::importNovels(
    const ImportOptions& _options) const
{
    //
    // Открываем файл
    //
    QFile textFile(_options.filePath);
    if (!textFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Импортируем
    //
    Document textDocument = importNovel(textFile.readAll());
    if (textDocument.name.isEmpty()) {
        textDocument.name = QFileInfo(_options.filePath).completeBaseName();
    }

    return textDocument;
}

NovelAbstractImporter::Document NovelMarkdownImporter::importNovel(const QString& _text) const
{
    if (_text.simplified().isEmpty()) {
        return {};
    }

    Document textDocument;

    //
    // Читаем plain text
    //
    // ... и пишем в документ
    //
    QXmlStreamWriter writer(&textDocument.text);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::Novel));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    const QStringList paragraphs = QString(_text).remove('\r').split('\n');
    for (const auto& paragraph : paragraphs) {
        if (paragraph.simplified().isEmpty()) {
            continue;
        }

        auto paragraphText = paragraph;

        //
        // Типы выделения текста
        //
        QVector<SelectionTypeInText> selectionTypesInText;

        //
        // Сдвинуть расположение типов выделения текста, которые уже собраны
        //
        auto movePreviousTypes = [&selectionTypesInText](int _position, int _offset) {
            for (auto& type : selectionTypesInText) {
                if (_position < type.from) {
                    type.from -= _offset;
                }
            }
        };

        //
        // Собираем типы выделения текста
        //
        for (const auto& selectionType : kMarkdownSelectionTypes) {
            int typeStart = -1;
            int typeEnd = -1;

            int offset = findTypeSelection(paragraphText, selectionType, typeStart, typeEnd);
            while (typeEnd != -1) {
                SelectionTypeInText type;

                paragraphText.remove(typeStart, offset);
                movePreviousTypes(typeStart, offset);

                typeEnd -= offset;
                paragraphText.remove(typeEnd, offset);
                movePreviousTypes(typeEnd, offset);

                type.attribute = selectionType.second;
                type.from = typeStart;
                type.length = typeEnd - typeStart;
                selectionTypesInText.append(type);

                offset = findTypeSelection(paragraphText, selectionType, typeStart, typeEnd);
            }
        }

        if (paragraph.startsWith("#")) {
            const QString stringBegin = paragraph.split(' ')[0];
            //
            // Если начало строки состоит только из '#'
            //
            if (!stringBegin.contains(QRegularExpression("[^#]"))) {
                //
                // ... то определяем уровень заголовка
                //
                const int headingLevel = stringBegin.count('#');
                switch (headingLevel) {
                case 1: {
                    writer.writeStartElement(toString(TextParagraphType::PartHeading));
                    break;
                }
                case 2:
                case 3:
                case 4:
                case 5: {
                    writer.writeStartElement(toString(TextParagraphType::ChapterHeading));
                    break;
                }
                case 6: {
                    writer.writeStartElement(toString(TextParagraphType::SceneHeading));
                    break;
                }
                default: {
                    writer.writeStartElement(toString(TextParagraphType::Text));
                    break;
                }
                }
            } else {
                writer.writeStartElement(toString(TextParagraphType::Text));
            }
            paragraphText.remove(kHeadingChecker);
            writer.writeStartElement(xml::kValueTag);
            removeEscapeingSymbol(paragraphText);
            writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
            writer.writeEndElement(); // value
        } else {
            writer.writeStartElement(toString(TextParagraphType::Text));
            writer.writeStartElement(xml::kValueTag);
            removeEscapeingSymbol(paragraphText);
            writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
            writer.writeEndElement(); // value
        }

        //
        // Пишем данные о типах выделения текста
        //
        if (!selectionTypesInText.isEmpty()) {
            writer.writeStartElement(xml::kFormatsTag);
            for (const auto& type : selectionTypesInText) {
                writer.writeStartElement(xml::kFormatTag);
                writer.writeAttribute(xml::kFromAttribute, QString::number(type.from));
                writer.writeAttribute(xml::kLengthAttribute, QString::number(type.length));
                writer.writeAttribute(type.attribute, "true");
                writer.writeEndElement(); // format
            }
            writer.writeEndElement(); // formats
        }

        writer.writeEndElement(); // block type
    }
    writer.writeEndDocument();

    return textDocument;
}

} // namespace BusinessLayer
