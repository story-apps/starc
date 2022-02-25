#include "screenplay_text_model_text_item.h"

#include "screenplay_text_model.h"
#include "screenplay_text_model_xml.h"

#include <business_layer/chronometry/chronometer.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <utils/helpers/string_helper.h>
#include <utils/helpers/text_helper.h>

#include <QColor>
#include <QLocale>
#include <QVariant>
#include <QXmlStreamReader>

#include <optional>

namespace BusinessLayer {

class ScreenplayTextModelTextItem::Implementation
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
     * @brief Номер блока
     */
    std::optional<Number> number;

    /**
     * @brief Длительность сцены
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{ 0 };

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
    ScreenplayParagraphType paragraphType = ScreenplayParagraphType::UnformattedText;

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
     * @brief Редакторские заметки в параграфе
     */
    QVector<ReviewMark> reviewMarks;

    /**
     * @brief Форматирование текста в параграфе
     */
    QVector<TextFormat> formats;

    /**
     * @brief Ревизии в блоке
     */
    QVector<Revision> revisions;

    /**
     * @brief Закешированный XML блока
     */
    QByteArray xml;
};

ScreenplayTextModelTextItem::Implementation::Implementation(QXmlStreamReader& _contentReader)
{
    paragraphType = screenplayParagraphTypeFromString(_contentReader.name().toString());
    Q_ASSERT(paragraphType != ScreenplayParagraphType::Undefined);

    auto currentTag = xml::readNextElement(_contentReader);

    if (currentTag == xml::kParametersTag) {
        if (_contentReader.attributes().hasAttribute(xml::kAlignAttribute)) {
            alignment = alignmentFromString(
                _contentReader.attributes().value(xml::kAlignAttribute).toString());
        }
        if (_contentReader.attributes().hasAttribute(xml::kInFirstColumn)) {
            isInFirstColumn
                = _contentReader.attributes().value(xml::kInFirstColumn).toString() == "true";
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

                formats.append(format);
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

    xml::readNextElement(_contentReader); // next
}

void ScreenplayTextModelTextItem::Implementation::updateXml()
{
    xml = buildXml(0, text.length());
}

QByteArray ScreenplayTextModelTextItem::Implementation::buildXml(int _from, int _length)
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
                        (isInFirstColumn.has_value()
                             ? QString(" %1=\"%2\"")
                                   .arg(xml::kInFirstColumn, *isInFirstColumn ? "true" : "false")
                             : ""))
                   .toUtf8();
    }
    if (bookmark.has_value()) {
        xml += QString("<%1 %2=\"%3\"><![CDATA[%4]]></%1>")
                   .arg(xml::kBookmarkTag, xml::kColorAttribute, bookmark->color.name(),
                        TextHelper::toHtmlEscaped(bookmark->text))
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
                    xml += QString("<%1 %2=\"%3\" %4=\"%5\"%6><![CDATA[%7]]></%1>")
                               .arg(xml::kCommentTag, xml::kAuthorAttribute,
                                    TextHelper::toHtmlEscaped(comment.author), xml::kDateAttribute,
                                    comment.date,
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
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6%7%8%9/>")
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
                                 : ""))
                       .toUtf8();
        }
        xml += QString("</%1>").arg(xml::kFormatsTag).toUtf8();
    }

    //
    // Сохраняем ревизии блока
    //
    if (!revisions.isEmpty()) {
        xml += QString("<%1>").arg(xml::kRevisionsTag).toUtf8();
        for (const auto& revision : std::as_const(revisions)) {
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\"/>")
                       .arg(xml::kRevisionTag, xml::kFromAttribute, QString::number(revision.from),
                            xml::kLengthAttribute, QString::number(revision.length),
                            xml::kColorAttribute, revision.color.name())
                       .toUtf8();
        }
        xml += QString("</%1>").arg(xml::kRevisionsTag).toUtf8();
    }

    //
    // Закрываем блок
    //
    xml += QString("</%1>\n").arg(toString(paragraphType)).toUtf8();

    return xml;
}


// ****


int ScreenplayTextModelTextItem::TextPart::end() const
{
    return from + length;
}

