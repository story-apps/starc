#include "screenplay_template.h"

#include "simple_text_template.h"

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

const QHash<ScreenplayParagraphType, QString> kScreenplayParagraphTypeToString
    = { { ScreenplayParagraphType::UnformattedText, QLatin1String("unformatted_text") },
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
        { ScreenplayParagraphType::PageSplitter, QLatin1String("page_splitter") } };

const QHash<ScreenplayBlockStyle::LineSpacingType, QString> kLineSpacingToString
    = { { ScreenplayBlockStyle::LineSpacingType::SingleLineSpacing, "single" },
        { ScreenplayBlockStyle::LineSpacingType::OneAndHalfLineSpacing, "oneandhalf" },
        { ScreenplayBlockStyle::LineSpacingType::DoubleLineSpacing, "double" },
        { ScreenplayBlockStyle::LineSpacingType::FixedLineSpacing, "fixed" } };

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

QString toDisplayString(ScreenplayParagraphType _type)
{
    switch (_type) {
    case ScreenplayParagraphType::SceneHeading:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Scene heading");
    case ScreenplayParagraphType::SceneCharacters:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Scene characters");
    case ScreenplayParagraphType::Action:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Action");
    case ScreenplayParagraphType::Character:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Character");
    case ScreenplayParagraphType::Parenthetical:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Parenthetical");
    case ScreenplayParagraphType::Dialogue:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Dialogue");
    case ScreenplayParagraphType::Lyrics:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Lyrics");
    case ScreenplayParagraphType::Shot:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Shot");
    case ScreenplayParagraphType::Transition:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Transition");
    case ScreenplayParagraphType::InlineNote:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Inline note");
    case ScreenplayParagraphType::UnformattedText:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Unformatted text");
    case ScreenplayParagraphType::FolderHeader:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Folder");
    case ScreenplayParagraphType::FolderFooter:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Folder footer");
    default:
        return QCoreApplication::translate("BusinessLayer::ScreenplayTemplate", "Undefined");
    }
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
        blockType = static_cast<ScreenplayParagraphType>(
            _block.blockFormat().intProperty(ScreenplayBlockStyle::PropertyType));
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
    m_blockFormatOnHalfPage.setProperty(ScreenplayBlockStyle::PropertyType,
                                        static_cast<int>(_type));
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
    m_blockFormat.setProperty(ScreenplayBlockStyle::PropertyType, static_cast<int>(m_type));
    m_blockFormatOnHalfPage.setProperty(ScreenplayBlockStyle::PropertyType,
                                        static_cast<int>(m_type));
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


class ScreenplayTemplate::Implementation
{
public:
    Implementation();

    /**
     * @brief Сформировать шаблон оформления титульной страницы
     */
    void buildTitlePageTemplate();


    /**
     * @brief Идентификатор
     */
    QString id;

    /**
     * @brief Является ли шаблон умолчальным
     */
    bool isDefault = false;

    /**
     * @brief Название
     */
    QString name;

    /**
     * @brief Описание
     */
    QString description;

    /**
     * @brief Формат страницы
     */
    QPageSize::PageSizeId pageSizeId;

    /**
     * @brief Поля страницы в миллиметрах
     */
    QMarginsF pageMargins;

    /**
     * @brief Расположение нумерации
     */
    Qt::Alignment pageNumbersAlignment;

    /**
     * @brief Процент от ширины страницы, которые занимает левая часть разделения
     */
    int leftHalfOfPageWidthPercents = 50;

    /**
     * @brief Шаблон-компаньён, используемый для титульной страницы
     */
    SimpleTextTemplate titlePageTemplate;

    /**
     * @brief Xml титульной страницы
     */
    QString titlePage;

    /**
     * @brief Стили блоков текста
     */
    QHash<ScreenplayParagraphType, ScreenplayBlockStyle> paragrapsStyles;
};

ScreenplayTemplate::Implementation::Implementation()
    : id(QUuid::createUuid().toString())
{
}

void ScreenplayTemplate::Implementation::buildTitlePageTemplate()
{
    titlePageTemplate.setPageSizeId(pageSizeId);
    titlePageTemplate.setPageMargins(pageMargins);
    titlePageTemplate.setPageNumbersAlignment(pageNumbersAlignment);

    TextBlockStyle defaultBlockStyle;
    defaultBlockStyle.setActive(true);
    defaultBlockStyle.setStartFromNewPage(false);
    const auto actionBlockStyle = paragrapsStyles.value(ScreenplayParagraphType::Action);
    defaultBlockStyle.setFont(actionBlockStyle.font());
    defaultBlockStyle.setAlign(actionBlockStyle.align());
    //
    for (auto type : {
             TextParagraphType::Heading1,
             TextParagraphType::Heading2,
             TextParagraphType::Heading3,
             TextParagraphType::Heading4,
             TextParagraphType::Heading5,
             TextParagraphType::Heading6,
             TextParagraphType::Text,
             TextParagraphType::InlineNote,
         }) {
        auto blockStyle = defaultBlockStyle;
        blockStyle.setType(type);
        titlePageTemplate.setParagraphStyle(blockStyle);
    }
}

// **

ScreenplayTemplate::ScreenplayTemplate()
    : d(new Implementation)
{
}

ScreenplayTemplate::ScreenplayTemplate(const ScreenplayTemplate& _other)
    : d(new Implementation)
{
    *d = *_other.d;
}

ScreenplayTemplate& ScreenplayTemplate::operator=(const ScreenplayTemplate& _other)
{
    if (this != &_other) {
        *d = *_other.d;
    }
    return *this;
}

ScreenplayTemplate::~ScreenplayTemplate() = default;

void ScreenplayTemplate::setIsNew()
{
    d->isDefault = false;
    d->name = QApplication::translate("BusinessLayer::ScreenplayTemplate", "Copy of ") + name();
    d->id = QUuid::createUuid().toString();
    d->description.clear();
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
    writer.writeAttribute("id", d->id);
    writer.writeAttribute("name", d->name);
    writer.writeAttribute("description", d->description);
    writer.writeAttribute("page_format", PageMetrics::stringFromPageSizeId(d->pageSizeId));
    writer.writeAttribute("page_margins", ::toString(d->pageMargins));
    writer.writeAttribute("page_numbers_alignment", ::toString(d->pageNumbersAlignment));
    writer.writeAttribute("left_half_of_page_width", ::toString(d->leftHalfOfPageWidthPercents));
    writer.writeStartElement("titlepage");

    writer.writeEndElement(); // titlepage
    writer.writeStartElement("blocks");
    for (const auto& blockStyle : std::as_const(d->paragrapsStyles)) {
        if (toString(blockStyle.type()).isEmpty()) {
            continue;
        }

        writer.writeStartElement("block");
        writer.writeAttribute("id", toString(blockStyle.type()));
        writer.writeAttribute("active", ::toString(blockStyle.isActive()));
        writer.writeAttribute("starts_frod->new_page", ::toString(blockStyle.isStartFromNewPage()));
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
    writer.writeEndElement(); // blocks
    writer.writeEndElement(); // style
    writer.writeEndDocument();

    templateFile.close();
}

QString ScreenplayTemplate::id() const
{
    return d->id;
}

bool ScreenplayTemplate::isDefault() const
{
    return d->isDefault;
}

QString ScreenplayTemplate::name() const
{
    if (d->name.isEmpty()) {
        if (d->id == "world_cp") {
            return QApplication::translate(
                "BusinessLayer::ScreenplayTemplate",
                "International template (page: A4; font: Courier Prime)");
        } else if (d->id == "world_cn") {
            return QApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "International template (page: A4; font: Courier New)");
        } else if (d->id == "ar") {
            return QApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "Arabic template (page: A4; font: Courier New)");
        } else if (d->id == "he") {
            return QApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "Hebrew template (page: A4; font: Arial)");
        } else if (d->id == "ru") {
            return QApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "Russian template (page: A4; font: Courier New)");
        } else if (d->id == "tamil") {
            return QApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "Tamil template (page: A4; font: Mukta Malar)");
        } else if (d->id == "us") {
            return QApplication::translate("BusinessLayer::ScreenplayTemplate",
                                           "US template (page: Letter; font: Courier Prime)");
        }
    }

    return d->name;
}

