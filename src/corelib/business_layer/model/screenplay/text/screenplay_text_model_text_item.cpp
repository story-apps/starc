#include "screenplay_text_model_text_item.h"

#include <business_layer/chronometry/chronometer.h>
#include <business_layer/templates/screenplay_template.h>

#include <utils/helpers/string_helper.h>
#include <utils/helpers/text_helper.h>

#include <QColor>
#include <QDomElement>
#include <QVariant>

#include <optional>

namespace BusinessLayer
{

namespace {
    const QString kBookmarkTag = QLatin1String("bm");
    const QString kValueTag = QLatin1String("v");
    const QString kReviewMarksTag = QLatin1String("rms");
    const QString kReviewMarkTag = QLatin1String("rm");
    const QString kCommentTag = QLatin1String("c");
    const QString kFormatsTag = QLatin1String("fms");
    const QString kFormatTag = QLatin1String("fm");
    const QString kRevisionsTag = QLatin1String("revs");
    const QString kRevisionTag = QLatin1String("rev");
    const QString kFromAttribute = QLatin1String("from");
    const QString kLengthAttribute = QLatin1String("length");
    const QString kColorAttribute = QLatin1String("color");
    const QString kBackgroundColorAttribute = QLatin1String("bgcolor");
    const QString kDoneAttribute = QLatin1String("done");
    const QString kAuthorAttribute = QLatin1String("author");
    const QString kDateAttribute = QLatin1String("date");
    const QString kBoldAttribute = QLatin1String("bold");
    const QString kItalicAttribute = QLatin1String("italic");
    const QString kUnderlineAttribute = QLatin1String("underline");
    const QString kAlignAttribute = QLatin1String("align");
}

class ScreenplayTextModelTextItem::Implementation
{
public:
    Implementation() = default;
    explicit Implementation(const QDomElement& _node);

    /**
     * @brief Обновить закешированный xml
     */
    void updateXml();

    /**
     * @brief Сформировать xml абзаца в заданном диапазоне текста
     */
    QByteArray buildXml(int _from, int _length);


    /**
     * @brief Номер сцены
     */
    std::optional<Number> number;

    /**
     * @brief Длительность сцены
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{0};

    /**
     * @brief Является ли блок декорацией
     */
    bool isCorrection = false;

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
    struct Bookmark {
        QColor color;
        QString text;
    };
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
    struct Revision : TextPart {
        QColor color;
    };
    QVector<Revision> revisions;

