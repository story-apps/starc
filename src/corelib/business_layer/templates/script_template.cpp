#include "script_template.h"

#include "script_template_facade.h"

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
    const QHash<ScriptParagraphType, QString> kScreenplayParagraphTypeToString
    = {{ ScriptParagraphType::UnformattedText, QLatin1String("unformatted_text") },
       { ScriptParagraphType::SceneName, QLatin1String("scene_name") },
       { ScriptParagraphType::SceneDescription, QLatin1String("scene_description") },
       { ScriptParagraphType::SceneHeading, QLatin1String("scene_heading") },
       { ScriptParagraphType::SceneCharacters, QLatin1String("scene_characters") },
       { ScriptParagraphType::Action, QLatin1String("action") },
       { ScriptParagraphType::Character, QLatin1String("character") },
       { ScriptParagraphType::Parenthetical, QLatin1String("parenthetical") },
       { ScriptParagraphType::Dialogue, QLatin1String("dialogue") },
       { ScriptParagraphType::Lyrics, QLatin1String("lyrics") },
       { ScriptParagraphType::Transition, QLatin1String("transition") },
       { ScriptParagraphType::Shot, QLatin1String("show") },
       { ScriptParagraphType::InlineNote, QLatin1String("inline_note") },
       { ScriptParagraphType::FolderHeader, QLatin1String("folder_header") },
       { ScriptParagraphType::FolderFooter, QLatin1String("folder_footer") }};

    /**
     * @brief Получить межстрочный интервал из строки
     */
    static ScriptBlockStyle::LineSpacing lineSpacingFromString(const QString& _lineSpacing) {
        const QHash<ScriptBlockStyle::LineSpacing, QString> lineSpacingToString
                = {{ ScriptBlockStyle::SingleLineSpacing, "single" },
                   { ScriptBlockStyle::OneAndHalfLineSpacing, "oneandhalf" },
                   { ScriptBlockStyle::DoubleLineSpacing, "double" },
                   { ScriptBlockStyle::FixedLineSpacing, "fixed" }};

        return lineSpacingToString.key(_lineSpacing);
    }
}


QString toString(ScriptParagraphType _type)
{
    return kScreenplayParagraphTypeToString.value(_type);
}

ScriptParagraphType scriptParagraphTypeFromString(const QString& _text)
{
    return kScreenplayParagraphTypeToString.key(_text, ScriptParagraphType::Undefined);
}


// ****


ScriptParagraphType ScriptBlockStyle::forBlock(const QTextBlock& _block)
{
    ScriptParagraphType blockType = ScriptParagraphType::Undefined;
    if (_block.blockFormat().hasProperty(ScriptBlockStyle::PropertyType)) {
        blockType = static_cast<ScriptParagraphType>(_block.blockFormat().intProperty(ScriptBlockStyle::PropertyType));
    }
    return blockType;
}

ScriptParagraphType ScriptBlockStyle::type() const
{
    return m_type;
}

bool ScriptBlockStyle::isActive() const
{
    return m_isActive;
}

QFont ScriptBlockStyle::font() const
{
    return m_font;
}

Qt::Alignment ScriptBlockStyle::align() const
{
    return m_align;
}

int ScriptBlockStyle::topSpace() const
{
    return m_topSpace;
}

int ScriptBlockStyle::bottomSpace() const
{
    return m_bottomSpace;
}

qreal ScriptBlockStyle::leftMargin() const
{
    return m_leftMargin;
}

qreal ScriptBlockStyle::topMargin() const
{
    return m_topMargin;
}

qreal ScriptBlockStyle::rightMargin() const
{
    return m_rightMargin;
}

qreal ScriptBlockStyle::bottomMargin() const
{
    return m_bottomMargin;
}

bool ScriptBlockStyle::hasVerticalSpacingInMM() const
{
    return m_topMargin != 0 || m_bottomMargin != 0;
}

ScriptBlockStyle::LineSpacing ScriptBlockStyle::lineSpacing() const
{
    return m_lineSpacing;
}

qreal ScriptBlockStyle::lineSpacingValue() const
{
    return m_lineSpacingValue;
}

void ScriptBlockStyle::setType(ScriptParagraphType _type)
{
    if (m_type == _type) {
        return;
    }

    m_type = _type;
    m_blockFormat.setProperty(ScriptBlockStyle::PropertyType, static_cast<int>(_type));
}

void ScriptBlockStyle::setIsActive(bool _isActive)
{
    m_isActive = _isActive;
}

void ScriptBlockStyle::setFont(const QFont& _font)
{
    if (m_font == _font) {
        return;
    }

    m_font = _font;
    m_charFormat.setFont(m_font);
    updateLineHeight();
}

