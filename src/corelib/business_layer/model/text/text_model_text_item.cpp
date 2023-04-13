#include "text_model_text_item.h"

#include "text_model.h"
#include "text_model_xml.h"

#include <business_layer/templates/templates_facade.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/string_helper.h>
#include <utils/helpers/text_helper.h>

#include <QColor>
#include <QLocale>
#include <QVariant>
#include <QXmlStreamReader>

#include <optional>


namespace BusinessLayer {

class TextModelTextItem::Implementation
{
public:
    explicit Implementation(TextModelTextItem* _q);

    /**
     * @brief Считать данные из xml
     */
    void readXml(QXmlStreamReader& _contentReader);

    /**
     * @brief Обновить закешированный xml
     */
    void updateXml();

    /**
     * @brief Сформировать xml абзаца в заданном диапазоне текста
     */
    QByteArray buildXml(int _from, int _length);


    TextModelTextItem* q = nullptr;

    /**
     * @brief Номер блока
     */
    std::optional<Number> number;

    /**
     * @brief Является ли блок декорацией
     */
    bool isCorrection = false;

    /**
     * @brief Является ли блок корректировкой вида (CONT) внутри разорванной реплики
     */
    bool isCorrectionContinued = false;

    /**
     * @brief Разорван ли блок на разрыве страниц
     */
    bool isBreakCorrectionStart = false;
    bool isBreakCorrectionEnd = false;

    /**
     * @brief Находится ли элемент в таблице, и если находится, то в какой колонке
     */
    std::optional<bool> isInFirstColumn;

    /**
     * @brief Тип параграфа
     */
    TextParagraphType paragraphType = TextParagraphType::Undefined;

    /**
     * @brief Выравнивание текста в блоке
     */
    std::optional<Qt::Alignment> alignment;

    /**
     * @brief Закладка в параграфе
     */
    std::optional<Bookmark> bookmark;

    /**
     * @brief Текст блока
     */
    QString text;

    /**
     * @brief Форматирование текста в параграфе
     */
    QVector<TextFormat> formats;

    /**
     * @brief Редакторские заметки в параграфе
     */
    QVector<ReviewMark> reviewMarks;

    /**
     * @brief Ресурсы блока
     */
    QVector<ResourceMark> resourceMarks;

    /**
     * @brief Ревизии в блоке
     */
    QVector<Revision> revisions;