bool ScreenplayTextModelTextItem::TextFormat::operator==(
    const ScreenplayTextModelTextItem::TextFormat& _other) const
{
    return from == _other.from && length == _other.length && isBold == _other.isBold
        && isItalic == _other.isItalic && isUnderline == _other.isUnderline
        && isStrikethrough == _other.isStrikethrough;
}

bool ScreenplayTextModelTextItem::TextFormat::isValid() const
{
    return isBold != false || isItalic != false || isUnderline != false || isStrikethrough != false;
}

QTextCharFormat ScreenplayTextModelTextItem::TextFormat::charFormat() const
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
    return format;
}

bool ScreenplayTextModelTextItem::ReviewComment::operator==(
    const ScreenplayTextModelTextItem::ReviewComment& _other) const
{
    return author == _other.author && date == _other.date && text == _other.text
        && isEdited == _other.isEdited;
}

bool ScreenplayTextModelTextItem::ReviewMark::operator==(
    const ScreenplayTextModelTextItem::ReviewMark& _other) const
{
    return from == _other.from && length == _other.length && textColor == _other.textColor
        && backgroundColor == _other.backgroundColor && isDone == _other.isDone
        && comments == _other.comments;
}

QTextCharFormat ScreenplayTextModelTextItem::ReviewMark::charFormat() const
{
    QTextCharFormat format;
    format.setProperty(ScreenplayBlockStyle::PropertyIsReviewMark, true);
    if (textColor.isValid()) {
        format.setForeground(textColor);
    }
    if (backgroundColor.isValid()) {
        format.setBackground(backgroundColor);
    }
    format.setProperty(ScreenplayBlockStyle::PropertyIsDone, isDone);
    QStringList authors, dates, comments, isEdited;
    for (const auto& comment : this->comments) {
        authors.append(comment.author);
        dates.append(comment.date);
        comments.append(comment.text);
        isEdited.append(QVariant(comment.isEdited).toString());
    }
    format.setProperty(ScreenplayBlockStyle::PropertyCommentsAuthors, authors);
    format.setProperty(ScreenplayBlockStyle::PropertyCommentsDates, dates);
    format.setProperty(ScreenplayBlockStyle::PropertyComments, comments);
    format.setProperty(ScreenplayBlockStyle::PropertyCommentsIsEdited, isEdited);
    return format;
}

bool ScreenplayTextModelTextItem::Bookmark::operator==(
    const ScreenplayTextModelTextItem::Bookmark& _other) const
{
    return color == _other.color && text == _other.text;
}

bool ScreenplayTextModelTextItem::Revision::operator==(const Revision& _other) const
{
    return from == _other.from && length == _other.length && color == _other.color;
}

ScreenplayTextModelTextItem::ScreenplayTextModelTextItem(const ScreenplayTextModel* _model)
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Text, _model)
    , d(new Implementation)
{
    d->updateXml();
}

ScreenplayTextModelTextItem::ScreenplayTextModelTextItem(const ScreenplayTextModel* _model,
                                                         QXmlStreamReader& _contentReaded)
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Text, _model)
    , d(new Implementation(_contentReaded))
{
    d->updateXml();
    updateDuration();
}

ScreenplayTextModelTextItem::~ScreenplayTextModelTextItem() = default;

std::optional<ScreenplayTextModelTextItem::Number> ScreenplayTextModelTextItem::number() const
{
    return d->number;
}

void ScreenplayTextModelTextItem::setNumber(int _number)
{
    if (d->number.has_value() && d->number->value == _number) {
        return;
    }

    const auto newNumber
        = QString(QLocale().textDirection() == Qt::LeftToRight ? "%1:" : ":%1").arg(_number);
    d->number = { _number, newNumber };
    markChanged();
}

std::chrono::milliseconds ScreenplayTextModelTextItem::duration() const
{
    return d->duration;
}

void ScreenplayTextModelTextItem::updateDuration()
{
    const auto duration = Chronometer::duration(d->paragraphType, d->text,
                                                model()->informationModel()->templateId());
    if (d->duration == duration) {
        return;
    }

    d->duration = duration;

    //
    // Помещаем изменённым для пересчёта хронометража в родительском элементе
    //
    markChanged();
}

bool ScreenplayTextModelTextItem::isCorrection() const
{
    return d->isCorrection;
}

void ScreenplayTextModelTextItem::setCorrection(bool _correction)
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
}