void ScreenplayTemplate::setName(const QString& _name)
{
    d->name = _name;
}

QString ScreenplayTemplate::description() const
{
    return d->description;
}

void ScreenplayTemplate::setDescription(const QString& _description)
{
    d->description = _description;
}

QPageSize::PageSizeId ScreenplayTemplate::pageSizeId() const
{
    return d->pageSizeId;
}

void ScreenplayTemplate::setPageSizeId(QPageSize::PageSizeId _pageSizeId)
{
    d->pageSizeId = _pageSizeId;
}

QMarginsF ScreenplayTemplate::pageMargins() const
{
    return d->pageMargins;
}

void ScreenplayTemplate::setPageMargins(const QMarginsF& _pageMargins)
{
    d->pageMargins = _pageMargins;
}

Qt::Alignment ScreenplayTemplate::pageNumbersAlignment() const
{
    return d->pageNumbersAlignment;
}

void ScreenplayTemplate::setPageNumbersAlignment(Qt::Alignment _alignment)
{
    d->pageNumbersAlignment = _alignment;
}

int ScreenplayTemplate::leftHalfOfPageWidthPercents() const
{
    return d->leftHalfOfPageWidthPercents;
}

void ScreenplayTemplate::setLeftHalfOfPageWidthPercents(int _width)
{
    d->leftHalfOfPageWidthPercents = _width;
}

