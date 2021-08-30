#include "comic_book_template.h"

#include <ui/widgets/text_edit/page/page_metrics.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/measurement_helper.h>
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

const QHash<ComicBookParagraphType, QString> kComicBookParagraphTypeToString
    = { { ComicBookParagraphType::UnformattedText, QLatin1String("unformatted_text") },
        { ComicBookParagraphType::Page, QLatin1String("page") },
        { ComicBookParagraphType::Panel, QLatin1String("panel") },
        { ComicBookParagraphType::Description, QLatin1String("description") },
        { ComicBookParagraphType::Character, QLatin1String("character") },
        { ComicBookParagraphType::Dialogue, QLatin1String("dialogue") },
        { ComicBookParagraphType::InlineNote, QLatin1String("inline_note") },
        { ComicBookParagraphType::PageSplitter, QLatin1String("page_splitter") } };

const QHash<ComicBookBlockStyle::LineSpacingType, QString> kLineSpacingToString
    = { { ComicBookBlockStyle::LineSpacingType::SingleLineSpacing, "single" },
        { ComicBookBlockStyle::LineSpacingType::OneAndHalfLineSpacing, "oneandhalf" },
        { ComicBookBlockStyle::LineSpacingType::DoubleLineSpacing, "double" },
        { ComicBookBlockStyle::LineSpacingType::FixedLineSpacing, "fixed" } };

QString toString(ComicBookBlockStyle::LineSpacingType _type)
{
    return kLineSpacingToString.value(_type);
}

ComicBookBlockStyle::LineSpacingType lineSpacingFromString(const QString& _lineSpacing)
{
    return kLineSpacingToString.key(_lineSpacing);
}

} // namespace


QString toString(ComicBookParagraphType _type)
{
    return kComicBookParagraphTypeToString.value(_type);
}

QString toDisplayString(ComicBookParagraphType _type)
{
    switch (_type) {
    case ComicBookParagraphType::Page:
        return QCoreApplication::translate("BusinessLayer::ComicBookTemplate", "Page");
    case ComicBookParagraphType::Panel:
        return QCoreApplication::translate("BusinessLayer::ComicBookTemplate", "Panel");
    case ComicBookParagraphType::Description:
        return QCoreApplication::translate("BusinessLayer::ComicBookTemplate", "Description");
    case ComicBookParagraphType::Character:
        return QCoreApplication::translate("BusinessLayer::ComicBookTemplate", "Character");
    case ComicBookParagraphType::Dialogue:
        return QCoreApplication::translate("BusinessLayer::ComicBookTemplate", "Dialogue");
    case ComicBookParagraphType::InlineNote:
        return QCoreApplication::translate("BusinessLayer::ComicBookTemplate", "Inline note");
    case ComicBookParagraphType::UnformattedText:
        return QCoreApplication::translate("BusinessLayer::ComicBookTemplate", "Unformatted text");
    default:
        return QCoreApplication::translate("BusinessLayer::ComicBookTemplate", "Undefined");
    }
}

ComicBookParagraphType comicBookParagraphTypeFromString(const QString& _text)
{
    return kComicBookParagraphTypeToString.key(_text, ComicBookParagraphType::Undefined);
}


// ****


ComicBookParagraphType ComicBookBlockStyle::forBlock(const QTextBlock& _block)
{
    ComicBookParagraphType blockType = ComicBookParagraphType::Undefined;
    if (_block.blockFormat().hasProperty(ComicBookBlockStyle::PropertyType)) {
        blockType = static_cast<ComicBookParagraphType>(
            _block.blockFormat().intProperty(ComicBookBlockStyle::PropertyType));
    }
    return blockType;
}

ComicBookParagraphType ComicBookBlockStyle::type() const
{
    return m_type;
}

void ComicBookBlockStyle::setType(ComicBookParagraphType _type)
{
    if (m_type == _type) {
        return;
    }

    m_type = _type;
    m_blockFormat.setProperty(ComicBookBlockStyle::PropertyType, static_cast<int>(_type));
    m_blockFormatOnHalfPage.setProperty(ComicBookBlockStyle::PropertyType, static_cast<int>(_type));
}

bool ComicBookBlockStyle::isActive() const
{
    return m_isActive;
}

void ComicBookBlockStyle::setActive(bool _active)
{
    m_isActive = _active;
}