bool ScreenplayTextModelTextItem::isCorrectionContinued() const
{
    return d->isCorrectionContinued;
}

void ScreenplayTextModelTextItem::setCorrectionContinued(bool _continued)
{
    if (d->isCorrectionContinued == _continued) {
        return;
    }

    d->isCorrectionContinued = _continued;
}

bool ScreenplayTextModelTextItem::isBreakCorrectionStart() const
{
    return d->isBreakCorrectionStart;
}

void ScreenplayTextModelTextItem::setBreakCorrectionStart(bool _broken)
{
    if (d->isBreakCorrectionStart == _broken) {
        return;
    }

    d->isBreakCorrectionStart = _broken;
}

bool ScreenplayTextModelTextItem::isBreakCorrectionEnd() const
{
    return d->isBreakCorrectionEnd;
}

void ScreenplayTextModelTextItem::setBreakCorrectionEnd(bool _broken)
{
    if (d->isBreakCorrectionEnd == _broken) {
        return;
    }

    d->isBreakCorrectionEnd = _broken;
}

std::optional<bool> ScreenplayTextModelTextItem::isInFirstColumn() const
{
    return d->isInFirstColumn;
}

void ScreenplayTextModelTextItem::setInFirstColumn(const std::optional<bool>& _in)
{
    if (d->isInFirstColumn == _in) {
        return;
    }

    d->isInFirstColumn = _in;
    d->updateXml();
    markChanged();
}

const ScreenplayParagraphType& ScreenplayTextModelTextItem::paragraphType() const
{
    return d->paragraphType;
}

void ScreenplayTextModelTextItem::setParagraphType(ScreenplayParagraphType _type)
{
    if (d->paragraphType == _type) {
        return;
    }

    d->paragraphType = _type;
    d->updateXml();
    updateDuration();
    markChanged();
}

std::optional<Qt::Alignment> ScreenplayTextModelTextItem::alignment() const
{
    return d->alignment;
}

void ScreenplayTextModelTextItem::setAlignment(Qt::Alignment _align)
{
    if (d->alignment.has_value() && d->alignment == _align) {
        return;
    }

    d->alignment = _align;
    d->updateXml();
    markChanged();
}

void ScreenplayTextModelTextItem::clearAlignment()
{
    if (!d->alignment.has_value()) {
        return;
    }

    d->alignment.reset();
    d->updateXml();
    markChanged();
}

std::optional<ScreenplayTextModelTextItem::Bookmark> ScreenplayTextModelTextItem::bookmark() const
{
    return d->bookmark;
}

void ScreenplayTextModelTextItem::setBookmark(
    const ScreenplayTextModelTextItem::Bookmark& _bookmark)
{
    if (d->bookmark.has_value() && d->bookmark->color == _bookmark.color
        && d->bookmark->text == _bookmark.text) {
        return;
    }

    d->bookmark = _bookmark;
    d->updateXml();
    markChanged();
}

const QString& ScreenplayTextModelTextItem::text() const
{
    return d->text;
}

void ScreenplayTextModelTextItem::setText(const QString& _text)
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
    updateDuration();
    markChanged();
}

void ScreenplayTextModelTextItem::removeText(int _from)
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
    updateDuration();
    markChanged();
}

const QVector<ScreenplayTextModelTextItem::TextFormat>& ScreenplayTextModelTextItem::formats() const
{
    return d->formats;
}

void ScreenplayTextModelTextItem::setFormats(const QVector<QTextLayout::FormatRange>& _formats)
{
    QVector<TextFormat> newFormats;
    const auto& currentTemplate
        = TemplatesFacade::screenplayTemplate(model()->informationModel()->templateId());
    const auto defaultBlockFormat = currentTemplate.paragraphStyle(d->paragraphType);
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

        newFormats.append(newFormat);
    }

    if (d->formats == newFormats) {
        return;
    }

    d->formats = newFormats;
    d->updateXml();
    markChanged();
}

const QVector<ScreenplayTextModelTextItem::ReviewMark>& ScreenplayTextModelTextItem::reviewMarks()
    const
{
    return d->reviewMarks;
}

