#include "text_model_text_item.h"

#include "text_model_xml.h"

#include <business_layer/templates/simple_text_template.h>
#include <utils/helpers/measurement_helper.h>
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
    Implementation() = default;
    explicit Implementation(QXmlStreamReader& _contentReader);

    /**
     * @brief Обновить закешированный xml
     */
    void updateXml();

    /**
     * @brief Сформировать xml абзаца в заданном диапазоне текста
     */
    QByteArray buildXml(int _from, int _length);


    /**
     * @brief Тип параграфа
     */
    TextParagraphType paragraphType = TextParagraphType::Text;

    /**
     * @brief Выравнивание текста в блоке
     */
    std::optional<Qt::Alignment> alignment;

    /**
     * @brief Текст блока
     */
    QString text;

    /**
     * @brief Редакторские заметки в параграфе
     */
    QVector<ReviewMark> reviewMarks;

    /**
     * @brief Форматирование текста в параграфе
     */
    QVector<TextFormat> formats;

    /**
     * @brief Закешированный XML блока
     */
    QByteArray xml;
};

TextModelTextItem::Implementation::Implementation(QXmlStreamReader& _contentReader)
{
    paragraphType = textParagraphTypeFromString(_contentReader.name().toString());
    Q_ASSERT(paragraphType != TextParagraphType::Undefined);

    auto currentTag = xml::readNextElement(_contentReader);

    if (currentTag == xml::kTextParametersTag) {
        if (_contentReader.attributes().hasAttribute(xml::kAlignAttribute)) {
            alignment = alignmentFromString(
                _contentReader.attributes().value(xml::kAlignAttribute).toString());
        }
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
                          commentAttributes.value(xml::kDateAttribute).toString(),
                          TextHelper::fromHtmlEscaped(
                              xml::readContent(_contentReader).toString()) });

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
                if (formatAttributes.hasAttribute(xml::kFontAttribute)) {
                    format.font = QFont(formatAttributes.value(xml::kFontAttribute).toString());
                    format.font->setPixelSize(MeasurementHelper::ptToPx(
                        formatAttributes.value(xml::kFontSizeAttribute).toDouble()));
                }
                format.isBold = formatAttributes.hasAttribute(xml::kBoldAttribute);
                format.isItalic = formatAttributes.hasAttribute(xml::kItalicAttribute);
                format.isUnderline = formatAttributes.hasAttribute(xml::kUnderlineAttribute);

                formats.append(format);
            }

            xml::readNextElement(_contentReader); // end
        } while (!_contentReader.atEnd());
    }

    xml::readNextElement(_contentReader); // next
}

void TextModelTextItem::Implementation::updateXml()
{
    xml = buildXml(0, text.length());
}

QByteArray TextModelTextItem::Implementation::buildXml(int _from, int _length)
{
    const auto _end = _from + _length;

    QByteArray xml;
    xml += QString("<%1>").arg(toString(paragraphType)).toUtf8();
    if (alignment.has_value()) {
        xml += QString("<%1%2/>")
                   .arg(xml::kTextParametersTag,
                        (alignment.has_value()
                             ? QString(" %1=\"%2\"").arg(xml::kAlignAttribute, toString(*alignment))
                             : ""))
                   .toUtf8();
    }
    xml += QString("<%1><![CDATA[%2]]></%1>")
               .arg(xml::kValueTag, TextHelper::toHtmlEscaped(text.mid(_from, _length)))
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
                    xml += QString("<%1 %2=\"%3\" %4=\"%5\"><![CDATA[%6]]></%1>")
                               .arg(xml::kCommentTag, xml::kAuthorAttribute,
                                    TextHelper::toHtmlEscaped(comment.author), xml::kDateAttribute,
                                    comment.date, TextHelper::toHtmlEscaped(comment.text))
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
        // Корректируем форматы, которые будут сохранены,
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
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6%7%8%9/>")
                       .arg(xml::kFormatTag, xml::kFromAttribute, QString::number(format.from),
                            xml::kLengthAttribute, QString::number(format.length),
                            (format.font.has_value()
                                 ? QString(" %1=\"%2\" %3=\"%4\"")
                                       .arg(xml::kFontAttribute, format.font->family(),
                                            xml::kFontSizeAttribute,
                                            QString::number(MeasurementHelper::pxToPt(
                                                format.font->pixelSize())))
                                 : ""),
                            (format.isBold ? QString(" %1=\"true\"").arg(xml::kBoldAttribute) : ""),
                            (format.isItalic ? QString(" %1=\"true\"").arg(xml::kItalicAttribute)
                                             : ""),
                            (format.isUnderline
                                 ? QString(" %1=\"true\"").arg(xml::kUnderlineAttribute)
                                 : ""))
                       .toUtf8();
        }
        xml += QString("</%1>").arg(xml::kFormatsTag).toUtf8();
    }

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
    return from == _other.from && length == _other.length && font == _other.font
        && isBold == _other.isBold && isItalic == _other.isItalic
        && isUnderline == _other.isUnderline;
}

bool TextModelTextItem::TextFormat::isValid() const
{
    return font.has_value() || isBold != false || isItalic != false || isUnderline != false;
}

QTextCharFormat TextModelTextItem::TextFormat::charFormat() const
{
    if (!isValid()) {
        return {};
    }

    QTextCharFormat format;
    if (font.has_value()) {
        format.setFont(font.value());
    }
    if (isBold) {
        format.setFontWeight(QFont::Bold);
    }
    if (isItalic) {
        format.setFontItalic(true);
    }
    if (isUnderline) {
        format.setFontUnderline(true);
    }
    return format;
}