bool ComicBookBlockStyle::isStartFromNewPage() const
{
    return m_isStartFromNewPage;
}

void ComicBookBlockStyle::setStartFromNewPage(bool _startFromNewPage)
{
    if (m_isStartFromNewPage == _startFromNewPage) {
        return;
    }

    m_isStartFromNewPage = _startFromNewPage;
    m_blockFormat.setPageBreakPolicy(m_isStartFromNewPage ? QTextFormat::PageBreak_AlwaysBefore
                                                          : QTextFormat::PageBreak_Auto);
    m_blockFormatOnHalfPage.setPageBreakPolicy(m_blockFormat.pageBreakPolicy());
}

QFont ComicBookBlockStyle::font() const
{
    return m_font;
}

void ComicBookBlockStyle::setFont(const QFont& _font)
{
    if (m_font == _font) {
        return;
    }

    m_font = _font;
    m_charFormat.setFont(m_font);
    updateLineHeight();
}

Qt::Alignment ComicBookBlockStyle::align() const
{
    return m_align;
}

void ComicBookBlockStyle::setAlign(Qt::Alignment _align)
{
    if (m_align == _align) {
        return;
    }

    m_align = _align;
    m_blockFormat.setAlignment(m_align);
    m_blockFormatOnHalfPage.setAlignment(m_blockFormat.alignment());
}

ComicBookBlockStyle::LineSpacingType ComicBookBlockStyle::lineSpacingType() const
{
    return m_lineSpacing.type;
}

void ComicBookBlockStyle::setLineSpacingType(ComicBookBlockStyle::LineSpacingType _type)
{
    if (m_lineSpacing.type == _type) {
        return;
    }

    m_lineSpacing.type = _type;
    updateLineHeight();
}

qreal ComicBookBlockStyle::lineSpacingValue() const
{
    return m_lineSpacing.value;
}

void ComicBookBlockStyle::setLineSpacingValue(qreal _value)
{
    if (m_lineSpacing.value == _value) {
        return;
    }

    m_lineSpacing.value = _value;
    updateLineHeight();
}

int ComicBookBlockStyle::linesBefore() const
{
    return m_linesBefore;
}

void ComicBookBlockStyle::setLinesBefore(int _linesBefore)
{
    if (m_linesBefore == _linesBefore) {
        return;
    }

    m_linesBefore = _linesBefore;
    updateTopMargin();
}

QMarginsF ComicBookBlockStyle::margins() const
{
    return m_margins;
}

void ComicBookBlockStyle::setMargins(const QMarginsF& _margins)
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

QMarginsF ComicBookBlockStyle::marginsOnHalfPage() const
{
    return m_marginsOnHalfPage;
}

void ComicBookBlockStyle::setMarginsOnHalfPage(const QMarginsF& _margins)
{
    if (m_marginsOnHalfPage.left() != _margins.left()) {
        m_marginsOnHalfPage.setLeft(_margins.left());
        m_blockFormatOnHalfPage.setLeftMargin(PageMetrics::mmToPx(m_marginsOnHalfPage.left()));
    }

    if (m_marginsOnHalfPage.right() != _margins.right()) {
        m_marginsOnHalfPage.setRight(_margins.right());
        m_blockFormatOnHalfPage.setRightMargin(PageMetrics::mmToPx(m_marginsOnHalfPage.right()));
    }
}

void ComicBookBlockStyle::setPageSplitterWidth(qreal _width)
{
    m_charFormat.setProperty(QTextFormat::TableCellLeftPadding, _width);
}

int ComicBookBlockStyle::linesAfter() const
{
    return m_linesAfter;
}

void ComicBookBlockStyle::setLinesAfter(int _linesAfter)
{
    if (m_linesAfter == _linesAfter) {
        return;
    }

    m_linesAfter = _linesAfter;
    updateBottomMargin();
}

QTextBlockFormat ComicBookBlockStyle::blockFormat(bool _onHalfPage) const
{
    return _onHalfPage ? m_blockFormatOnHalfPage : m_blockFormat;
}

void ComicBookBlockStyle::setBackgroundColor(const QColor& _color)
{
    m_blockFormat.setBackground(_color);
    m_blockFormatOnHalfPage.setBackground(m_blockFormat.background());
}

QTextCharFormat ComicBookBlockStyle::charFormat() const
{
    return m_charFormat;
}

