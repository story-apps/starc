#include "screenplay_template.h"

#include <ui/widgets/text_edit/page/page_metrics.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>

#include <utils/helpers/string_helper.h>
#include <utils/helpers/text_helper.h>

#include <QApplication>
#include <QFontMetricsF>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QXmlStreamAttributes>
#include <QUuid>


namespace BusinessLayer
{

namespace {

const QHash<ScreenplayParagraphType, QString> kScreenplayParagraphTypeToString
        = {{ ScreenplayParagraphType::UnformattedText, QLatin1String("unformatted_text") },
           { ScreenplayParagraphType::SceneHeading, QLatin1String("scene_heading") },
           { ScreenplayParagraphType::SceneCharacters, QLatin1String("scene_characters") },
           { ScreenplayParagraphType::Action, QLatin1String("action") },
           { ScreenplayParagraphType::Character, QLatin1String("character") },
           { ScreenplayParagraphType::Parenthetical, QLatin1String("parenthetical") },
           { ScreenplayParagraphType::Dialogue, QLatin1String("dialogue") },
           { ScreenplayParagraphType::Lyrics, QLatin1String("lyrics") },
           { ScreenplayParagraphType::Transition, QLatin1String("transition") },
           { ScreenplayParagraphType::Shot, QLatin1String("shot") },
           { ScreenplayParagraphType::InlineNote, QLatin1String("inline_note") },
           { ScreenplayParagraphType::FolderHeader, QLatin1String("folder_header") },
           { ScreenplayParagraphType::FolderFooter, QLatin1String("folder_footer") },
           { ScreenplayParagraphType::PageSplitter, QLatin1String("page_splitter") }};

const QHash<ScreenplayBlockStyle::LineSpacingType, QString> kLineSpacingToString
        = {{ ScreenplayBlockStyle::LineSpacingType::SingleLineSpacing, "single" },
           { ScreenplayBlockStyle::LineSpacingType::OneAndHalfLineSpacing, "oneandhalf" },
           { ScreenplayBlockStyle::LineSpacingType::DoubleLineSpacing, "double" },
           { ScreenplayBlockStyle::LineSpacingType::FixedLineSpacing, "fixed" }};

QString toString(ScreenplayBlockStyle::LineSpacingType _type)
{
    return kLineSpacingToString.value(_type);
}

ScreenplayBlockStyle::LineSpacingType lineSpacingFromString(const QString& _lineSpacing)
{
    return kLineSpacingToString.key(_lineSpacing);
}

} // namespace


QString toString(ScreenplayParagraphType _type)
{
    return kScreenplayParagraphTypeToString.value(_type);
}

ScreenplayParagraphType screenplayParagraphTypeFromString(const QString& _text)
{
    return kScreenplayParagraphTypeToString.key(_text, ScreenplayParagraphType::Undefined);
}


// ****


ScreenplayParagraphType ScreenplayBlockStyle::forBlock(const QTextBlock& _block)
{
    ScreenplayParagraphType blockType = ScreenplayParagraphType::Undefined;
    if (_block.blockFormat().hasProperty(ScreenplayBlockStyle::PropertyType)) {
        blockType = static_cast<ScreenplayParagraphType>(_block.blockFormat().intProperty(ScreenplayBlockStyle::PropertyType));
    }
    return blockType;
}

ScreenplayParagraphType ScreenplayBlockStyle::type() const
{
    return m_type;
}

void ScreenplayBlockStyle::setType(ScreenplayParagraphType _type)
{
    if (m_type == _type) {
        return;
    }

    m_type = _type;
    m_blockFormat.setProperty(ScreenplayBlockStyle::PropertyType, static_cast<int>(_type));
    m_blockFormatOnHalfPage.setProperty(ScreenplayBlockStyle::PropertyType, static_cast<int>(_type));
}

bool ScreenplayBlockStyle::isActive() const
{
    return m_isActive;
}

void ScreenplayBlockStyle::setActive(bool _active)
{
    m_isActive = _active;
}

bool ScreenplayBlockStyle::isStartFromNewPage() const
{
    return m_isStartFromNewPage;
}

void ScreenplayBlockStyle::setStartFromNewPage(bool _startFromNewPage)
{
    if (m_isStartFromNewPage == _startFromNewPage) {
        return;
    }

    m_isStartFromNewPage = _startFromNewPage;
    m_blockFormat.setPageBreakPolicy(m_isStartFromNewPage ? QTextFormat::PageBreak_AlwaysBefore
                                                          : QTextFormat::PageBreak_Auto);
    m_blockFormatOnHalfPage.setPageBreakPolicy(m_blockFormat.pageBreakPolicy());
}

QFont ScreenplayBlockStyle::font() const
{
    return m_font;
}

void ScreenplayBlockStyle::setFont(const QFont& _font)
{
    if (m_font == _font) {
        return;
    }

    m_font = _font;
    m_charFormat.setFont(m_font);
    updateLineHeight();
}

Qt::Alignment ScreenplayBlockStyle::align() const
{
    return m_align;
}

void ScreenplayBlockStyle::setAlign(Qt::Alignment _align)
{
    if (m_align == _align) {
        return;
    }

    m_align = _align;
    m_blockFormat.setAlignment(m_align);
    m_blockFormatOnHalfPage.setAlignment(m_blockFormat.alignment());
}

ScreenplayBlockStyle::LineSpacingType ScreenplayBlockStyle::lineSpacingType() const
{
    return m_lineSpacing.type;
}

void ScreenplayBlockStyle::setLineSpacingType(ScreenplayBlockStyle::LineSpacingType _type)
{
    if (m_lineSpacing.type == _type) {
        return;
    }

    m_lineSpacing.type = _type;
    updateLineHeight();
}

qreal ScreenplayBlockStyle::lineSpacingValue() const
{
    return m_lineSpacing.value;
}

void ScreenplayBlockStyle::setLineSpacingValue(qreal _value)
{
    if (m_lineSpacing.value == _value) {
        return;
    }

    m_lineSpacing.value = _value;
    updateLineHeight();
}

int ScreenplayBlockStyle::linesBefore() const
{
    return m_linesBefore;
}

void ScreenplayBlockStyle::setLinesBefore(int _linesBefore)
{
    if (m_linesBefore == _linesBefore) {
        return;
    }

    m_linesBefore = _linesBefore;
    updateTopMargin();
}

QMarginsF ScreenplayBlockStyle::margins() const
{
    return m_margins;
}

void ScreenplayBlockStyle::setMargins(const QMarginsF& _margins)
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

QMarginsF ScreenplayBlockStyle::marginsOnHalfPage() const
{
    return m_marginsOnHalfPage;
}

void ScreenplayBlockStyle::setMarginsOnHalfPage(const QMarginsF& _margins)
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

void ScreenplayBlockStyle::setPageSplitterWidth(qreal _width)
{
    m_charFormat.setProperty(QTextFormat::TableCellLeftPadding, _width);
}

int ScreenplayBlockStyle::linesAfter() const
{
    return m_linesAfter;
}

void ScreenplayBlockStyle::setLinesAfter(int _linesAfter)
{
    if (m_linesAfter == _linesAfter) {
        return;
    }

    m_linesAfter = _linesAfter;
    updateBottomMargin();
}

QTextBlockFormat ScreenplayBlockStyle::blockFormat(bool _onHalfPage) const
{
    return _onHalfPage ? m_blockFormatOnHalfPage : m_blockFormat;
}

void ScreenplayBlockStyle::setBackgroundColor(const QColor& _color)
{
    m_blockFormat.setBackground(_color);
    m_blockFormatOnHalfPage.setBackground(m_blockFormat.background());
}

QTextCharFormat ScreenplayBlockStyle::charFormat() const
{
    return m_charFormat;
}

void ScreenplayBlockStyle::setTextColor(const QColor& _color)
{
    m_charFormat.setForeground(_color);
}

bool ScreenplayBlockStyle::isCanModify() const
{
    return m_charFormat.boolProperty(ScreenplayBlockStyle::PropertyIsCanModify);
}

QString ScreenplayBlockStyle::prefix() const
{
    return m_charFormat.stringProperty(ScreenplayBlockStyle::PropertyPrefix);
}

QString ScreenplayBlockStyle::postfix() const
{
    return m_charFormat.stringProperty(ScreenplayBlockStyle::PropertyPostfix);
}

ScreenplayBlockStyle::ScreenplayBlockStyle(const QXmlStreamAttributes& _blockAttributes)
{
    //
    // Считываем параметры
    //
    // ... тип блока и его основные параметры в стиле
    //
    m_type = screenplayParagraphTypeFromString(_blockAttributes.value("id").toString());
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
                             ? QFont::AllUppercase : QFont::MixedCase);
    //
    // ... расположение блока
    //
    m_align = alignmentFromString(_blockAttributes.value("alignment").toString());
    m_lineSpacing.type = lineSpacingFromString(_blockAttributes.value("line_spacing").toString());
    m_lineSpacing.value = _blockAttributes.value("line_spacing_value").toDouble();
    m_linesBefore = _blockAttributes.value("lines_before").toInt();
    m_margins = marginsFromString(_blockAttributes.value("margins").toString());
    m_marginsOnHalfPage = marginsFromString(_blockAttributes.value("margins_on_half_page").toString());
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
    m_blockFormat.setProperty(ScreenplayBlockStyle::PropertyType, static_cast<int>(m_type));
    m_blockFormatOnHalfPage.setProperty(ScreenplayBlockStyle::PropertyType, static_cast<int>(m_type));
    m_charFormat.setProperty(ScreenplayBlockStyle::PropertyIsFirstUppercase, true);
    m_charFormat.setProperty(ScreenplayBlockStyle::PropertyIsCanModify, true);