    /**
     * @brief Закешированный XML блока
     */
    QByteArray xml;
};

TextModelTextItem::Implementation::Implementation(TextModelTextItem* _q)
    : q(_q)
{
}

void TextModelTextItem::Implementation::readXml(QXmlStreamReader& _contentReader)
{
    paragraphType = textParagraphTypeFromString(_contentReader.name().toString());
    Q_ASSERT(paragraphType != TextParagraphType::Undefined);

    auto currentTag = xml::readNextElement(_contentReader);

    if (currentTag == xml::kParametersTag) {
        if (_contentReader.attributes().hasAttribute(xml::kAlignAttribute)) {
            alignment = alignmentFromString(
                _contentReader.attributes().value(xml::kAlignAttribute).toString());
        }
        if (_contentReader.attributes().hasAttribute(xml::kInFirstColumnAttribute)) {
            isInFirstColumn
                = _contentReader.attributes().value(xml::kInFirstColumnAttribute).toString()
                == "true";
        }
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kBookmarkTag) {
        const auto attributes = _contentReader.attributes();
        bookmark = { attributes.value(xml::kColorAttribute).toString(),
                     TextHelper::fromHtmlEscaped(xml::readContent(_contentReader).toString()) };
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kValueTag) {
        text = TextHelper::fromHtmlEscaped(xml::readContent(_contentReader).toString());
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kReviewMarksTag) {
        do {
            currentTag = xml::readNextElement(_contentReader);

            //
            // Прекращаем обработку, если дошли до конца редакторских заметок
            //
            if (currentTag == xml::kReviewMarksTag && _contentReader.isEndElement()) {
                currentTag = xml::readNextElement(_contentReader);
                break;
            }
            //
            // Считываем заметки
            //
            else if (currentTag == xml::kReviewMarkTag) {
                const auto reviewMarkAttributes = _contentReader.attributes();
                ReviewMark reviewMark;
                reviewMark.from = reviewMarkAttributes.value(xml::kFromAttribute).toInt();
                reviewMark.length = reviewMarkAttributes.value(xml::kLengthAttribute).toInt();
                if (reviewMarkAttributes.hasAttribute(xml::kColorAttribute)) {
                    reviewMark.textColor
                        = reviewMarkAttributes.value(xml::kColorAttribute).toString();
                }
                if (reviewMarkAttributes.hasAttribute(xml::kBackgroundColorAttribute)) {
                    reviewMark.backgroundColor
                        = reviewMarkAttributes.value(xml::kBackgroundColorAttribute).toString();
                }
                reviewMark.isDone = reviewMarkAttributes.hasAttribute(xml::kDoneAttribute);

                do {
                    currentTag = xml::readNextElement(_contentReader);
                    if (currentTag != xml::kCommentTag) {
                        break;
                    }

                    const auto commentAttributes = _contentReader.attributes();
                    reviewMark.comments.append(
                        { TextHelper::fromHtmlEscaped(
                              commentAttributes.value(xml::kAuthorAttribute).toString()),
                          TextHelper::fromHtmlEscaped(
                              commentAttributes.value(xml::kEmailAttribute).toString()),
                          commentAttributes.value(xml::kDateAttribute).toString(),
                          TextHelper::fromHtmlEscaped(xml::readContent(_contentReader).toString()),
                          commentAttributes.hasAttribute(xml::kIsCommentEditedAttribute) });

                    xml::readNextElement(_contentReader); // end
                } while (!_contentReader.atEnd());

                reviewMarks.append(reviewMark);
            }
        } while (!_contentReader.atEnd());
    }

    if (currentTag == xml::kFormatsTag) {
        do {
            currentTag = xml::readNextElement(_contentReader);

            //
            // Прекращаем обработку, если дошли до конца форматов
            //
            if (currentTag == xml::kFormatsTag && _contentReader.isEndElement()) {
                currentTag = xml::readNextElement(_contentReader);
                break;
            }

            else if (currentTag == xml::kFormatTag) {
                const auto formatAttributes = _contentReader.attributes();
                TextFormat format;
                format.from = formatAttributes.value(xml::kFromAttribute).toInt();
                format.length = formatAttributes.value(xml::kLengthAttribute).toInt();
                format.isBold = formatAttributes.hasAttribute(xml::kBoldAttribute);
                format.isItalic = formatAttributes.hasAttribute(xml::kItalicAttribute);
                format.isUnderline = formatAttributes.hasAttribute(xml::kUnderlineAttribute);
                format.isStrikethrough
                    = formatAttributes.hasAttribute(xml::kStrikethroughAttribute);
                if (formatAttributes.hasAttribute(xml::kFontFamilyAttribute)
                    && formatAttributes.hasAttribute(xml::kFontSizeAttribute)) {
                    format.font.family
                        = formatAttributes.value(xml::kFontFamilyAttribute).toString();
                    format.font.size = formatAttributes.value(xml::kFontSizeAttribute).toInt();
                }

                formats.append(format);
            }

            xml::readNextElement(_contentReader); // end
        } while (!_contentReader.atEnd());
    }

    if (currentTag == xml::kResourceMarksTag) {
        do {
            currentTag = xml::readNextElement(_contentReader);

            //
            // Прекращаем обработку, если дошли до конца ресурсов
            //
            if (currentTag == xml::kResourceMarksTag && _contentReader.isEndElement()) {
                currentTag = xml::readNextElement(_contentReader);
                break;
            }

            else if (currentTag == xml::kResourceMarkTag) {
                const auto formatAttributes = _contentReader.attributes();
                ResourceMark format;
                format.from = formatAttributes.value(xml::kFromAttribute).toInt();
                format.length = formatAttributes.value(xml::kLengthAttribute).toInt();
                format.uuid = formatAttributes.value(xml::kUuidAttribute).toString();

                resourceMarks.append(format);
            }

            xml::readNextElement(_contentReader); // end
        } while (!_contentReader.atEnd());
    }

    if (currentTag == xml::kRevisionsTag) {
        Q_ASSERT(false);
        //        auto revisionNode = revisionsNode.firstChildElement();
        //        while (!revisionNode.isNull()) {
        //            revisions.append({{ revisionNode.attribute(xml::kFromAttribute).toInt(),
        //                                revisionNode.attribute(xml::kLengthAttribute).toInt()},
        //                               revisionNode.attribute(xml::kColorAttribute) });
        //            //
        //            revisionNode = revisionNode.nextSiblingElement();
        //        }
    }

    currentTag = q->readCustomContent(_contentReader);

    xml::readNextElement(_contentReader); // next
}

void TextModelTextItem::Implementation::updateXml()
{
    xml = buildXml(0, q->textToSave().length());
}

QByteArray TextModelTextItem::Implementation::buildXml(int _from, int _length)
{
    if (isCorrection) {
        return {};
    }

    const auto _end = _from + _length;

    QByteArray xml;
    xml += QString("<%1>").arg(toString(paragraphType)).toUtf8();
    if (alignment.has_value() || isInFirstColumn.has_value()) {
        xml += QString("<%1%2%3/>")
                   .arg(xml::kParametersTag,
                        (alignment.has_value()
                             ? QString(" %1=\"%2\"").arg(xml::kAlignAttribute, toString(*alignment))
                             : ""),
                        (isInFirstColumn.has_value() ? QString(" %1=\"%2\"")
                                                           .arg(xml::kInFirstColumnAttribute,
                                                                *isInFirstColumn ? "true" : "false")
                                                     : ""))
                   .toUtf8();
    }
    if (bookmark.has_value()) {
        xml += QString("<%1 %2=\"%3\"><![CDATA[%4]]></%1>")
                   .arg(xml::kBookmarkTag, xml::kColorAttribute, bookmark->color.name(),
                        TextHelper::toHtmlEscaped(bookmark->name))
                   .toUtf8();
    }
    xml += QString("<%1><![CDATA[%2]]></%1>")
               .arg(xml::kValueTag, TextHelper::toHtmlEscaped(q->textToSave().mid(_from, _length)))
               .toUtf8();

    //
    // Сохранить редакторские заметки
    //
    QVector<ReviewMark> reviewMarksToSave;
    for (const auto& reviewMark : std::as_const(reviewMarks)) {
        if (reviewMark.from >= _end) {
            continue;
        }

        //
        // Корректируем заметки, которые будут сохранены,
        // т.к. начало и конец сохраняемого блока могут отличаться
        //
        auto reviewMarkToSave = reviewMark;
        if (reviewMark.from >= _from) {
            reviewMarkToSave.from -= _from;
        } else {
            reviewMarkToSave.from = 0;
            reviewMarkToSave.length -= _from - reviewMark.from;
        }
        if (reviewMark.end() > _end) {
            reviewMarkToSave.length -= reviewMark.end() - _end;
        }
        reviewMarksToSave.append(reviewMarkToSave);
    }
    //
    // Собственно сохраняем
    //
    if (!reviewMarksToSave.isEmpty()) {
        xml += QString("<%1>").arg(xml::kReviewMarksTag).toUtf8();
        for (const auto& reviewMark : std::as_const(reviewMarksToSave)) {
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6%7%9")
                       .arg(xml::kReviewMarkTag, xml::kFromAttribute,
                            QString::number(reviewMark.from), xml::kLengthAttribute,
                            QString::number(reviewMark.length),
                            (reviewMark.textColor.isValid()
                                 ? QString(" %1=\"%2\"")
                                       .arg(xml::kColorAttribute, reviewMark.textColor.name())
                                 : ""),
                            (reviewMark.backgroundColor.isValid()
                                 ? QString(" %1=\"%2\"")
                                       .arg(xml::kBackgroundColorAttribute,
                                            reviewMark.backgroundColor.name())
                                 : ""),
                            (reviewMark.isDone ? QString(" %1=\"true\"").arg(xml::kDoneAttribute)
                                               : ""))
                       .toUtf8();
            if (!reviewMark.comments.isEmpty()) {
                xml += ">";
                for (const auto& comment : std::as_const(reviewMark.comments)) {
                    xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\"%8><![CDATA[%9]]></%1>")
                               .arg(xml::kCommentTag, xml::kAuthorAttribute,
                                    TextHelper::toHtmlEscaped(comment.author), xml::kEmailAttribute,
                                    TextHelper::toHtmlEscaped(comment.authorEmail),
                                    xml::kDateAttribute, comment.date,
                                    (comment.isEdited ? QString(" %1=\"true\"")
                                                            .arg(xml::kIsCommentEditedAttribute)
                                                      : ""),
                                    TextHelper::toHtmlEscaped(comment.text))
                               .toUtf8();
                }
                xml += QString("</%1>").arg(xml::kReviewMarkTag).toUtf8();
            } else {
                xml += "/>";
            }
        }
        xml += QString("</%1>").arg(xml::kReviewMarksTag).toUtf8();
    }

