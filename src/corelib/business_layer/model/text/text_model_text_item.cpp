#include "text_model_text_item.h"

#include "text_model_xml.h"

#include <business_layer/templates/text_template.h>
#include <business_layer/templates/templates_facade.h>

#include <utils/helpers/string_helper.h>
#include <utils/helpers/text_helper.h>

#include <QColor>
#include <QLocale>
#include <QVariant>
#include <QXmlStreamReader>

#include <optional>

namespace BusinessLayer
{

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
     * @brief Выравнивание текста в блоке
     */
    std::optional<Qt::Alignment> alignment;

    /**
     * @brief Текст блока
     */
    QString text;

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
    if (_contentReader.attributes().hasAttribute(xml::kAlignAttribute)) {
        alignment = alignmentFromString(_contentReader.attributes().value(xml::kAlignAttribute).toString());
    }

    auto currentTag = xml::readNextElement(_contentReader);

    if (currentTag == xml::kValueTag) {
        text = TextHelper::fromHtmlEscaped(xml::readContent(_contentReader).toString());
        xml::readNextElement(_contentReader); // end
        currentTag = xml::readNextElement(_contentReader); // next
    }

    if (currentTag == xml::kFormatsTag) {
        do {
            currentTag = xml::readNextElement(_contentReader);

            //
            // Прекращаем обработку, если дошли до конца форматов
            //
            if (currentTag == xml::kFormatsTag
                && _contentReader.isEndElement()) {
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
    xml += QString("<%1%2>")
           .arg(xml::kParagraphTag,
                (alignment.has_value() && alignment->testFlag(Qt::AlignHorizontal_Mask)
                 ? QString(" %1=\"%2\"").arg(xml::kAlignAttribute, toString(*alignment))
                 : "")).toUtf8();
    xml += QString("<%1><![CDATA[%2]]></%1>")
           .arg(xml::kValueTag,
                TextHelper::toHtmlEscaped(text.mid(_from, _length))).toUtf8();

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
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6%7%8/>")
                   .arg(xml::kFormatTag,
                        xml::kFromAttribute, QString::number(format.from),
                        xml::kLengthAttribute, QString::number(format.length),
                        (format.isBold
                         ? QString(" %1=\"true\"").arg(xml::kBoldAttribute)
                         : ""),
                        (format.isItalic
                         ? QString(" %1=\"true\"").arg(xml::kItalicAttribute)
                         : ""),
                        (format.isUnderline
                         ? QString(" %1=\"true\"").arg(xml::kUnderlineAttribute)
                         : "")).toUtf8();
        }
        xml += QString("</%1>").arg(xml::kFormatsTag).toUtf8();
    }

    //
    // Закрываем блок
    //
    xml += QString("</%1>\n").arg(xml::kParagraphTag).toUtf8();

    return xml;
}


// ****


int TextModelTextItem::TextPart::end() const
{
     return from + length;
}

bool TextModelTextItem::TextFormat::operator==(const TextModelTextItem::TextFormat& _other) const
{
    return from == _other.from
            && length == _other.length
            && isBold == _other.isBold
            && isItalic == _other.isItalic
            && isUnderline == _other.isUnderline;
}

bool TextModelTextItem::TextFormat::isValid() const
{
    return isBold != false
            || isItalic != false
            || isUnderline != false;
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
    return format;
}

TextModelTextItem::TextModelTextItem()
    : d(new Implementation)
{
    d->updateXml();
}

TextModelTextItem::TextModelTextItem(QXmlStreamReader& _contentReaded)
    : d(new Implementation(_contentReaded))
{
    d->updateXml();
}

TextModelTextItem::~TextModelTextItem() = default;

std::optional<Qt::Alignment> TextModelTextItem::alignment() const
{
    return d->alignment;
}

void TextModelTextItem::setAlignment(Qt::Alignment _align)
{
    if (d->alignment.has_value()
        && d->alignment == _align) {
        return;
    }

    d->alignment = _align;
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

void TextModelTextItem::setFormats(const QVector<QTextLayout::FormatRange>& _formats)
{
    QVector<TextFormat> newFormats;
    const auto defaultBlockFormat = TemplatesFacade::textTemplate().blockStyle(TextParagraphType::Text);
    for (const auto& format : _formats) {
        if (format.format == defaultBlockFormat.charFormat()) {
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

void TextModelTextItem::mergeWith(const TextModelTextItem* _other)
{
    if (_other == nullptr
        || _other->text().isEmpty()) {
        return;
    }

    const auto sourceTextLength = d->text.length();
    d->text += _other->text();
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
            return {};
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

void TextModelTextItem::copyFrom(TextModelTextItem* _item)
{
    d->alignment = _item->d->alignment;
    d->text = _item->d->text;
    d->formats = _item->d->formats;
    d->xml = _item->d->xml;

    markChanged();
}

bool TextModelTextItem::isEqual(TextModelTextItem* _item) const
{
    return d->alignment == _item->d->alignment
            && d->text == _item->d->text
            && d->formats == _item->d->formats;
}

void TextModelTextItem::markChanged()
{
    setChanged(true);
}

} // namespace BusinessLayer