    //
    // Настроим остальные характеристики
    //
    switch (m_type) {
        case ScreenplayParagraphType::Parenthetical: {
            m_charFormat.setProperty(ScreenplayBlockStyle::PropertyIsFirstUppercase, false);
            //
            // Стандартное обрамление
            //
            m_charFormat.setProperty(ScreenplayBlockStyle::PropertyPrefix, "(");
            m_charFormat.setProperty(ScreenplayBlockStyle::PropertyPostfix, ")");
            break;
        }

        case ScreenplayParagraphType::PageSplitter: {
            //
            // Запрещаем редактирование данного блока и отображение в нём курсора
            //
            m_charFormat.setProperty(ScreenplayBlockStyle::PropertyIsCanModify, false);
            m_blockFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
            break;
        }

        default: {
            break;
        }
    }
}

void ScreenplayBlockStyle::updateLineHeight()
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

void ScreenplayBlockStyle::updateTopMargin()
{
    m_blockFormat.setTopMargin(m_blockFormat.lineHeight() * m_linesBefore
                               + PageMetrics::mmToPx(m_margins.top()));
    m_blockFormatOnHalfPage.setTopMargin(m_blockFormat.topMargin());
}

void ScreenplayBlockStyle::updateBottomMargin()
{
    m_blockFormat.setBottomMargin(m_blockFormat.lineHeight() * m_linesAfter
                                  + PageMetrics::mmToPx(m_margins.bottom()));
    m_blockFormatOnHalfPage.setBottomMargin(m_blockFormat.bottomMargin());
}