    //
    // Сохраняем форматированое блока
    //
    QVector<TextFormat> formatsToSave;
    for (const auto& format : std::as_const(formats)) {
        if (format.from >= _end) {
            continue;
        }

        //
        // Корректируем заметки, которые будут сохранены,
        // т.к. начало и конец сохраняемого блока могут отличаться
        //
        auto formatToSave = format;
        if (format.from >= _from) {
            formatToSave.from -= _from;
        } else {
            formatToSave.from = 0;
            formatToSave.length -= _from - format.from;
        }
        if (format.end() > _end) {
            formatToSave.length -= format.end() - _end;
        }
        formatsToSave.append(formatToSave);
    }
    //
    // Собственно сохраняем
    //
    if (!formatsToSave.isEmpty()) {
        xml += QString("<%1>").arg(xml::kFormatsTag).toUtf8();
        for (const auto& format : std::as_const(formatsToSave)) {
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6%7%8%9%10/>")
                       .arg(xml::kFormatTag, xml::kFromAttribute, QString::number(format.from),
                            xml::kLengthAttribute, QString::number(format.length),
                            (format.isBold ? QString(" %1=\"true\"").arg(xml::kBoldAttribute) : ""),
                            (format.isItalic ? QString(" %1=\"true\"").arg(xml::kItalicAttribute)
                                             : ""),
                            (format.isUnderline
                                 ? QString(" %1=\"true\"").arg(xml::kUnderlineAttribute)
                                 : ""),
                            (format.isStrikethrough
                                 ? QString(" %1=\"true\"").arg(xml::kStrikethroughAttribute)
                                 : ""),
                            (!format.font.family.isEmpty() && format.font.size > 0
                                 ? QString(" %1=\"%2\" %3=\"%4\"")
                                       .arg(xml::kFontFamilyAttribute, format.font.family,
                                            xml::kFontSizeAttribute,
                                            QString::number(format.font.size))
                                 : ""))
                       .toUtf8();
        }
        xml += QString("</%1>").arg(xml::kFormatsTag).toUtf8();
    }

