#include "screenplay_text_model_text_item.h"

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
     * @brief Номер сцены
     */
    std::optional<Number> number;

    /**
     * @brief Длительность сцены
     */
    std::chrono::seconds duration = std::chrono::seconds{0};

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
    if (isCorrection) {
        return;
    }

    xml.clear();
    xml += QString("<%1%2>")
           .arg(toString(paragraphType),
                (alignment.has_value() && alignment->testFlag(Qt::AlignHorizontal_Mask)
                 ? QString(" %1=\"%2\"").arg(kAlignAttribute, toString(*alignment))
                 : ""));
    if (bookmark.has_value()) {
        xml += QString("<%1 %2=\"%3\"><![CDATA[%4]]></%1>")
               .arg(kBookmarkTag,
                    kColorAttribute, bookmark->color.name(),
                    TextHelper::toHtmlEscaped(bookmark->text));
    }
    xml += QString("<%1><![CDATA[%2]]></%1>")
           .arg(kValueTag,
                TextHelper::toHtmlEscaped(text));
    if (!reviewMarks.isEmpty()) {
        xml += QString("<%1>").arg(kReviewMarksTag);
        for (const auto& reviewMarks : std::as_const(reviewMarks)) {
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6%7%9")
                   .arg(kReviewMarkTag,
                        kFromAttribute, QString::number(reviewMarks.from),
                        kLengthAttribute, QString::number(reviewMarks.length),
                        (reviewMarks.textColor.isValid()
                         ? QString(" %1=\"%2\"").arg(kColorAttribute, reviewMarks.textColor.name())
                         : ""),
                        (reviewMarks.backgroundColor.isValid()
                         ? QString(" %1=\"%2\"").arg(kBackgroundColorAttribute, reviewMarks.backgroundColor.name())
                         : ""),
                        (reviewMarks.isDone
                         ? QString(" %1=\"true\"").arg(kDoneAttribute)
                         : ""));
            if (!reviewMarks.comments.isEmpty()) {
                xml += ">";
                for (const auto& comment : std::as_const(reviewMarks.comments)) {
                    xml += QString("<%1 %2=\"%3\" %4=\"%5\"><![CDATA[%6]]></%1>")
                           .arg(kCommentTag,
                                kAuthorAttribute, TextHelper::toHtmlEscaped(comment.author),
                                kDateAttribute, comment.date,
                                TextHelper::toHtmlEscaped(comment.text));
                }
                xml += QString("</%1>").arg(kReviewMarkTag);
            } else {
                xml += "/>";
            }
        }
        xml += QString("</%1>").arg(kReviewMarksTag);
    }
    if (!formats.isEmpty()) {
        xml += QString("<%1>").arg(kFormatsTag);
        for (const auto& format : std::as_const(formats)) {
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
                         : ""));
        }
        xml += QString("</%1>").arg(kFormatsTag);
    }
    if (!revisions.isEmpty()) {
        xml += QString("<%1>").arg(kRevisionsTag);
        for (const auto& revision : std::as_const(revisions)) {
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\"/>")
                   .arg(kRevisionTag,
                        kFromAttribute, QString::number(revision.from),
                        kLengthAttribute, QString::number(revision.length),
                        kColorAttribute, revision.color.name());
        }
        xml += QString("</%1>").arg(kRevisionsTag);
    }
    xml += QString("</%1>\n").arg(toString(paragraphType));
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

std::chrono::seconds ScreenplayTextModelTextItem::duration() const
{
    return d->duration;
}

void ScreenplayTextModelTextItem::setDuration(std::chrono::seconds _duration)
{
    if (d->duration == _duration) {
        return;
    }

    d->duration = _duration;
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
    //
    // Т.к. пока мы не сохраняем номера, в указании, что произошли изменения нет смысла
    //
//    markChanged();
}

QVariant ScreenplayTextModelTextItem::data(int _role) const
{
    if (_role == Qt::DisplayRole) {
        return d->text;
    }

    return ScreenplayTextModelItem::data(_role);
}

QString ScreenplayTextModelTextItem::toXml() const
{
    return d->xml;
}

void ScreenplayTextModelTextItem::markChanged()
{
    if (isCorrection()) {
        return;
    }

    setChanged(true);
}

} // namespace BusinessLayer
