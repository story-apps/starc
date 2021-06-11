#include "text_template.h"

#include <ui/widgets/text_edit/page/page_metrics.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/string_helper.h>
#include <utils/helpers/text_helper.h>

#include <QApplication>
#include <QFontMetricsF>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QUuid>
#include <QXmlStreamAttributes>


namespace BusinessLayer {

namespace {

const QHash<TextParagraphType, QString> kTextParagraphTypeToString
    = { { TextParagraphType::Heading1, QLatin1String("heading_1") },
        { TextParagraphType::Heading2, QLatin1String("heading_2") },
        { TextParagraphType::Heading3, QLatin1String("heading_3") },
        { TextParagraphType::Heading4, QLatin1String("heading_4") },
        { TextParagraphType::Heading5, QLatin1String("heading_5") },
        { TextParagraphType::Heading6, QLatin1String("heading_6") },
        { TextParagraphType::Text, QLatin1String("text") },
        { TextParagraphType::InlineNote, QLatin1String("inline_note") } };

const QHash<TextBlockStyle::LineSpacingType, QString> kLineSpacingToString
    = { { TextBlockStyle::LineSpacingType::SingleLineSpacing, "single" },
        { TextBlockStyle::LineSpacingType::OneAndHalfLineSpacing, "oneandhalf" },
        { TextBlockStyle::LineSpacingType::DoubleLineSpacing, "double" },
        { TextBlockStyle::LineSpacingType::FixedLineSpacing, "fixed" } };

QString toString(TextBlockStyle::LineSpacingType _type)
{
    return kLineSpacingToString.value(_type);
}

TextBlockStyle::LineSpacingType lineSpacingFromString(const QString& _lineSpacing)
{
    return kLineSpacingToString.key(_lineSpacing);
}

} // namespace


QString toString(TextParagraphType _type)
{
    return kTextParagraphTypeToString.value(_type);
}

TextParagraphType textParagraphTypeFromString(const QString& _text)
{
    return kTextParagraphTypeToString.key(_text, TextParagraphType::Undefined);
}


// ****


TextParagraphType TextBlockStyle::forBlock(const QTextBlock& _block)
{
    TextParagraphType blockType = TextParagraphType::Undefined;
    if (_block.blockFormat().hasProperty(TextBlockStyle::PropertyType)) {
        blockType = static_cast<TextParagraphType>(
            _block.blockFormat().intProperty(TextBlockStyle::PropertyType));
    }
    return blockType;
}

TextParagraphType TextBlockStyle::type() const
{
    return m_type;
}

void TextBlockStyle::setType(TextParagraphType _type)
{
    if (m_type == _type) {
        return;
    }

    m_type = _type;
    m_blockFormat.setProperty(TextBlockStyle::PropertyType, static_cast<int>(_type));
}

bool TextBlockStyle::isActive() const
{
    return m_isActive;
}

void TextBlockStyle::setActive(bool _active)
{
    m_isActive = _active;
}

bool TextBlockStyle::isStartFromNewPage() const
{
    return m_isStartFromNewPage;
}

void TextBlockStyle::setStartFromNewPage(bool _startFromNewPage)
{
    if (m_isStartFromNewPage == _startFromNewPage) {
        return;
    }

    m_isStartFromNewPage = _startFromNewPage;
    m_blockFormat.setPageBreakPolicy(m_isStartFromNewPage ? QTextFormat::PageBreak_AlwaysBefore
                                                          : QTextFormat::PageBreak_Auto);
}

QFont TextBlockStyle::font() const
{
    return m_font;
}

void TextBlockStyle::setFont(const QFont& _font)
{
    if (m_font == _font) {
        return;
    }

    m_font = _font;
    m_charFormat.setFont(m_font);
    updateLineHeight();
}

Qt::Alignment TextBlockStyle::align() const
{
    return m_align;
}

void TextBlockStyle::setAlign(Qt::Alignment _align)
{
    if (m_align == _align) {
        return;
    }

    m_align = _align;
    m_blockFormat.setAlignment(m_align);
}

TextBlockStyle::LineSpacingType TextBlockStyle::lineSpacingType() const
{
    return m_lineSpacing.type;
}

void TextBlockStyle::setLineSpacingType(TextBlockStyle::LineSpacingType _type)
{
    if (m_lineSpacing.type == _type) {
        return;
    }

    m_lineSpacing.type = _type;
    updateLineHeight();
}

qreal TextBlockStyle::lineSpacingValue() const
{
    return m_lineSpacing.value;
}

void TextBlockStyle::setLineSpacingValue(qreal _value)
{
    if (m_lineSpacing.value == _value) {
        return;
    }

    m_lineSpacing.value = _value;
    updateLineHeight();
}

int TextBlockStyle::linesBefore() const
{
    return m_linesBefore;
}

void TextBlockStyle::setLinesBefore(int _linesBefore)
{
    if (m_linesBefore == _linesBefore) {
        return;
    }

    m_linesBefore = _linesBefore;
    updateTopMargin();
}

QMarginsF TextBlockStyle::margins() const
{
    return m_margins;
}

void TextBlockStyle::setMargins(const QMarginsF& _margins)
{
    if (m_margins.left() != _margins.left()) {
        m_margins.setLeft(_margins.left());
        m_blockFormat.setLeftMargin(PageMetrics::mmToPx(m_margins.left()));
    }

    if (m_margins.top() != _margins.top()) {
        m_margins.setTop(_margins.top());
        updateTopMargin();
    }

    if (m_margins.right() != _margins.right()) {
        m_margins.setRight(_margins.right());
        m_blockFormat.setRightMargin(PageMetrics::mmToPx(m_margins.right()));
    }

    if (m_margins.bottom() != _margins.bottom()) {
        m_margins.setBottom(_margins.bottom());
        updateBottomMargin();
    }
}

int TextBlockStyle::linesAfter() const
{
    return m_linesAfter;
}

void TextBlockStyle::setLinesAfter(int _linesAfter)
{
    if (m_linesAfter == _linesAfter) {
        return;
    }

    m_linesAfter = _linesAfter;
    updateBottomMargin();
}

QTextBlockFormat TextBlockStyle::blockFormat() const
{
    return m_blockFormat;
}

void TextBlockStyle::setBackgroundColor(const QColor& _color)
{
    m_blockFormat.setBackground(_color);
}

QTextCharFormat TextBlockStyle::charFormat() const
{
    return m_charFormat;
}

void TextBlockStyle::setTextColor(const QColor& _color)
{
    m_charFormat.setForeground(_color);
}

TextBlockStyle::TextBlockStyle(const QXmlStreamAttributes& _blockAttributes)
{
    //
    // Считываем параметры
    //
    // ... тип блока и его основные параметры в стиле
    //
    m_type = textParagraphTypeFromString(_blockAttributes.value("id").toString());
    m_isActive = _blockAttributes.value("active").toString() == "true";
    m_isStartFromNewPage = _blockAttributes.value("starts_from_new_page").toString() == "true";
    //
    // ... настройки шрифта
    //
    m_font.setFamily(_blockAttributes.value("font_family").toString());
    m_font.setPixelSize(PageMetrics::ptToPx(_blockAttributes.value("font_size").toDouble()));
    //
    // ... начертание
    //
    m_font.setBold(_blockAttributes.value("bold").toString() == "true");
    m_font.setItalic(_blockAttributes.value("italic").toString() == "true");
    m_font.setUnderline(_blockAttributes.value("underline").toString() == "true");
    m_font.setCapitalization(_blockAttributes.value("uppercase").toString() == "true"
                                 ? QFont::AllUppercase
                                 : QFont::MixedCase);
    //
    // ... расположение блока
    //
    m_align = alignmentFromString(_blockAttributes.value("alignment").toString());
    m_lineSpacing.type = lineSpacingFromString(_blockAttributes.value("line_spacing").toString());
    m_lineSpacing.value = _blockAttributes.value("line_spacing_value").toDouble();
    m_linesBefore = _blockAttributes.value("lines_before").toInt();
    m_margins = marginsFromString(_blockAttributes.value("margins").toString());
    m_linesAfter = _blockAttributes.value("lines_after").toInt();

    //
    // Настроим форматы
    //
    // ... блока
    //
    m_blockFormat.setAlignment(m_align);
    m_blockFormat.setLeftMargin(PageMetrics::mmToPx(m_margins.left()));
    m_blockFormat.setRightMargin(PageMetrics::mmToPx(m_margins.right()));
    m_blockFormat.setPageBreakPolicy(m_isStartFromNewPage ? QTextFormat::PageBreak_AlwaysBefore
                                                          : QTextFormat::PageBreak_Auto);
    updateLineHeight();
    //
    // ... текста
    //
    m_charFormat.setFont(m_font);

    //
    // Запомним в стиле его настройки
    //
    m_blockFormat.setProperty(TextBlockStyle::PropertyType, static_cast<int>(m_type));
    m_charFormat.setProperty(TextBlockStyle::PropertyIsFirstUppercase, true);
}

void TextBlockStyle::updateLineHeight()
{
    qreal lineHeight = TextHelper::fineLineSpacing(m_font);
    switch (m_lineSpacing.type) {
    case LineSpacingType::FixedLineSpacing: {
        lineHeight = PageMetrics::mmToPx(m_lineSpacing.value);
        break;
    }

    case LineSpacingType::DoubleLineSpacing: {
        lineHeight *= 2.0;
        break;
    }

    case LineSpacingType::OneAndHalfLineSpacing: {
        lineHeight *= 1.5;
        break;
    }

    case LineSpacingType::SingleLineSpacing:
    default: {
        break;
    }
    }
    m_blockFormat.setLineHeight(lineHeight, QTextBlockFormat::FixedHeight);

    updateTopMargin();
    updateBottomMargin();
}

void TextBlockStyle::updateTopMargin()
{
    m_blockFormat.setTopMargin(m_blockFormat.lineHeight() * m_linesBefore
                               + PageMetrics::mmToPx(m_margins.top()));
}

void TextBlockStyle::updateBottomMargin()
{
    m_blockFormat.setBottomMargin(m_blockFormat.lineHeight() * m_linesAfter
                                  + PageMetrics::mmToPx(m_margins.bottom()));
}


// ****


void TextTemplate::setIsNew()
{
    m_isDefault = false;
    m_name.prepend(QApplication::translate("BusinessLayer::ScriptTemplate", "Copy of "));
    m_description.clear();
}

void TextTemplate::saveToFile(const QString& _filePath) const
{
    QFile templateFile(_filePath);
    if (!templateFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QXmlStreamWriter writer(&templateFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("style");
    writer.writeAttribute("name", m_name);
    writer.writeAttribute("description", m_description);
    writer.writeAttribute("page_format", PageMetrics::stringFromPageSizeId(m_pageSizeId));
    writer.writeAttribute("page_margins", ::toString(m_pageMargins));
    writer.writeAttribute("page_numbers_alignment", ::toString(m_pageNumbersAlignment));
    for (const auto& blockStyle : m_blockStyles.values()) {
        if (toString(blockStyle.type()).isEmpty()) {
            continue;
        }

        writer.writeStartElement("block");
        writer.writeAttribute("id", toString(blockStyle.type()));
        writer.writeAttribute("active", ::toString(blockStyle.isActive()));
        writer.writeAttribute("starts_from_new_page", ::toString(blockStyle.isStartFromNewPage()));
        writer.writeAttribute("font_family", blockStyle.font().family());
        writer.writeAttribute("font_size", ::toString(blockStyle.font().pointSize()));
        writer.writeAttribute("bold", ::toString(blockStyle.font().bold()));
        writer.writeAttribute("italic", ::toString(blockStyle.font().italic()));
        writer.writeAttribute("underline", ::toString(blockStyle.font().underline()));
        writer.writeAttribute(
            "uppercase", ::toString(blockStyle.font().capitalization() == QFont::AllUppercase));
        writer.writeAttribute("alignment", toString(blockStyle.align()));
        writer.writeAttribute("line_spacing", toString(blockStyle.lineSpacingType()));
        writer.writeAttribute("line_spacing_value", ::toString(blockStyle.lineSpacingValue()));
        writer.writeAttribute("lines_before", ::toString(blockStyle.linesBefore()));
        writer.writeAttribute("margins", ::toString(blockStyle.margins()));
        writer.writeAttribute("lines_after", ::toString(blockStyle.linesAfter()));
        writer.writeEndElement(); // block
    }
    writer.writeEndElement(); // style
    writer.writeEndDocument();

    templateFile.close();
}

QString TextTemplate::id() const
{
    return m_id;
}

bool TextTemplate::isDefault() const
{
    return m_isDefault;
}

QString TextTemplate::name() const
{
    if (m_name.isEmpty()) {
        if (m_id == "mono_cp_a4") {
            return QApplication::translate("BusinessLayer::TextTemplate",
                                           "Monotype template (page: A4; font: Courier Prime)");
        } else if (m_id == "mono_cn_a4") {
            return QApplication::translate("BusinessLayer::TextTemplate",
                                           "Monotype template (page: A4; font: Courier New)");
        } else if (m_id == "mono_cp_letter") {
            return QApplication::translate("BusinessLayer::TextTemplate",
                                           "Monotype template (page: Letter; font: Courier Prime)");
        }
    }

    return m_name;
}

void TextTemplate::setName(const QString& _name)
{
    m_name = _name;
}

QString TextTemplate::description() const
{
    return m_description;
}

void TextTemplate::setDescription(const QString& _description)
{
    m_description = _description;
}

QPageSize::PageSizeId TextTemplate::pageSizeId() const
{
    return m_pageSizeId;
}

void TextTemplate::setPageSizeId(QPageSize::PageSizeId _pageSizeId)
{
    m_pageSizeId = _pageSizeId;
}

QMarginsF TextTemplate::pageMargins() const
{
    return m_pageMargins;
}

void TextTemplate::setPageMargins(const QMarginsF& _pageMargins)
{
    m_pageMargins = _pageMargins;
}

Qt::Alignment TextTemplate::pageNumbersAlignment() const
{
    return m_pageNumbersAlignment;
}

void TextTemplate::setPageNumbersAlignment(Qt::Alignment _alignment)
{
    m_pageNumbersAlignment = _alignment;
}

TextBlockStyle TextTemplate::blockStyle(TextParagraphType _forType) const
{
    return m_blockStyles.value(_forType);
}

TextBlockStyle TextTemplate::blockStyle(const QTextBlock& _forBlock) const
{
    return blockStyle(TextBlockStyle::forBlock(_forBlock));
}

void TextTemplate::setBlockStyle(const TextBlockStyle& _blockStyle)
{
    m_blockStyles.insert(_blockStyle.type(), _blockStyle);
}

TextTemplate::TextTemplate(const QString& _fromFile)
    : m_id(QUuid::createUuid().toString())
{
    load(_fromFile);
}

void TextTemplate::load(const QString& _fromFile)
{
    QFile xmlData(_fromFile);
    if (!xmlData.open(QIODevice::ReadOnly)) {
        return;
    }

    QXmlStreamReader reader(&xmlData);

    //
    // Считываем данные в соответствии с заданным форматом
    //
    if (!reader.readNextStartElement() || reader.name() != "style") {
        return;
    }

    //
    // Считываем атрибуты шаблона
    //
    QXmlStreamAttributes templateAttributes = reader.attributes();
    if (templateAttributes.hasAttribute("id")) {
        m_id = templateAttributes.value("id").toString();
    }
    m_isDefault = templateAttributes.value("default").toString() == "true";
    m_name = templateAttributes.value("name").toString();
    m_description = templateAttributes.value("description").toString();
    m_pageSizeId
        = PageMetrics::pageSizeIdFromString(templateAttributes.value("page_format").toString());
    m_pageMargins = marginsFromString(templateAttributes.value("page_margins").toString());
    m_pageNumbersAlignment
        = alignmentFromString(templateAttributes.value("page_numbers_alignment").toString());

    //
    // Считываем настройки оформления блоков текста
    //
    while (reader.readNextStartElement() && reader.name() == "block") {
        const TextBlockStyle blockStyle(reader.attributes());
        m_blockStyles.insert(blockStyle.type(), blockStyle);

        //
        // Если ещё не находимся в конце элемента, то остальное пропускаем
        //
        if (!reader.isEndElement()) {
            reader.skipCurrentElement();
        }
    }
}

} // namespace BusinessLayer