void ComicBookBlockStyle::setTextColor(const QColor& _color)
{
    m_charFormat.setForeground(_color);
}

bool ComicBookBlockStyle::isCanModify() const
{
    return m_charFormat.boolProperty(ComicBookBlockStyle::PropertyIsCanModify);
}

ComicBookBlockStyle::ComicBookBlockStyle(const QXmlStreamAttributes& _blockAttributes)
{
    //
    // Считываем параметры
    //
    // ... тип блока и его основные параметры в стиле
    //
    m_type = comicBookParagraphTypeFromString(_blockAttributes.value("id").toString());
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
    m_marginsOnHalfPage
        = marginsFromString(_blockAttributes.value("margins_on_half_page").toString());
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
    m_blockFormatOnHalfPage.setAlignment(m_align);
    m_blockFormatOnHalfPage.setLeftMargin(PageMetrics::mmToPx(m_marginsOnHalfPage.left()));
    m_blockFormatOnHalfPage.setRightMargin(PageMetrics::mmToPx(m_marginsOnHalfPage.right()));
    m_blockFormatOnHalfPage.setPageBreakPolicy(m_blockFormat.pageBreakPolicy());
    updateLineHeight();
    //
    // ... текста
    //
    m_charFormat.setFont(m_font);

    //
    // Запомним в стиле его настройки
    //
    m_blockFormat.setProperty(ComicBookBlockStyle::PropertyType, static_cast<int>(m_type));
    m_blockFormatOnHalfPage.setProperty(ComicBookBlockStyle::PropertyType,
                                        static_cast<int>(m_type));
    m_charFormat.setProperty(ComicBookBlockStyle::PropertyIsFirstUppercase, true);
    m_charFormat.setProperty(ComicBookBlockStyle::PropertyIsCanModify, true);

    //
    // Настроим остальные характеристики
    //
    switch (m_type) {
    case ComicBookParagraphType::PageSplitter: {
        //
        // Запрещаем редактирование данного блока и отображение в нём курсора
        //
        m_charFormat.setProperty(ComicBookBlockStyle::PropertyIsCanModify, false);
        m_blockFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
        break;
    }

    default: {
        break;
    }
    }
}

void ComicBookBlockStyle::updateLineHeight()
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
    m_blockFormatOnHalfPage.setLineHeight(lineHeight, QTextBlockFormat::FixedHeight);

    updateTopMargin();
    updateBottomMargin();
}

void ComicBookBlockStyle::updateTopMargin()
{
    m_blockFormat.setTopMargin(m_blockFormat.lineHeight() * m_linesBefore
                               + PageMetrics::mmToPx(m_margins.top()));
    m_blockFormatOnHalfPage.setTopMargin(m_blockFormat.topMargin());
}

void ComicBookBlockStyle::updateBottomMargin()
{
    m_blockFormat.setBottomMargin(m_blockFormat.lineHeight() * m_linesAfter
                                  + PageMetrics::mmToPx(m_margins.bottom()));
    m_blockFormatOnHalfPage.setBottomMargin(m_blockFormat.bottomMargin());
}


// ****


void ComicBookTemplate::setIsNew()
{
    m_isDefault = false;
    m_name = QApplication::translate("BusinessLayer::ComicBookTemplate", "Copy of ") + name();
    m_id = QUuid::createUuid().toString();
    m_description.clear();
}