// ****


void ScreenplayTemplate::setIsNew()
{
    m_isDefault = false;
    m_name.prepend(QApplication::translate("BusinessLayer::ScriptTemplate", "Copy of "));
    m_description.clear();
}

void ScreenplayTemplate::saveToFile(const QString& _filePath) const
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
    writer.writeAttribute("left_half_of_page_width", ::toString(m_leftHalfOfPageWidthPercents));
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
        writer.writeAttribute("uppercase", ::toString(blockStyle.font().capitalization() == QFont::AllUppercase));
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

QString ScreenplayTemplate::id() const
{
    return m_id;
}

bool ScreenplayTemplate::isDefault() const
{
    return m_isDefault;
}

QString ScreenplayTemplate::name() const
{
    if (m_name.isEmpty()) {
        if (m_id == "world_cp") {
            return QApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "International template (page: A4; font: Courier Prime)");
        } else if (m_id == "world_cn") {
            return QApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "International template (page: A4; font: Courier New)");
        } else if (m_id == "ar") {
            return QApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "Arabic template (page: A4; font: Courier New)");
        } else if (m_id == "he") {
            return QApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "Hebrew template (page: A4; font: Arial)");
        } else if (m_id == "ru") {
            return QApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "Russian template (page: A4; font: Courier New)");
        } else if (m_id == "us") {
            return QApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "US template (page: Letter; font: Courier Prime)");
        }
    }

    return m_name;
}