    //
    // Сохранить ресурсные заметки
    //
    QVector<ResourceMark> resourceMarksToSave;
    for (const auto& resourceMark : std::as_const(resourceMarks)) {
        if (resourceMark.from >= _end) {
            continue;
        }

        //
        // Корректируем заметки, которые будут сохранены,
        // т.к. начало и конец сохраняемого блока могут отличаться
        //
        auto resourceMarkToSave = resourceMark;
        if (resourceMark.from >= _from) {
            resourceMarkToSave.from -= _from;
        } else {
            resourceMarkToSave.from = 0;
            resourceMarkToSave.length -= _from - resourceMark.from;
        }
        if (resourceMark.end() > _end) {
            resourceMarkToSave.length -= resourceMark.end() - _end;
        }
        resourceMarksToSave.append(resourceMarkToSave);
    }
    //
    // Собственно сохраняем
    //
    if (!resourceMarksToSave.isEmpty()) {
        xml += QString("<%1>").arg(xml::kResourceMarksTag).toUtf8();
        for (const auto& resourceMark : std::as_const(resourceMarksToSave)) {
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\"/>")
                       .arg(xml::kResourceMarkTag, xml::kFromAttribute,
                            QString::number(resourceMark.from), xml::kLengthAttribute,
                            QString::number(resourceMark.length), xml::kUuidAttribute,
                            resourceMark.uuid.toString())
                       .toUtf8();
        }
        xml += QString("</%1>").arg(xml::kResourceMarksTag).toUtf8();
    }

    //
    // Сохраняем ревизии блока
    //
    //    if (!revisions.isEmpty()) {
    //        xml += QString("<%1>").arg(xml::kRevisionsTag).toUtf8();
    //        for (const auto& revision : std::as_const(revisions)) {
    //            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\"/>")
    //                       .arg(xml::kRevisionTag, xml::kFromAttribute,
    //                       QString::number(revision.from),
    //                            xml::kLengthAttribute, QString::number(revision.length),
    //                            xml::kColorAttribute, revision.color.name())
    //                       .toUtf8();
    //        }
    //        xml += QString("</%1>").arg(xml::kRevisionsTag).toUtf8();
    //    }

    //
    // Сохраняем кастомные данные
    //
    xml += q->customContent();

    //
    // Закрываем блок
    //
    xml += QString("</%1>\n").arg(toString(paragraphType)).toUtf8();

    return xml;
}


// ****


int TextModelTextItem::TextPart::end() const
{
    return from + length;
}

bool TextModelTextItem::TextFormat::operator==(const TextModelTextItem::TextFormat& _other) const
{
    return from == _other.from && length == _other.length && isBold == _other.isBold
        && isItalic == _other.isItalic && isUnderline == _other.isUnderline
        && isStrikethrough == _other.isStrikethrough && font.family == _other.font.family
        && font.size == _other.font.size;
}

bool TextModelTextItem::TextFormat::isValid() const
{
    return isBold != false || isItalic != false || isUnderline != false || isStrikethrough != false
        || (!font.family.isEmpty() && font.size > 0);
}