void ScriptBlockStyle::setAlign(Qt::Alignment _align)
{
    if (m_align == _align) {
        return;
    }

    m_align = _align;
    m_blockFormat.setAlignment(m_align);
    m_blockFormat.setTopMargin(QFontMetricsF(m_font).lineSpacing() * m_topSpace);
}

void ScriptBlockStyle::setTopSpace(int _topSpace)
{
    if (m_topSpace == _topSpace) {
        return;
    }

    m_topSpace = _topSpace;
    updateTopMargin();
}

void ScriptBlockStyle::setBottomSpace(int _bottomSpace)
{
    if (m_bottomSpace == _bottomSpace) {
        return;
    }

    m_bottomSpace = _bottomSpace;
    updateBottomMargin();
}

void ScriptBlockStyle::setLeftMargin(qreal _leftMargin)
{
    if (m_leftMargin == _leftMargin) {
        return;
    }

    m_leftMargin = _leftMargin;
    m_blockFormat.setLeftMargin(PageMetrics::mmToPx(m_leftMargin));
}

void ScriptBlockStyle::setTopMargin(qreal _topMargin)
{
    if (m_topMargin == _topMargin) {
        return;
    }

    m_topMargin = _topMargin;
    updateTopMargin();
}

void ScriptBlockStyle::setRightMargin(qreal _rightMargin)
{
    if (m_rightMargin == _rightMargin) {
        return;
    }

    m_rightMargin = _rightMargin;
    m_blockFormat.setRightMargin(PageMetrics::mmToPx(m_rightMargin));
}

void ScriptBlockStyle::setBottomMargin(qreal _bottomMargin)
{
    if (m_bottomMargin == _bottomMargin) {
        return;
    }

    m_bottomMargin = _bottomMargin;
    updateBottomMargin();
}

void ScriptBlockStyle::setLineSpacing(ScriptBlockStyle::LineSpacing _lineSpacing)
{
    if (m_lineSpacing == _lineSpacing) {
        return;
    }

    m_lineSpacing = _lineSpacing;
    updateLineHeight();
}

void ScriptBlockStyle::setLineSpacingValue(qreal _value)
{
    if (m_lineSpacingValue == _value) {
        return;
    }

    m_lineSpacingValue = _value;
    updateLineHeight();
}

QTextBlockFormat ScriptBlockStyle::blockFormat() const
{
    return m_blockFormat;
}

void ScriptBlockStyle::setBackgroundColor(const QColor& _color)
{
    m_blockFormat.setBackground(_color);
}

QTextCharFormat ScriptBlockStyle::charFormat() const
{
    return m_charFormat;
}

void ScriptBlockStyle::setTextColor(const QColor& _color)
{
    m_charFormat.setForeground(_color);
}

bool ScriptBlockStyle::isFirstUppercase() const
{
    return m_charFormat.boolProperty(ScriptBlockStyle::PropertyIsFirstUppercase);
}

bool ScriptBlockStyle::isCanModify() const
{
    return m_charFormat.boolProperty(ScriptBlockStyle::PropertyIsCanModify);
}

void ScriptBlockStyle::setCanModify(bool _can)
{
    m_charFormat.setProperty(ScriptBlockStyle::PropertyIsCanModify, _can);
}

bool ScriptBlockStyle::hasDecoration() const
{
    return !prefix().isEmpty() || !postfix().isEmpty();
}

QString ScriptBlockStyle::prefix() const
{
    return m_charFormat.stringProperty(ScriptBlockStyle::PropertyPrefix);
}

void ScriptBlockStyle::setPrefix(const QString& _prefix)
{
    m_charFormat.setProperty(ScriptBlockStyle::PropertyPrefix, _prefix);
}

QString ScriptBlockStyle::postfix() const
{
    return m_charFormat.stringProperty(ScriptBlockStyle::PropertyPostfix);
}

void ScriptBlockStyle::setPostfix(const QString& _postfix)
{
    m_charFormat.setProperty(ScriptBlockStyle::PropertyPostfix, _postfix);
}

bool ScriptBlockStyle::isEmbeddable() const
{
    return m_type == ScriptParagraphType::FolderHeader
           || m_type == ScriptParagraphType::FolderFooter;
}

bool ScriptBlockStyle::isEmbeddableHeader() const
{
    return m_type == ScriptParagraphType::FolderHeader;
}

ScriptParagraphType ScriptBlockStyle::embeddableFooter() const
{
    if (m_type == ScriptParagraphType::FolderHeader) {
        return ScriptParagraphType::FolderFooter;
    } else {
        return ScriptParagraphType::Undefined;
    }
}