void ComicBookTemplate::saveToFile(const QString& _filePath) const
{
    QFile templateFile(_filePath);
    if (!templateFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QXmlStreamWriter writer(&templateFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("style");
    writer.writeAttribute("id", m_id);
    writer.writeAttribute("name", m_name);
    writer.writeAttribute("description", m_description);
    writer.writeAttribute("page_format", PageMetrics::stringFromPageSizeId(m_pageSizeId));
    writer.writeAttribute("page_margins", ::toString(m_pageMargins));
    writer.writeAttribute("page_numbers_alignment", ::toString(m_pageNumbersAlignment));
    writer.writeAttribute("left_half_of_page_width", ::toString(m_leftHalfOfPageWidthPercents));
    for (const auto& blockStyle : std::as_const(m_paragrapsStyles)) {
        if (toString(blockStyle.type()).isEmpty()) {
            continue;
        }

        writer.writeStartElement("block");
        writer.writeAttribute("id", toString(blockStyle.type()));
        writer.writeAttribute("active", ::toString(blockStyle.isActive()));
        writer.writeAttribute("starts_from_new_page", ::toString(blockStyle.isStartFromNewPage()));
        writer.writeAttribute("font_family", blockStyle.font().family());
        writer.writeAttribute("font_size",
                              ::toString(MeasurementHelper::pxToPt(blockStyle.font().pixelSize())));
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
        writer.writeAttribute("margins_on_half_page", ::toString(blockStyle.marginsOnHalfPage()));
        writer.writeAttribute("lines_after", ::toString(blockStyle.linesAfter()));
        writer.writeEndElement(); // block
    }
    writer.writeEndElement(); // style
    writer.writeEndDocument();

    templateFile.close();
}

QString ComicBookTemplate::id() const
{
    return m_id;
}

bool ComicBookTemplate::isDefault() const
{
    return m_isDefault;
}

QString ComicBookTemplate::name() const
{
    if (m_name.isEmpty()) {
        if (m_id == "world") {
            return QApplication::translate(
                "BusinessLayer::ComicBookTemplate",
                "International template (page: A4; font: Courier Prime)");
        } else if (m_id == "us") {
            return QApplication::translate("BusinessLayer::ComicBookTemplate",
                                           "US template (page: Letter; font: Courier Prime)");
        }
    }

    return m_name;
}

void ComicBookTemplate::setName(const QString& _name)
{
    m_name = _name;
}

QString ComicBookTemplate::description() const
{
    return m_description;
}

void ComicBookTemplate::setDescription(const QString& _description)
{
    m_description = _description;
}

QPageSize::PageSizeId ComicBookTemplate::pageSizeId() const
{
    return m_pageSizeId;
}

void ComicBookTemplate::setPageSizeId(QPageSize::PageSizeId _pageSizeId)
{
    m_pageSizeId = _pageSizeId;
}

QMarginsF ComicBookTemplate::pageMargins() const
{
    return m_pageMargins;
}

void ComicBookTemplate::setPageMargins(const QMarginsF& _pageMargins)
{
    m_pageMargins = _pageMargins;
}

Qt::Alignment ComicBookTemplate::pageNumbersAlignment() const
{
    return m_pageNumbersAlignment;
}

void ComicBookTemplate::setPageNumbersAlignment(Qt::Alignment _alignment)
{
    m_pageNumbersAlignment = _alignment;
}

int ComicBookTemplate::leftHalfOfPageWidthPercents() const
{
    return m_leftHalfOfPageWidthPercents;
}

void ComicBookTemplate::setLeftHalfOfPageWidthPercents(int _width)
{
    m_leftHalfOfPageWidthPercents = _width;
}

qreal ComicBookTemplate::pageSplitterWidth() const
{
    //
    // TODO: вынести в параметры шаблона
    //
    return PageMetrics::mmToPx(5);
}

ComicBookBlockStyle ComicBookTemplate::paragraphStyle(ComicBookParagraphType _forType) const
{
    return m_paragrapsStyles.value(_forType);
}

ComicBookBlockStyle ComicBookTemplate::paragraphStyle(const QTextBlock& _forBlock) const
{
    return paragraphStyle(ComicBookBlockStyle::forBlock(_forBlock));
}

void ComicBookTemplate::setParagraphStyle(const ComicBookBlockStyle& _style)
{
    m_paragrapsStyles.insert(_style.type(), _style);
}

ComicBookTemplate::ComicBookTemplate(const QString& _fromFile)
    : m_id(QUuid::createUuid().toString())
{
    load(_fromFile);
}

void ComicBookTemplate::load(const QString& _fromFile)
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
    m_leftHalfOfPageWidthPercents = templateAttributes.value("left_half_of_page_width").toInt();

    //
    // Считываем настройки оформления блоков текста
    //
    while (reader.readNextStartElement() && reader.name() == "block") {
        ComicBookBlockStyle blockStyle(reader.attributes());
        blockStyle.setPageSplitterWidth(pageSplitterWidth());
        m_paragrapsStyles.insert(blockStyle.type(), blockStyle);

        //
        // Если ещё не находимся в конце элемента, то остальное пропускаем
        //
        if (!reader.isEndElement()) {
            reader.skipCurrentElement();
        }
    }
}

} // namespace BusinessLayer