QTextCharFormat TextModelTextItem::TextFormat::charFormat() const
{
    if (!isValid()) {
        return {};
    }

    QTextCharFormat format;
    if (isBold) {
        format.setFontWeight(QFont::Bold);
    }
    if (isItalic) {
        format.setFontItalic(true);
    }
    if (isUnderline) {
        format.setFontUnderline(true);
    }
    if (isStrikethrough) {
        format.setFontStrikeOut(true);
    }
    if (!font.family.isEmpty() && font.size > 0) {
        auto formatFont = format.font();
        formatFont.setFamily(font.family);
        formatFont.setPixelSize(font.size);
        format.setFont(formatFont);
    }
    return format;
}

bool TextModelTextItem::ReviewComment::operator==(
    const TextModelTextItem::ReviewComment& _other) const
{
    return author == _other.author && authorEmail == _other.authorEmail && date == _other.date
        && text == _other.text && isEdited == _other.isEdited;
}

bool TextModelTextItem::ReviewComment::isPartiallyEqual(const ReviewComment& _other) const
{
    return author == _other.author && authorEmail == _other.authorEmail && date == _other.date;
}

bool TextModelTextItem::ReviewMark::operator==(const TextModelTextItem::ReviewMark& _other) const
{
    return from == _other.from && length == _other.length && textColor == _other.textColor
        && backgroundColor == _other.backgroundColor && isDone == _other.isDone
        && comments == _other.comments;
}

bool TextModelTextItem::ReviewMark::isPartiallyEqual(const ReviewMark& _other) const
{
    return textColor == _other.textColor && backgroundColor == _other.backgroundColor
        && isDone == _other.isDone
        && ((comments.isEmpty() && _other.comments.isEmpty())
            || (!comments.isEmpty() && !_other.comments.isEmpty()
                && comments.constFirst().isPartiallyEqual(_other.comments.constFirst())));
}

QTextCharFormat TextModelTextItem::ReviewMark::charFormat() const
{
    QTextCharFormat format;
    format.setProperty(TextBlockStyle::PropertyIsReviewMark, true);
    if (textColor.isValid()) {
        format.setForeground(textColor);
    }
    if (backgroundColor.isValid()) {
        format.setBackground(backgroundColor);
        format.setForeground(ColorHelper::contrasted(backgroundColor));
    }
    format.setProperty(TextBlockStyle::PropertyIsDone, isDone);
    QStringList authors, emails, dates, comments, isEdited;
    for (const auto& comment : this->comments) {
        authors.append(comment.author);
        emails.append(comment.authorEmail);
        dates.append(comment.date);
        comments.append(comment.text);
        isEdited.append(QVariant(comment.isEdited).toString());
    }
    format.setProperty(TextBlockStyle::PropertyCommentsAuthors, authors);
    format.setProperty(TextBlockStyle::PropertyCommentsAuthorsEmails, emails);
    format.setProperty(TextBlockStyle::PropertyCommentsDates, dates);
    format.setProperty(TextBlockStyle::PropertyComments, comments);
    format.setProperty(TextBlockStyle::PropertyCommentsIsEdited, isEdited);
    return format;
}

bool TextModelTextItem::ResourceMark::operator==(const ResourceMark& _other) const
{
    return from == _other.from && length == _other.length && uuid == _other.uuid;
}

QTextCharFormat TextModelTextItem::ResourceMark::charFormat() const
{
    QTextCharFormat format;
    format.setProperty(TextBlockStyle::PropertyIsResourceMark, true);
    format.setProperty(TextBlockStyle::PropertyResourceUuid, uuid);
    return format;
}

bool TextModelTextItem::Revision::operator==(const Revision& _other) const
{
    return from == _other.from && length == _other.length && color == _other.color;
}

bool TextModelTextItem::Bookmark::operator==(const TextModelTextItem::Bookmark& _other) const
{
    return color == _other.color && name == _other.name;
}

bool TextModelTextItem::Bookmark::isValid() const
{
    return color.isValid();
}

TextModelTextItem::TextModelTextItem(const TextModel* _model)
    : TextModelItem(TextModelItemType::Text, _model)
    , d(new Implementation(this))
{
}

TextModelTextItem::~TextModelTextItem() = default;

int TextModelTextItem::subtype() const
{
    return static_cast<int>(paragraphType());
}

const TextParagraphType& TextModelTextItem::paragraphType() const
{
    return d->paragraphType;
}

void TextModelTextItem::setParagraphType(TextParagraphType _type)
{
    if (d->paragraphType == _type) {
        return;
    }

    d->paragraphType = _type;
    d->updateXml();
    markChanged();
}