ScriptBlockStyle::ScriptBlockStyle(const QXmlStreamAttributes& _blockAttributes)
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
    m_type = scriptParagraphTypeFromString(typeName);
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
    m_blockFormat.setProperty(ScriptBlockStyle::PropertyType, static_cast<int>(m_type));
    m_charFormat.setProperty(ScriptBlockStyle::PropertyIsFirstUppercase, true);
    m_charFormat.setProperty(ScriptBlockStyle::PropertyIsCanModify, true);

    //
    // Настроим остальные характеристики
    //
    switch (m_type) {
        case ScriptParagraphType::Parenthetical: {
            m_charFormat.setProperty(ScriptBlockStyle::PropertyIsFirstUppercase, false);
            //
            // Стандартное обрамление
            //
            m_charFormat.setProperty(ScriptBlockStyle::PropertyPrefix, "(");
            m_charFormat.setProperty(ScriptBlockStyle::PropertyPostfix, ")");
            break;
        }

        case ScriptParagraphType::FolderFooter: {
            m_charFormat.setProperty(ScriptBlockStyle::PropertyIsCanModify, false);
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
        m_charFormat.setProperty(ScriptBlockStyle::PropertyPrefix, prefix);
    }
    const QString postfix = _blockAttributes.value("postfix").toString();
    if (!postfix.isEmpty()) {
        m_charFormat.setProperty(ScriptBlockStyle::PropertyPostfix, postfix);
    }
}

void ScriptBlockStyle::updateLineHeight()
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

void ScriptBlockStyle::updateTopMargin()
{
    m_blockFormat.setTopMargin(m_blockFormat.lineHeight() * m_topSpace
                               + PageMetrics::mmToPx(m_topMargin));
}

void ScriptBlockStyle::updateBottomMargin()
{
    m_blockFormat.setBottomMargin(m_blockFormat.lineHeight() * m_bottomSpace
                                  + PageMetrics::mmToPx(m_bottomMargin));
}


// ****


void ScriptTemplate::setIsNew()
{
    m_isDefault = false;
    m_name.prepend(QApplication::translate("BusinessLayer::ScriptTemplate", "Copy of "));
    m_description.clear();
}

void ScriptTemplate::saveToFile(const QString& _filePath) const
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

bool ScriptTemplate::isDefault() const
{
    return m_isDefault;
}

QString ScriptTemplate::name() const
{
    return m_name;
}

QString ScriptTemplate::description() const
{
    return m_description;
}

QPageSize::PageSizeId ScriptTemplate::pageSizeId() const
{
    return m_pageSizeId;
}

QMarginsF ScriptTemplate::pageMargins() const
{
    return m_pageMargins;
}

Qt::Alignment ScriptTemplate::numberingAlignment() const
{
    return m_numberingAlignment;
}

ScriptBlockStyle ScriptTemplate::blockStyle(ScriptParagraphType _forType) const
{
    return m_blockStyles.value(_forType);
}

ScriptBlockStyle ScriptTemplate::blockStyle(const QTextBlock& _forBlock) const
{
    return blockStyle(ScriptBlockStyle::forBlock(_forBlock));
}

void ScriptTemplate::setName(const QString& _name)
{
    m_name = _name;
}

void ScriptTemplate::setDescription(const QString& _description)
{
    m_description = _description;
}

void ScriptTemplate::setPageSizeId(QPageSize::PageSizeId _pageSizeId)
{
    m_pageSizeId = _pageSizeId;
}

void ScriptTemplate::setPageMargins(const QMarginsF& _pageMargins)
{
    m_pageMargins = _pageMargins;
}

void ScriptTemplate::setNumberingAlignment(Qt::Alignment _alignment)
{
    m_numberingAlignment = _alignment;
}

//
// FIXME: переименовать
//
void ScriptTemplate::setBlockStyle(const ScriptBlockStyle& _blockStyle)
{
    m_blockStyles.insert(_blockStyle.type(), _blockStyle);
}

void ScriptTemplate::updateBlocksColors()
{
    //
    // TODO:
    //
}

ScriptTemplate::ScriptTemplate(const QString& _fromFile)
{
    load(_fromFile);
}

void ScriptTemplate::load(const QString& _fromFile)
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
        ScriptBlockStyle blockStyle(reader.attributes());
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
        ScriptBlockStyle sceneHeadingShadowStyle = m_blockStyles.value(ScriptParagraphType::SceneHeading);
        sceneHeadingShadowStyle.setType(ScriptParagraphType::SceneHeadingShadow);
        setBlockStyle(sceneHeadingShadowStyle);
    }
}

} // namespace BusinessLayer