    /**
     * @brief Закешированный XML блока
     */
    QByteArray xml;
};

ScreenplayTextModelTextItem::Implementation::Implementation(const QDomElement& _node)
{
    paragraphType = screenplayParagraphTypeFromString(_node.tagName());
    Q_ASSERT(paragraphType != ScreenplayParagraphType::Undefined);

    if (_node.hasAttribute(kAlignAttribute)) {
        alignment = alignmentFromString(_node.attribute(kAlignAttribute));
    }
    const auto bookmarkNode = _node.firstChildElement(kBookmarkTag);
    if (!bookmarkNode.isNull()) {
        bookmark = { bookmarkNode.attribute(kColorAttribute),
                     bookmarkNode.text() };
    }
    const auto textNode = _node.firstChildElement(kValueTag);
    text = TextHelper::fromHtmlEscaped(textNode.text());
    const auto reviewMarksNode = _node.firstChildElement(kReviewMarksTag);
    if (!reviewMarksNode.isNull()) {
        auto reviewMarkNode = reviewMarksNode.firstChildElement();
        while (!reviewMarkNode.isNull()) {
            ReviewMark reviewMark;
            reviewMark.from = reviewMarkNode.attribute(kFromAttribute).toInt();
            reviewMark.length = reviewMarkNode.attribute(kLengthAttribute).toInt();
            if (reviewMarkNode.hasAttribute(kColorAttribute)) {
                reviewMark.textColor = reviewMarkNode.attribute(kColorAttribute);
            }
            if (reviewMarkNode.hasAttribute(kBackgroundColorAttribute)) {
                reviewMark.backgroundColor = reviewMarkNode.attribute(kBackgroundColorAttribute);
            }
            if (reviewMarkNode.hasAttribute(kDoneAttribute)) {
                reviewMark.isDone = true;
            }
            if (reviewMarkNode.hasChildNodes()) {
                auto commentNode = reviewMarkNode.firstChildElement();
                while (!commentNode.isNull()) {
                    reviewMark.comments.append({ TextHelper::fromHtmlEscaped(commentNode.attribute(kAuthorAttribute)),
                                                 commentNode.attribute(kDateAttribute),
                                                 TextHelper::fromHtmlEscaped(commentNode.text()) });
                    //
                    commentNode = commentNode.nextSiblingElement();
                }
            }
            reviewMarks.append(reviewMark);
            //
            reviewMarkNode = reviewMarkNode.nextSiblingElement();
        }
    }
    const auto formatsNode = _node.firstChildElement(kFormatsTag);
    if (!formatsNode.isNull()) {
        auto formatNode = formatsNode.firstChildElement();
        while (!formatNode.isNull()) {
            TextFormat format;
            format.from = formatNode.attribute(kFromAttribute).toInt();
            format.length = formatNode.attribute(kLengthAttribute).toInt();
            if (formatNode.hasAttribute(kBoldAttribute)) {
                format.isBold = true;
            }
            if (formatNode.hasAttribute(kItalicAttribute)) {
                format.isItalic = true;
            }
            if (formatNode.hasAttribute(kUnderlineAttribute)) {
                format.isUnderline = true;
            }
            formats.append(format);
            //
            formatNode = formatNode.nextSiblingElement();
        }
    }
    const auto revisionsNode = _node.firstChildElement(kRevisionsTag);
    if (!revisionsNode.isNull()) {
        auto revisionNode = revisionsNode.firstChildElement();
        while (!revisionNode.isNull()) {
            revisions.append({{ revisionNode.attribute(kFromAttribute).toInt(),
                                revisionNode.attribute(kLengthAttribute).toInt()},
                               revisionNode.attribute(kColorAttribute) });
            //
            revisionNode = revisionNode.nextSiblingElement();
        }
    }
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
    xml += QString("<%1%2>")
           .arg(toString(paragraphType),
                (alignment.has_value() && alignment->testFlag(Qt::AlignHorizontal_Mask)
                 ? QString(" %1=\"%2\"").arg(kAlignAttribute, toString(*alignment))
                 : "")).toUtf8();
    if (bookmark.has_value()) {
        xml += QString("<%1 %2=\"%3\"><![CDATA[%4]]></%1>")
               .arg(kBookmarkTag,
                    kColorAttribute, bookmark->color.name(),
                    TextHelper::toHtmlEscaped(bookmark->text)).toUtf8();
    }
    xml += QString("<%1><![CDATA[%2]]></%1>")
           .arg(kValueTag,
                TextHelper::toHtmlEscaped(text.mid(_from, _length))).toUtf8();

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
        xml += QString("<%1>").arg(kReviewMarksTag).toUtf8();
        for (const auto& reviewMark : std::as_const(reviewMarksToSave)) {
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6%7%9")
                   .arg(kReviewMarkTag,
                        kFromAttribute, QString::number(reviewMark.from),
                        kLengthAttribute, QString::number(reviewMark.length),
                        (reviewMark.textColor.isValid()
                         ? QString(" %1=\"%2\"").arg(kColorAttribute, reviewMark.textColor.name())
                         : ""),
                        (reviewMark.backgroundColor.isValid()
                         ? QString(" %1=\"%2\"").arg(kBackgroundColorAttribute, reviewMark.backgroundColor.name())
                         : ""),
                        (reviewMark.isDone
                         ? QString(" %1=\"true\"").arg(kDoneAttribute)
                         : "")).toUtf8();
            if (!reviewMark.comments.isEmpty()) {
                xml += ">";
                for (const auto& comment : std::as_const(reviewMark.comments)) {
                    xml += QString("<%1 %2=\"%3\" %4=\"%5\"><![CDATA[%6]]></%1>")
                           .arg(kCommentTag,
                                kAuthorAttribute, TextHelper::toHtmlEscaped(comment.author),
                                kDateAttribute, comment.date,
                                TextHelper::toHtmlEscaped(comment.text)).toUtf8();
                }
                xml += QString("</%1>").arg(kReviewMarkTag).toUtf8();
            } else {
                xml += "/>";
            }
        }
        xml += QString("</%1>").arg(kReviewMarksTag).toUtf8();
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
        xml += QString("<%1>").arg(kFormatsTag).toUtf8();
        for (const auto& format : std::as_const(formatsToSave)) {
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6%7%8/>")
                   .arg(kFormatTag,
                        kFromAttribute, QString::number(format.from),
                        kLengthAttribute, QString::number(format.length),
                        (format.isBold
                         ? QString(" %1=\"true\"").arg(kBoldAttribute)
                         : ""),
                        (format.isItalic
                         ? QString(" %1=\"true\"").arg(kItalicAttribute)
                         : ""),
                        (format.isUnderline
                         ? QString(" %1=\"true\"").arg(kUnderlineAttribute)
                         : "")).toUtf8();
        }
        xml += QString("</%1>").arg(kFormatsTag).toUtf8();
    }