std::optional<TextModelTextItem::Number> TextModelTextItem::number() const
{
    return d->number;
}

void TextModelTextItem::setNumber(int _number)
{
    if (d->number.has_value() && d->number->value == _number) {
        return;
    }

    const auto newNumber
        = QString(QLocale().textDirection() == Qt::LeftToRight ? "%1." : ".%1").arg(_number);
    d->number = { _number, newNumber };
    markChanged();
}

bool TextModelTextItem::isCorrection() const
{
    return d->isCorrection;
}

void TextModelTextItem::setCorrection(bool _correction)
{
    if (d->isCorrection == _correction) {
        return;
    }

    d->isCorrection = _correction;
    if (d->isCorrection) {
        d->xml.clear();
    } else {
        d->updateXml();
    }

    //
    // Не используем тут markChanged(), т.к. он игнорирует блоки корректировок, но в момент
    // изменения типа блока обычный <-> корректировка, нужно обязательно пометить его изменённым,
    // поэтому делаем тут ручками
    //
    setChanged(true);
}

bool TextModelTextItem::isCorrectionContinued() const
{
    return d->isCorrectionContinued;
}

void TextModelTextItem::setCorrectionContinued(bool _continued)
{
    if (d->isCorrectionContinued == _continued) {
        return;
    }

    d->isCorrectionContinued = _continued;
}

bool TextModelTextItem::isBreakCorrectionStart() const
{
    return d->isBreakCorrectionStart;
}

void TextModelTextItem::setBreakCorrectionStart(bool _broken)
{
    if (d->isBreakCorrectionStart == _broken) {
        return;
    }

    d->isBreakCorrectionStart = _broken;
}

bool TextModelTextItem::isBreakCorrectionEnd() const
{
    return d->isBreakCorrectionEnd;
}

void TextModelTextItem::setBreakCorrectionEnd(bool _broken)
{
    if (d->isBreakCorrectionEnd == _broken) {
        return;
    }

    d->isBreakCorrectionEnd = _broken;
}

std::optional<bool> TextModelTextItem::isInFirstColumn() const
{
    return d->isInFirstColumn;
}

void TextModelTextItem::setInFirstColumn(const std::optional<bool>& _in)
{
    if (d->isInFirstColumn == _in) {
        return;
    }

    d->isInFirstColumn = _in;
    d->updateXml();
    markChanged();
}

std::optional<Qt::Alignment> TextModelTextItem::alignment() const
{
    return d->alignment;
}

void TextModelTextItem::setAlignment(Qt::Alignment _align)
{
    if (d->alignment.has_value() && d->alignment == _align) {
        return;
    }

    d->alignment = _align;
    d->updateXml();
    markChanged();
}

void TextModelTextItem::clearAlignment()
{
    if (!d->alignment.has_value()) {
        return;
    }

    d->alignment.reset();
    d->updateXml();
    markChanged();
}

std::optional<TextModelTextItem::Bookmark> TextModelTextItem::bookmark() const
{
    return d->bookmark;
}

void TextModelTextItem::setBookmark(const TextModelTextItem::Bookmark& _bookmark)
{
    if (d->bookmark.has_value() && d->bookmark->color == _bookmark.color
        && d->bookmark->name == _bookmark.name) {
        return;
    }

    d->bookmark = _bookmark;
    d->updateXml();
    markChanged();
}

void TextModelTextItem::clearBookmark()
{
    if (!d->bookmark.has_value()) {
        return;
    }

    d->bookmark.reset();
    d->updateXml();
    markChanged();
}

const QString& TextModelTextItem::text() const
{
    return d->text;
}

void TextModelTextItem::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

    //
    // FIXME: если новый текст короче чем старый, то нужно скорректировать
    //        границы редакторских заметок, форматирования и ревизий
    //

    d->text = _text;
    d->updateXml();
    markChanged();
}

void TextModelTextItem::removeText(int _from)
{
    if (_from >= d->text.length()) {
        return;
    }

    //
    // Корректируем
    //
    // ... текст
    //
    d->text = d->text.left(_from);
    //
    // ... форматирование
    //
    for (int index = 0; index < d->formats.size(); ++index) {
        auto& format = d->formats[index];
        if (format.end() < _from) {
            continue;
        }

        if (format.from < _from) {
            format.length = _from - format.from;
            continue;
        }

        d->formats.remove(index);
        --index;
    }
    //
    // ... редакторские заметки
    //
    for (int index = 0; index < d->reviewMarks.size(); ++index) {
        auto& reviewMark = d->reviewMarks[index];
        if (reviewMark.end() < _from) {
            continue;
        }

        if (reviewMark.from < _from) {
            reviewMark.length = _from - reviewMark.from;
            continue;
        }

        d->reviewMarks.remove(index);
        --index;
    }
    //
    // ... ресурсы
    //
    for (int index = 0; index < d->resourceMarks.size(); ++index) {
        auto& format = d->resourceMarks[index];
        if (format.end() < _from) {
            continue;
        }

        if (format.from < _from) {
            format.length = _from - format.from;
            continue;
        }

        d->resourceMarks.remove(index);
        --index;
    }

    d->updateXml();
    markChanged();
}