qreal ScreenplayTemplate::pageSplitterWidth() const
{
    //
    // TODO: вынести в параметры шаблона
    //
    return PageMetrics::mmToPx(5);
}

const SimpleTextTemplate& ScreenplayTemplate::titlePageTemplate() const
{
    return d->titlePageTemplate;
}

const QString& ScreenplayTemplate::titlePage() const
{
    return d->titlePage;
}

ScreenplayBlockStyle ScreenplayTemplate::paragraphStyle(ScreenplayParagraphType _forType) const
{
    return d->paragrapsStyles.value(_forType);
}

ScreenplayBlockStyle ScreenplayTemplate::paragraphStyle(const QTextBlock& _forBlock) const
{
    return paragraphStyle(ScreenplayBlockStyle::forBlock(_forBlock));
}

void ScreenplayTemplate::setParagraphStyle(const ScreenplayBlockStyle& _style)
{
    d->paragrapsStyles.insert(_style.type(), _style);

    //
    // Если сменился стиль описания действия, то надо перестоить шаблон оформления титульного листа,
    // т.к. она базируется на шрифте из стиля описания действия
    //
    if (_style.type() == ScreenplayParagraphType::Action) {
        d->buildTitlePageTemplate();
    }
}

ScreenplayTemplate::ScreenplayTemplate(const QString& _fromFile)
    : d(new Implementation)
{
    load(_fromFile);
}

void ScreenplayTemplate::load(const QString& _fromFile)
{
    QFile templateFile(_fromFile);
    if (!templateFile.open(QIODevice::ReadOnly)) {
        return;
    }
    const QString templateXml = templateFile.readAll();
    QXmlStreamReader reader(templateXml);

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
        d->id = templateAttributes.value("id").toString();
    }
    d->isDefault = templateAttributes.value("default").toString() == "true";
    d->name = templateAttributes.value("name").toString();
    d->description = templateAttributes.value("description").toString();
    d->pageSizeId
        = PageMetrics::pageSizeIdFromString(templateAttributes.value("page_format").toString());
    d->pageMargins = marginsFromString(templateAttributes.value("page_margins").toString());
    d->pageNumbersAlignment
        = alignmentFromString(templateAttributes.value("page_numbers_alignment").toString());
    d->leftHalfOfPageWidthPercents = templateAttributes.value("left_half_of_page_width").toInt();

    //
    // Считываем титульную страницу
    //
    reader.readNextStartElement();
    Q_ASSERT(reader.name() == "titlepage");
    const auto titlePageXmlFrom = reader.characterOffset();
    reader.readNextStartElement();
    reader.skipCurrentElement();
    const auto titlePageXmlEnd = reader.characterOffset();
    d->titlePage
        = templateXml.mid(titlePageXmlFrom, titlePageXmlEnd - titlePageXmlFrom).simplified();
    reader.readNext();
    reader.readNext();

    //
    // Считываем настройки оформления блоков текста
    //
    reader.readNextStartElement();
    Q_ASSERT(reader.name() == "blocks");
    while (reader.readNextStartElement() && reader.name() == "block") {
        ScreenplayBlockStyle blockStyle(reader.attributes());
        blockStyle.setPageSplitterWidth(pageSplitterWidth());
        d->paragrapsStyles.insert(blockStyle.type(), blockStyle);

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
        ScreenplayBlockStyle sceneHeadingShadowStyle
            = d->paragrapsStyles.value(ScreenplayParagraphType::SceneHeading);
        sceneHeadingShadowStyle.setType(ScreenplayParagraphType::SceneHeadingShadow);
        setParagraphStyle(sceneHeadingShadowStyle);
    }

    //
    // Сформиуем стиль компаньон для титульной страницы
    //
    d->buildTitlePageTemplate();
}

} // namespace BusinessLayer