    //
    // Сохраняем ревизии блока
    //
    if (!revisions.isEmpty()) {
        xml += QString("<%1>").arg(kRevisionsTag).toUtf8();
        for (const auto& revision : std::as_const(revisions)) {
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\"/>")
                   .arg(kRevisionTag,
                        kFromAttribute, QString::number(revision.from),
                        kLengthAttribute, QString::number(revision.length),
                        kColorAttribute, revision.color.name()).toUtf8();
        }
        xml += QString("</%1>").arg(kRevisionsTag).toUtf8();
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

bool ScreenplayTextModelTextItem::TextFormat::operator==(const ScreenplayTextModelTextItem::TextFormat& _other) const
{
    return from == _other.from
            && length == _other.length
            && isBold == _other.isBold
            && isItalic == _other.isItalic
            && isUnderline == _other.isUnderline;
}

bool ScreenplayTextModelTextItem::TextFormat::isValid() const
{
    return isBold != false
            || isItalic != false
            || isUnderline != false;
}

bool ScreenplayTextModelTextItem::ReviewComment::operator==(const ScreenplayTextModelTextItem::ReviewComment& _other) const
{
    return author == _other.author
            && date == _other.date
            && text == _other.text;
}

QTextCharFormat ScreenplayTextModelTextItem::ReviewMark::charFormat() const
{
    QTextCharFormat reviewFormat;
    reviewFormat.setProperty(ScreenplayBlockStyle::PropertyIsReviewMark, true);
    if (textColor.isValid()) {
        reviewFormat.setForeground(textColor);
    }
    if (backgroundColor.isValid()) {
        reviewFormat.setBackground(backgroundColor);
    }
    reviewFormat.setProperty(ScreenplayBlockStyle::PropertyIsDone, isDone);
    QStringList authors, dates, comments;
    for (const auto& comment : this->comments) {
        authors.append(comment.author);
        dates.append(comment.date);
        comments.append(comment.text);
    }
    reviewFormat.setProperty(ScreenplayBlockStyle::PropertyCommentsAuthors, authors);
    reviewFormat.setProperty(ScreenplayBlockStyle::PropertyCommentsDates, dates);
    reviewFormat.setProperty(ScreenplayBlockStyle::PropertyComments, comments);
    return reviewFormat;
}

bool ScreenplayTextModelTextItem::ReviewMark::operator==(const ScreenplayTextModelTextItem::ReviewMark& _other) const
{
    return from == _other.from
            && length == _other.length
            && textColor == _other.textColor
            && backgroundColor == _other.backgroundColor
            && isDone == _other.isDone
            && comments == _other.comments;
}

ScreenplayTextModelTextItem::ScreenplayTextModelTextItem()
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Text),
      d(new Implementation)
{
    d->updateXml();
}