const QVector<TextModelTextItem::TextFormat>& TextModelTextItem::formats() const
{
    return d->formats;
}

void TextModelTextItem::setFormats(const QVector<QTextLayout::FormatRange>& _formats)
{
    QVector<TextFormat> newFormats;
    const auto& currentTemplate = TemplatesFacade::textTemplate(model());
    const auto& defaultBlockFormat = currentTemplate.paragraphStyle(d->paragraphType);
    for (const auto& format : _formats) {
        if (format.start == 0 && format.length == d->text.length()
            && format.format == defaultBlockFormat.charFormat()) {
            continue;
        }

        TextFormat newFormat;
        newFormat.from = format.start;
        newFormat.length = format.length;
        if (format.format.hasProperty(QTextFormat::FontWeight)) {
            newFormat.isBold = format.format.font().bold();
        }
        if (format.format.hasProperty(QTextFormat::FontItalic)) {
            newFormat.isItalic = format.format.font().italic();
        }
        if (format.format.hasProperty(QTextFormat::TextUnderlineStyle)) {
            newFormat.isUnderline = format.format.font().underline();
        }
        if (format.format.hasProperty(QTextFormat::FontStrikeOut)) {
            newFormat.isStrikethrough = format.format.font().strikeOut();
        }
        if (format.format.font().family() != defaultBlockFormat.font().family()
            || format.format.font().pixelSize() != defaultBlockFormat.font().pixelSize()) {
            newFormat.font.family = format.format.font().family();
            newFormat.font.size = format.format.font().pixelSize();
        }

        newFormats.append(newFormat);
    }

    if (d->formats == newFormats) {
        return;
    }

    d->formats = newFormats;
    d->updateXml();
    markChanged();
}

const QVector<TextModelTextItem::ReviewMark>& TextModelTextItem::reviewMarks() const
{
    return d->reviewMarks;
}

void TextModelTextItem::setReviewMarks(const QVector<TextModelTextItem::ReviewMark>& _reviewMarks)
{
    if (d->reviewMarks == _reviewMarks) {
        return;
    }

    d->reviewMarks = _reviewMarks;
    d->updateXml();
    markChanged();
}

void TextModelTextItem::setReviewMarks(const QVector<QTextLayout::FormatRange>& _formats)
{
    QVector<ReviewMark> newReviewMarks;
    for (const auto& format : _formats) {
        if (format.format.boolProperty(TextBlockStyle::PropertyIsReviewMark) == false) {
            continue;
        }

        ReviewMark newReviewMark;
        newReviewMark.from = format.start;
        newReviewMark.length = format.length;
        if (format.format.hasProperty(QTextFormat::ForegroundBrush)) {
            newReviewMark.textColor = format.format.foreground().color();
        }
        if (format.format.hasProperty(QTextFormat::BackgroundBrush)) {
            newReviewMark.backgroundColor = format.format.background().color();
        }
        newReviewMark.isDone = format.format.boolProperty(TextBlockStyle::PropertyIsDone);
        const QStringList comments
            = format.format.property(TextBlockStyle::PropertyComments).toStringList();
        const QStringList dates
            = format.format.property(TextBlockStyle::PropertyCommentsDates).toStringList();
        const QStringList authors
            = format.format.property(TextBlockStyle::PropertyCommentsAuthors).toStringList();
        const QStringList emails
            = format.format.property(TextBlockStyle::PropertyCommentsAuthorsEmails).toStringList();
        const QStringList isEdited
            = format.format.property(TextBlockStyle::PropertyCommentsIsEdited).toStringList();
        for (int commentIndex = 0; commentIndex < comments.size(); ++commentIndex) {
            newReviewMark.comments.append({ authors.at(commentIndex), emails.at(commentIndex),
                                            dates.at(commentIndex), comments.at(commentIndex),
                                            isEdited.at(commentIndex) == "true" });
        }

        newReviewMarks.append(newReviewMark);
    }

    setReviewMarks(newReviewMarks);
}

