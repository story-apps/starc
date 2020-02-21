#include "screenplay_template.h"

#include "screenplay_template_facade.h"

#include <ui/widgets/text_edit/page/page_metrics.h>

#include <utils/helpers/string_helper.h>

#include <QApplication>
#include <QFontMetricsF>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QXmlStreamAttributes>


namespace BusinessLayer
{

namespace {
    const QHash<ScreenplayParagraphType, QString> kScreenplayParagraphTypeToString
    = {{ ScreenplayParagraphType::UnformattedText, QLatin1String("unformatted_text") },
       { ScreenplayParagraphType::SceneName, QLatin1String("scene_name") },
       { ScreenplayParagraphType::SceneDescription, QLatin1String("scene_description") },
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
       { ScreenplayParagraphType::FolderFooter, QLatin1String("folder_footer") }};

    /**
     * @brief Получить межстрочный интервал из строки
     */
    static ScreenplayBlockStyle::LineSpacing lineSpacingFromString(const QString& _lineSpacing) {
        const QHash<ScreenplayBlockStyle::LineSpacing, QString> lineSpacingToString
                = {{ ScreenplayBlockStyle::SingleLineSpacing, "single" },
                   { ScreenplayBlockStyle::OneAndHalfLineSpacing, "oneandhalf" },
                   { ScreenplayBlockStyle::DoubleLineSpacing, "double" },
                   { ScreenplayBlockStyle::FixedLineSpacing, "fixed" }};

        return lineSpacingToString.key(_lineSpacing);
    }
}


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

bool ScreenplayBlockStyle::isActive() const
{
    return m_isActive;
}

QFont ScreenplayBlockStyle::font() const
{
    return m_font;
}

Qt::Alignment ScreenplayBlockStyle::align() const
{
    return m_align;
}

int ScreenplayBlockStyle::topSpace() const
{
    return m_topSpace;
}

int ScreenplayBlockStyle::bottomSpace() const
{
    return m_bottomSpace;
}

qreal ScreenplayBlockStyle::leftMargin() const
{
    return m_leftMargin;
}

qreal ScreenplayBlockStyle::topMargin() const
{
    return m_topMargin;
}

qreal ScreenplayBlockStyle::rightMargin() const
{
    return m_rightMargin;
}

qreal ScreenplayBlockStyle::bottomMargin() const
{
    return m_bottomMargin;
}

bool ScreenplayBlockStyle::hasVerticalSpacingInMM() const
{
    return m_topMargin != 0 || m_bottomMargin != 0;
}

ScreenplayBlockStyle::LineSpacing ScreenplayBlockStyle::lineSpacing() const
{
    return m_lineSpacing;
}

qreal ScreenplayBlockStyle::lineSpacingValue() const
{
    return m_lineSpacingValue;
}

void ScreenplayBlockStyle::setType(ScreenplayParagraphType _type)
{
    if (m_type == _type) {
        return;
    }

    m_type = _type;
    m_blockFormat.setProperty(ScreenplayBlockStyle::PropertyType, static_cast<int>(_type));
}

void ScreenplayBlockStyle::setIsActive(bool _isActive)
{
    m_isActive = _isActive;
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

void ScreenplayBlockStyle::setAlign(Qt::Alignment _align)
{
    if (m_align == _align) {
        return;
    }

    m_align = _align;
    m_blockFormat.setAlignment(m_align);
    m_blockFormat.setTopMargin(QFontMetricsF(m_font).lineSpacing() * m_topSpace);
}

void ScreenplayBlockStyle::setTopSpace(int _topSpace)
{
    if (m_topSpace == _topSpace) {
        return;
    }

    m_topSpace = _topSpace;
    updateTopMargin();
}

void ScreenplayBlockStyle::setBottomSpace(int _bottomSpace)
{
    if (m_bottomSpace == _bottomSpace) {
        return;
    }

    m_bottomSpace = _bottomSpace;
    updateBottomMargin();
}

void ScreenplayBlockStyle::setLeftMargin(qreal _leftMargin)
{
    if (m_leftMargin == _leftMargin) {
        return;
    }

    m_leftMargin = _leftMargin;
    m_blockFormat.setLeftMargin(PageMetrics::mmToPx(m_leftMargin));
}

void ScreenplayBlockStyle::setTopMargin(qreal _topMargin)
{
    if (m_topMargin == _topMargin) {
        return;
    }

    m_topMargin = _topMargin;
    updateTopMargin();
}

void ScreenplayBlockStyle::setRightMargin(qreal _rightMargin)
{
    if (m_rightMargin == _rightMargin) {
        return;
    }

    m_rightMargin = _rightMargin;
    m_blockFormat.setRightMargin(PageMetrics::mmToPx(m_rightMargin));
}

void ScreenplayBlockStyle::setBottomMargin(qreal _bottomMargin)
{
    if (m_bottomMargin == _bottomMargin) {
        return;
    }

    m_bottomMargin = _bottomMargin;
    updateBottomMargin();
}

void ScreenplayBlockStyle::setLineSpacing(ScreenplayBlockStyle::LineSpacing _lineSpacing)
{
    if (m_lineSpacing == _lineSpacing) {
        return;
    }

    m_lineSpacing = _lineSpacing;
    updateLineHeight();
}

void ScreenplayBlockStyle::setLineSpacingValue(qreal _value)
{
    if (m_lineSpacingValue == _value) {
        return;
    }

    m_lineSpacingValue = _value;
    updateLineHeight();
}

QTextBlockFormat ScreenplayBlockStyle::blockFormat() const
{
    return m_blockFormat;
}

void ScreenplayBlockStyle::setBackgroundColor(const QColor& _color)
{
    m_blockFormat.setBackground(_color);
}

QTextCharFormat ScreenplayBlockStyle::charFormat() const
{
    return m_charFormat;
}

void ScreenplayBlockStyle::setTextColor(const QColor& _color)
{
    m_charFormat.setForeground(_color);
}

bool ScreenplayBlockStyle::isFirstUppercase() const
{
    return m_charFormat.boolProperty(ScreenplayBlockStyle::PropertyIsFirstUppercase);
}

bool ScreenplayBlockStyle::isCanModify() const
{
    return m_charFormat.boolProperty(ScreenplayBlockStyle::PropertyIsCanModify);
}

void ScreenplayBlockStyle::setCanModify(bool _can)
{
    m_charFormat.setProperty(ScreenplayBlockStyle::PropertyIsCanModify, _can);
}

bool ScreenplayBlockStyle::hasDecoration() const
{
    return !prefix().isEmpty() || !postfix().isEmpty();
}

QString ScreenplayBlockStyle::prefix() const
{
    return m_charFormat.stringProperty(ScreenplayBlockStyle::PropertyPrefix);
}

void ScreenplayBlockStyle::setPrefix(const QString& _prefix)
{
    m_charFormat.setProperty(ScreenplayBlockStyle::PropertyPrefix, _prefix);
}

QString ScreenplayBlockStyle::postfix() const
{
    return m_charFormat.stringProperty(ScreenplayBlockStyle::PropertyPostfix);
}

void ScreenplayBlockStyle::setPostfix(const QString& _postfix)
{
    m_charFormat.setProperty(ScreenplayBlockStyle::PropertyPostfix, _postfix);
}

bool ScreenplayBlockStyle::isEmbeddable() const
{
    return m_type == ScreenplayParagraphType::FolderHeader
           || m_type == ScreenplayParagraphType::FolderFooter;
}

bool ScreenplayBlockStyle::isEmbeddableHeader() const
{
    return m_type == ScreenplayParagraphType::FolderHeader;
}

ScreenplayParagraphType ScreenplayBlockStyle::embeddableFooter() const
{
    if (m_type == ScreenplayParagraphType::FolderHeader) {
        return ScreenplayParagraphType::FolderFooter;
    } else {
        return ScreenplayParagraphType::Undefined;
    }
}

ScreenplayBlockStyle::ScreenplayBlockStyle(const QXmlStreamAttributes& _blockAttributes)
{
    //
    // Считываем параметры
    //
    // ... тип блока и его активность в стиле
    // NOTE: обрабатываем так же и старые стили
    //
    QString typeName = _blockAttributes.value("id").toString();
    if (typeName == "time_and_place") {
        typeName = "scene_heading";
    }
    m_type = screenplayParagraphTypeFromString(typeName);
    m_isActive = _blockAttributes.value("active").toString() == "true";
    //
    // ... настройки шрифта
    //
    m_font.setFamily(_blockAttributes.value("font_family").toString());
    m_font.setPointSizeF(_blockAttributes.value("font_size").toDouble());
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
    m_topSpace = _blockAttributes.value("top_space").toInt();
    m_bottomSpace = _blockAttributes.value("bottom_space").toInt();
    m_leftMargin = _blockAttributes.value("left_margin").toDouble();
    m_topMargin = _blockAttributes.value("top_margin").toDouble();
    m_rightMargin = _blockAttributes.value("right_margin").toDouble();
    m_bottomMargin = _blockAttributes.value("bottom_margin").toDouble();
    m_lineSpacing = lineSpacingFromString(_blockAttributes.value("line_spacing").toString());
    m_lineSpacingValue = _blockAttributes.value("line_spacing_value").toDouble();

    //
    // Настроим форматы
    //
    // ... блока
    //
    m_blockFormat.setAlignment(m_align);
    m_blockFormat.setLeftMargin(PageMetrics::mmToPx(m_leftMargin));
    m_blockFormat.setRightMargin(PageMetrics::mmToPx(m_rightMargin));
    updateLineHeight();
    //
    // ... текста
    //
    m_charFormat.setFont(m_font);

    //
    // Запомним в стиле его настройки
    //
    m_blockFormat.setProperty(ScreenplayBlockStyle::PropertyType, static_cast<int>(m_type));
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

        case ScreenplayParagraphType::FolderFooter: {
            m_charFormat.setProperty(ScreenplayBlockStyle::PropertyIsCanModify, false);
            break;
        }

        default: {
            break;
        }
    }
    //
    // ... обрамление блока
    //
    const QString prefix = _blockAttributes.value("prefix").toString();
    if (!prefix.isEmpty()) {
        m_charFormat.setProperty(ScreenplayBlockStyle::PropertyPrefix, prefix);
    }
    const QString postfix = _blockAttributes.value("postfix").toString();
    if (!postfix.isEmpty()) {
        m_charFormat.setProperty(ScreenplayBlockStyle::PropertyPostfix, postfix);
    }
}

void ScreenplayBlockStyle::updateLineHeight()
{
    auto lineHeight = QFontMetricsF(m_font).lineSpacing();
    switch (m_lineSpacing) {
        case FixedLineSpacing: {
            lineHeight = PageMetrics::mmToPx(m_lineSpacingValue);
            break;
        }

        case DoubleLineSpacing: {
            lineHeight *= 2;
            break;
        }

        case OneAndHalfLineSpacing: {
            lineHeight *= 1.5;
            break;
        }

        case SingleLineSpacing:
        default: {
            break;
        }
    }
    m_blockFormat.setLineHeight(lineHeight, QTextBlockFormat::FixedHeight);

    updateTopMargin();
    updateBottomMargin();
}

void ScreenplayBlockStyle::updateTopMargin()
{
    m_blockFormat.setTopMargin(m_blockFormat.lineHeight() * m_topSpace
                               + PageMetrics::mmToPx(m_topMargin));
}

void ScreenplayBlockStyle::updateBottomMargin()
{
    m_blockFormat.setBottomMargin(m_blockFormat.lineHeight() * m_bottomSpace
                                  + PageMetrics::mmToPx(m_bottomMargin));
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
    writer.writeAttribute("page_margins", toString(m_pageMargins));
    writer.writeAttribute("numbering_alignment", toString(m_numberingAlignment));
    for (const auto& blockStyle : m_blockStyles.values()) {
        if (toString(blockStyle.type()).isEmpty()) {
            continue;
        }

        writer.writeStartElement("block");
        writer.writeAttribute("id", toString(blockStyle.type()));
        writer.writeAttribute("active", ::toString(blockStyle.isActive()));
        writer.writeAttribute("font_family", blockStyle.font().family());
        writer.writeAttribute("font_size", ::toString(blockStyle.font().pointSize()));
        writer.writeAttribute("bold", ::toString(blockStyle.font().bold()));
        writer.writeAttribute("italic", ::toString(blockStyle.font().italic()));
        writer.writeAttribute("underline", ::toString(blockStyle.font().underline()));
        writer.writeAttribute("uppercase", ::toString(blockStyle.font().capitalization() == QFont::AllUppercase));
        writer.writeAttribute("alignment", toString(blockStyle.align()));
        writer.writeAttribute("top_space", ::toString(blockStyle.topSpace()));
        writer.writeAttribute("bottom_space", ::toString(blockStyle.bottomSpace()));
        writer.writeAttribute("left_margin", ::toString(blockStyle.leftMargin()));
        writer.writeAttribute("top_margin", ::toString(blockStyle.topMargin()));
        writer.writeAttribute("right_margin", ::toString(blockStyle.rightMargin()));
        writer.writeAttribute("bottom_margin", ::toString(blockStyle.bottomMargin()));
        writer.writeAttribute("line_spacing", ::toString(blockStyle.lineSpacing()));
        writer.writeAttribute("line_spacing_value", ::toString(blockStyle.lineSpacingValue()));
        writer.writeAttribute("prefix", blockStyle.prefix());
        writer.writeAttribute("postfix", blockStyle.postfix());
        writer.writeEndElement(); // block
    }
    writer.writeEndElement(); // style
    writer.writeEndDocument();

    templateFile.close();
}

bool ScreenplayTemplate::isDefault() const
{
    return m_isDefault;
}

QString ScreenplayTemplate::name() const
{
    return m_name;
}

QString ScreenplayTemplate::description() const
{
    return m_description;
}

QPageSize::PageSizeId ScreenplayTemplate::pageSizeId() const
{
    return m_pageSizeId;
}

QMarginsF ScreenplayTemplate::pageMargins() const
{
    return m_pageMargins;
}

Qt::Alignment ScreenplayTemplate::numberingAlignment() const
{
    return m_numberingAlignment;
}

ScreenplayBlockStyle ScreenplayTemplate::blockStyle(ScreenplayParagraphType _forType) const
{
    return m_blockStyles.value(_forType);
}

ScreenplayBlockStyle ScreenplayTemplate::blockStyle(const QTextBlock& _forBlock) const
{
    return blockStyle(ScreenplayBlockStyle::forBlock(_forBlock));
}

void ScreenplayTemplate::setName(const QString& _name)
{
    m_name = _name;
}

void ScreenplayTemplate::setDescription(const QString& _description)
{
    m_description = _description;
}

void ScreenplayTemplate::setPageSizeId(QPageSize::PageSizeId _pageSizeId)
{
    m_pageSizeId = _pageSizeId;
}

void ScreenplayTemplate::setPageMargins(const QMarginsF& _pageMargins)
{
    m_pageMargins = _pageMargins;
}

void ScreenplayTemplate::setNumberingAlignment(Qt::Alignment _alignment)
{
    m_numberingAlignment = _alignment;
}

//
// FIXME: переименовать
//
void ScreenplayTemplate::setBlockStyle(const ScreenplayBlockStyle& _blockStyle)
{
    m_blockStyles.insert(_blockStyle.type(), _blockStyle);
}

void ScreenplayTemplate::updateBlocksColors()
{
    //
    // TODO:
    //
}

ScreenplayTemplate::ScreenplayTemplate(const QString& _fromFile)
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
    m_isDefault = templateAttributes.value("default").toString() == "true";
    m_name = templateAttributes.value("name").toString();
    m_description = templateAttributes.value("description").toString();
    m_pageSizeId = PageMetrics::pageSizeIdFromString(templateAttributes.value("page_format").toString());
    m_pageMargins = marginsFromString(templateAttributes.value("page_margins").toString());
    m_numberingAlignment = alignmentFromString(templateAttributes.value("numbering_alignment").toString());

    //
    // Считываем настройки оформления блоков текста
    //
    while (reader.readNextStartElement() && reader.name() == "block")
    {
        ScreenplayBlockStyle blockStyle(reader.attributes());
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