ScreenplayTextModelTextItem::ScreenplayTextModelTextItem(const QDomElement& _node)
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Text),
      d(new Implementation(_node))
{
    d->updateXml();
}

ScreenplayTextModelTextItem::~ScreenplayTextModelTextItem() = default;

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

ScreenplayParagraphType ScreenplayTextModelTextItem::paragraphType() const
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

const QString& ScreenplayTextModelTextItem::text() const
{
    return d->text;
}

void ScreenplayTextModelTextItem::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

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

void ScreenplayTextModelTextItem::setFormats(const QVector<QTextLayout::FormatRange>& _formats)
{
    QVector<TextFormat> newFormats;
    for (const auto& format : _formats) {
        if (format.format.boolProperty(ScreenplayBlockStyle::PropertyIsFormatting) == false) {
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

        newFormats.append(newFormat);
    }

    if (d->formats == newFormats) {
        return;
    }

    d->formats = newFormats;
    d->updateXml();
    markChanged();
}

const QVector<ScreenplayTextModelTextItem::ReviewMark>& ScreenplayTextModelTextItem::reviewMarks() const
{
    return d->reviewMarks;
}

void ScreenplayTextModelTextItem::setReviewMarks(const QVector<ScreenplayTextModelTextItem::ReviewMark>& _reviewMarks)
{
    if (d->reviewMarks == _reviewMarks) {
        return;
    }

    d->reviewMarks = _reviewMarks;
    d->updateXml();
    markChanged();
}

void ScreenplayTextModelTextItem::setReviewMarks(const QVector<QTextLayout::FormatRange>& _reviewMarks)
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
        const QStringList comments = reviewMark.format.property(ScreenplayBlockStyle::PropertyComments).toStringList();
        const QStringList dates = reviewMark.format.property(ScreenplayBlockStyle::PropertyCommentsDates).toStringList();
        const QStringList authors = reviewMark.format.property(ScreenplayBlockStyle::PropertyCommentsAuthors).toStringList();
        for (int commentIndex = 0; commentIndex < comments.size(); ++commentIndex) {
            newReviewMark.comments.append({ authors.at(commentIndex), dates.at(commentIndex), comments.at(commentIndex) });
        }

        newReviewMarks.append(newReviewMark);
    }

    setReviewMarks(newReviewMarks);
}

const QVector<ScreenplayTextModelTextItem::TextFormat>& ScreenplayTextModelTextItem::formats() const
{
    return d->formats;
}

std::chrono::milliseconds ScreenplayTextModelTextItem::duration() const
{
    return d->duration;
}

void ScreenplayTextModelTextItem::updateDuration()
{
    const auto duration = Chronometer::duration(d->paragraphType, d->text);
    if (d->duration == duration) {
        return;
    }

    d->duration = duration;

    //
    // Помещаем изменённым для пересчёта хронометража в родительском элементе
    //
    markChanged();
}

ScreenplayTextModelTextItem::Number ScreenplayTextModelTextItem::number() const
{
    if (!d->number.has_value()) {
        return {};
    }

    return *d->number;
}

void ScreenplayTextModelTextItem::setNumber(int _number)
{
    const auto newNumber = QString("%1:").arg(_number);
    if (d->number.has_value()
        && d->number->value == newNumber) {
        return;
    }

    d->number = { newNumber };
    markChanged();
}

void ScreenplayTextModelTextItem::mergeWith(const ScreenplayTextModelTextItem* _other)
{
    if (_other == nullptr
        || _other->text().isEmpty()) {
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
    if (_role == Qt::DisplayRole) {
        return d->text;
    }

    return ScreenplayTextModelItem::data(_role);
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

void ScreenplayTextModelTextItem::markChanged()
{
    if (isCorrection()) {
        return;
    }

    setChanged(true);
}

} // namespace BusinessLayer