void ScreenplayTemplate::setName(const QString& _name)
{
    m_name = _name;
}

QString ScreenplayTemplate::description() const
{
    return m_description;
}

void ScreenplayTemplate::setDescription(const QString& _description)
{
    m_description = _description;
}

QPageSize::PageSizeId ScreenplayTemplate::pageSizeId() const
{
    return m_pageSizeId;
}

void ScreenplayTemplate::setPageSizeId(QPageSize::PageSizeId _pageSizeId)
{
    m_pageSizeId = _pageSizeId;
}

QMarginsF ScreenplayTemplate::pageMargins() const
{
    return m_pageMargins;
}

void ScreenplayTemplate::setPageMargins(const QMarginsF& _pageMargins)
{
    m_pageMargins = _pageMargins;
}

Qt::Alignment ScreenplayTemplate::pageNumbersAlignment() const
{
    return m_pageNumbersAlignment;
}

void ScreenplayTemplate::setPageNumbersAlignment(Qt::Alignment _alignment)
{
    m_pageNumbersAlignment = _alignment;
}

int ScreenplayTemplate::leftHalfOfPageWidthPercents() const
{
    return m_leftHalfOfPageWidthPercents;
}

void ScreenplayTemplate::setLeftHalfOfPageWidthPercents(int _width)
{
    m_leftHalfOfPageWidthPercents = _width;
}

qreal ScreenplayTemplate::pageSplitterWidth() const
{
    //
    // TODO: вынести в параметры шаблона
    //
    return PageMetrics::mmToPx(5);
}

ScreenplayBlockStyle ScreenplayTemplate::blockStyle(ScreenplayParagraphType _forType) const
{
    return m_blockStyles.value(_forType);
}

ScreenplayBlockStyle ScreenplayTemplate::blockStyle(const QTextBlock& _forBlock) const
{
    return blockStyle(ScreenplayBlockStyle::forBlock(_forBlock));
}

void ScreenplayTemplate::setBlockStyle(const ScreenplayBlockStyle& _blockStyle)
{
    m_blockStyles.insert(_blockStyle.type(), _blockStyle);
}

ScreenplayTemplate::ScreenplayTemplate(const QString& _fromFile)
    : m_id(QUuid::createUuid().toString())
{
    load(_fromFile);
}

void ScreenplayTemplate::load(const QString& _fromFile)
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
    m_pageSizeId = PageMetrics::pageSizeIdFromString(templateAttributes.value("page_format").toString());
    m_pageMargins = marginsFromString(templateAttributes.value("page_margins").toString());
    m_pageNumbersAlignment = alignmentFromString(templateAttributes.value("page_numbers_alignment").toString());
    m_leftHalfOfPageWidthPercents = templateAttributes.value("left_half_of_page_width").toInt();

    //
    // Считываем настройки оформления блоков текста
    //
    while (reader.readNextStartElement() && reader.name() == "block")
    {
        ScreenplayBlockStyle blockStyle(reader.attributes());
        blockStyle.setPageSplitterWidth(pageSplitterWidth());
        m_blockStyles.insert(blockStyle.type(), blockStyle);

        //
        // Если ещё не находимся в конце элемента, то остальное пропускаем
        //
        if (!reader.isEndElement()) {
            reader.skipCurrentElement();
        }
    }

    //
    // Копируем стиль для теневых заголовков из стиля обычных заголовков
    //
    {
        ScreenplayBlockStyle sceneHeadingShadowStyle = m_blockStyles.value(ScreenplayParagraphType::SceneHeading);
        sceneHeadingShadowStyle.setType(ScreenplayParagraphType::SceneHeadingShadow);
        setBlockStyle(sceneHeadingShadowStyle);
    }
}

} // namespace BusinessLayer