const QVector<TextModelTextItem::ResourceMark>& TextModelTextItem::resourceMarks() const
{
    return d->resourceMarks;
}

void TextModelTextItem::setResourceMarks(const QVector<ResourceMark>& _resourceMarks)
{
    if (d->resourceMarks == _resourceMarks) {
        return;
    }

    d->resourceMarks = _resourceMarks;
    d->updateXml();
    markChanged();
}

void TextModelTextItem::setResourceMarks(const QVector<QTextLayout::FormatRange>& _formats)
{
    QVector<ResourceMark> newResourceMarks;
    for (const auto& format : _formats) {
        if (format.format.boolProperty(TextBlockStyle::PropertyIsResourceMark) == false) {
            continue;
        }

        ResourceMark newResourceMark;
        newResourceMark.from = format.start;
        newResourceMark.length = format.length;
        newResourceMark.uuid
            = format.format.property(TextBlockStyle::PropertyResourceUuid).toUuid();

        newResourceMarks.append(newResourceMark);
    }

    setResourceMarks(newResourceMarks);
}

const QVector<TextModelTextItem::Revision>& TextModelTextItem::revisions() const
{
    return d->revisions;
}

void TextModelTextItem::mergeWith(const TextModelTextItem* _other)
{
    if (_other == nullptr || _other->text().isEmpty()) {
        return;
    }

    const auto sourceTextLength = d->text.length();
    d->text += _other->text();
    for (auto reviewMark : _other->reviewMarks()) {
        reviewMark.from += sourceTextLength;
        d->reviewMarks.append(reviewMark);
    }
    for (auto format : _other->formats()) {
        format.from += sourceTextLength;
        d->formats.append(format);
    }
    for (auto resourceMark : _other->resourceMarks()) {
        resourceMark.from += sourceTextLength;
        d->resourceMarks.append(resourceMark);
    }

    d->updateXml();
    markChanged();
}

QVariant TextModelTextItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return u8"\U000F09A8";
    }

    case Qt::DisplayRole: {
        return d->text;
    }

    case TextNumberRole: {
        if (d->number.has_value()) {
            return d->number->text;
        }
        return {};
    }

    default: {
        return TextModelItem::data(_role);
    }
    }
}

void TextModelTextItem::readContent(QXmlStreamReader& _contentReader)
{
    //
    // Считываем контент
    //
    d->readXml(_contentReader);
    //
    // и обновляем собственное представление в соответствии с обновлёнными данными
    //
    d->updateXml();

    //
    // Делаем необходимую работу после изменения данных элемента
    //
    markChanged();
}

QByteArray TextModelTextItem::toXml() const
{
    return d->xml;
}

QByteArray TextModelTextItem::toXml(int _from, int _length)
{
    //
    // Для блока целиком, используем закешированные данные
    //
    if (_from == 0 && _length == d->text.length()) {
        return toXml();
    }

    return d->buildXml(_from, _length);
}

void TextModelTextItem::copyFrom(TextModelItem* _item)
{
    if (_item == nullptr || type() != _item->type()) {
        Q_ASSERT(false);
        return;
    }

    auto textItem = static_cast<TextModelTextItem*>(_item);
    d->isInFirstColumn = textItem->d->isInFirstColumn;
    d->paragraphType = textItem->d->paragraphType;
    d->alignment = textItem->d->alignment;
    d->bookmark = textItem->d->bookmark;
    d->text = textItem->d->text;
    d->reviewMarks = textItem->d->reviewMarks;
    d->resourceMarks = textItem->d->resourceMarks;
    d->formats = textItem->d->formats;
    d->revisions = textItem->d->revisions;
    d->xml = textItem->d->xml;

    markChanged();
}

bool TextModelTextItem::isEqual(TextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto textItem = static_cast<TextModelTextItem*>(_item);
    return d->isInFirstColumn == textItem->d->isInFirstColumn
        && d->paragraphType == textItem->d->paragraphType && d->alignment == textItem->d->alignment
        && d->bookmark == textItem->d->bookmark
        //
        // Сравниваем текст для сохранения, т.к. он может отличаться от текущего текста блока
        //
        && textToSave() == textItem->textToSave()
        //
        && d->reviewMarks == textItem->d->reviewMarks
        && d->resourceMarks == textItem->d->resourceMarks && d->formats == textItem->d->formats
        && d->revisions == textItem->d->revisions;
}

void TextModelTextItem::markChanged()
{
    if (isCorrection()) {
        return;
    }

    setChanged(true);
}

QString TextModelTextItem::textToSave() const
{
    return d->text;
}

} // namespace BusinessLayer