void ScreenplayTextModelTextItem::setReviewMarks(
    const QVector<ScreenplayTextModelTextItem::ReviewMark>& _reviewMarks)
{
    if (d->reviewMarks == _reviewMarks) {
        return;
    }

    d->reviewMarks = _reviewMarks;
    d->updateXml();
    markChanged();
}

void ScreenplayTextModelTextItem::setReviewMarks(
    const QVector<QTextLayout::FormatRange>& _reviewMarks)
{
    QVector<ReviewMark> newReviewMarks;
    for (const auto& reviewMark : _reviewMarks) {
        if (reviewMark.format.boolProperty(ScreenplayBlockStyle::PropertyIsReviewMark) == false) {
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
        newReviewMark.isDone = reviewMark.format.boolProperty(ScreenplayBlockStyle::PropertyIsDone);
        const QStringList comments
            = reviewMark.format.property(ScreenplayBlockStyle::PropertyComments).toStringList();
        const QStringList dates
            = reviewMark.format.property(ScreenplayBlockStyle::PropertyCommentsDates)
                  .toStringList();
        const QStringList authors
            = reviewMark.format.property(ScreenplayBlockStyle::PropertyCommentsAuthors)
                  .toStringList();
        const QStringList isEdited
            = reviewMark.format.property(ScreenplayBlockStyle::PropertyCommentsIsEdited)
                  .toStringList();
        for (int commentIndex = 0; commentIndex < comments.size(); ++commentIndex) {
            newReviewMark.comments.append({ authors.at(commentIndex), dates.at(commentIndex),
                                            comments.at(commentIndex),
                                            isEdited.at(commentIndex) == "true" });
        }

        newReviewMarks.append(newReviewMark);
    }

    setReviewMarks(newReviewMarks);
}

const QVector<ScreenplayTextModelTextItem::Revision>& ScreenplayTextModelTextItem::revisions() const
{
    return d->revisions;
}

void ScreenplayTextModelTextItem::mergeWith(const ScreenplayTextModelTextItem* _other)
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
    updateDuration();
    markChanged();
}

QVariant ScreenplayTextModelTextItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return d->paragraphType == ScreenplayParagraphType::Shot ? u8"\U000F0332" : u8"\U000F09A8";
    }

    case Qt::DisplayRole: {
        return d->text;
    }

    default: {
        return ScreenplayTextModelItem::data(_role);
    }
    }
}

QByteArray ScreenplayTextModelTextItem::toXml() const
{
    return d->xml;
}

QByteArray ScreenplayTextModelTextItem::toXml(int _from, int _length)
{
    //
    // Для блока целиком, используем закешированные данные
    //
    if (_from == 0 && _length == d->text.length()) {
        return toXml();
    }

    return d->buildXml(_from, _length);
}

void ScreenplayTextModelTextItem::copyFrom(ScreenplayTextModelItem* _item)
{
    if (_item->type() != ScreenplayTextModelItemType::Text) {
        Q_ASSERT(false);
        return;
    }

    auto textItem = static_cast<ScreenplayTextModelTextItem*>(_item);
    d->isInFirstColumn = textItem->d->isInFirstColumn;
    d->paragraphType = textItem->d->paragraphType;
    d->alignment = textItem->d->alignment;
    d->bookmark = textItem->d->bookmark;
    d->text = textItem->d->text;
    d->reviewMarks = textItem->d->reviewMarks;
    d->formats = textItem->d->formats;
    d->revisions = textItem->d->revisions;
    d->xml = textItem->d->xml;

    updateDuration();
    markChanged();
}

bool ScreenplayTextModelTextItem::isEqual(ScreenplayTextModelItem* _item) const
{
    if (_item == nullptr || type() != _item->type()) {
        return false;
    }

    const auto textItem = static_cast<ScreenplayTextModelTextItem*>(_item);
    return d->isInFirstColumn == textItem->d->isInFirstColumn
        && d->paragraphType == textItem->d->paragraphType && d->alignment == textItem->d->alignment
        && d->bookmark == textItem->d->bookmark && d->text == textItem->d->text
        && d->reviewMarks == textItem->d->reviewMarks && d->formats == textItem->d->formats
        && d->revisions == textItem->d->revisions;
}

void ScreenplayTextModelTextItem::markChanged()
{
    if (isCorrection()) {
        return;
    }

    setChanged(true);
}

} // namespace BusinessLayer