bool TextModelTextItem::ReviewComment::operator==(
    const TextModelTextItem::ReviewComment& _other) const
{
    return author == _other.author && date == _other.date && text == _other.text;
}

bool TextModelTextItem::ReviewMark::operator==(const TextModelTextItem::ReviewMark& _other) const
{
    return from == _other.from && length == _other.length && textColor == _other.textColor
        && backgroundColor == _other.backgroundColor && isDone == _other.isDone
        && comments == _other.comments;
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
    }
    format.setProperty(TextBlockStyle::PropertyIsDone, isDone);
    QStringList authors, dates, comments;
    for (const auto& comment : this->comments) {
        authors.append(comment.author);
        dates.append(comment.date);
        comments.append(comment.text);
    }
    format.setProperty(TextBlockStyle::PropertyCommentsAuthors, authors);
    format.setProperty(TextBlockStyle::PropertyCommentsDates, dates);
    format.setProperty(TextBlockStyle::PropertyComments, comments);
    return format;
}

TextModelTextItem::TextModelTextItem()
    : TextModelItem(TextModelItemType::Text)
    , d(new Implementation)
{
    d->updateXml();
}

TextModelTextItem::TextModelTextItem(QXmlStreamReader& _contentReaded)
    : TextModelItem(TextModelItemType::Text)
    , d(new Implementation(_contentReaded))
{
    d->updateXml();
}

TextModelTextItem::~TextModelTextItem() = default;

TextParagraphType TextModelTextItem::paragraphType() const
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

const QString& TextModelTextItem::text() const
{
    return d->text;
}

void TextModelTextItem::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

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

    d->updateXml();
    markChanged();
}

const QVector<TextModelTextItem::TextFormat>& TextModelTextItem::formats() const
{
    return d->formats;
}

void TextModelTextItem::setFormats(const QVector<QTextLayout::FormatRange>& _formats,
                                   const QTextCharFormat& _blockCharFormat)
{
    QVector<TextFormat> newFormats;
    for (const auto& format : _formats) {
        if (format.start == 0 && format.length == d->text.length()
            && format.format == _blockCharFormat) {
            continue;
        }

        TextFormat newFormat;
        newFormat.from = format.start;
        newFormat.length = format.length;
        if (format.format.font() != _blockCharFormat.font()) {
            newFormat.font = format.format.font();
        }
        if (format.format.hasProperty(QTextFormat::FontWeight)) {
            newFormat.isBold = format.format.font().bold();
        }
        if (format.format.hasProperty(QTextFormat::FontItalic)) {
            newFormat.isItalic = format.format.font().italic();
        }
        if (format.format.hasProperty(QTextFormat::TextUnderlineStyle)) {
            newFormat.isUnderline = format.format.font().underline();
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

void TextModelTextItem::setReviewMarks(const QVector<QTextLayout::FormatRange>& _reviewMarks)
{
    QVector<ReviewMark> newReviewMarks;
    for (const auto& reviewMark : _reviewMarks) {
        if (reviewMark.format.boolProperty(TextBlockStyle::PropertyIsReviewMark) == false) {
            continue;
        }

        ReviewMark newReviewMark;
        newReviewMark.from = reviewMark.start;
        newReviewMark.length = reviewMark.length;
        if (reviewMark.format.hasProperty(QTextFormat::ForegroundBrush)) {
            newReviewMark.textColor = reviewMark.format.foreground().color();
        }
        if (reviewMark.format.hasProperty(QTextFormat::BackgroundBrush)) {
            newReviewMark.backgroundColor = reviewMark.format.background().color();
        }
        newReviewMark.isDone = reviewMark.format.boolProperty(TextBlockStyle::PropertyIsDone);
        const QStringList comments
            = reviewMark.format.property(TextBlockStyle::PropertyComments).toStringList();
        const QStringList dates
            = reviewMark.format.property(TextBlockStyle::PropertyCommentsDates).toStringList();
        const QStringList authors
            = reviewMark.format.property(TextBlockStyle::PropertyCommentsAuthors).toStringList();
        for (int commentIndex = 0; commentIndex < comments.size(); ++commentIndex) {
            newReviewMark.comments.append(
                { authors.at(commentIndex), dates.at(commentIndex), comments.at(commentIndex) });
        }

        newReviewMarks.append(newReviewMark);
    }

    setReviewMarks(newReviewMarks);
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

    default: {
        return TextModelItem::data(_role);
    }
    }
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
    if (_item->type() != TextModelItemType::Text) {
        Q_ASSERT(false);
        return;
    }

    auto textItem = static_cast<TextModelTextItem*>(_item);
    d->paragraphType = textItem->d->paragraphType;
    d->alignment = textItem->d->alignment;
    d->text = textItem->d->text;
    d->reviewMarks = textItem->d->reviewMarks;
    d->formats = textItem->d->formats;
    d->xml = textItem->d->xml;

    markChanged();
}

bool TextModelTextItem::isEqual(TextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto textItem = static_cast<TextModelTextItem*>(_item);
    return d->paragraphType == textItem->d->paragraphType && d->alignment == textItem->d->alignment
        && d->text == textItem->d->text && d->reviewMarks == textItem->d->reviewMarks
        && d->formats == textItem->d->formats;
}

void TextModelTextItem::markChanged()
{
    setChanged(true);
}

} // namespace BusinessLayer
