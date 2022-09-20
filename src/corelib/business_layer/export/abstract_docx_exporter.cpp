#include "abstract_docx_exporter.h"

#include "qtzip/QtZipWriter"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/document/text/text_document.h>
#include <business_layer/export/export_options.h>
#include <business_layer/model/text/text_model.h>
#include <business_layer/model/text/text_model_splitter_item.h>
#include <business_layer/templates/templates_facade.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QFile>
#include <QFontMetrics>
#include <QLocale>
#include <QTextBlock>
#include <QTextLayout>


namespace BusinessLayer {

namespace {

/**
 * @brief Перевести миллиметры в твипсы (мера длины в формате RTF)
 */
int mmToTwips(qreal _mm)
{
    return MeasurementHelper::mmToTwips(_mm);
}

/**
 * @brief Перевести пиксели в твипсы
 */
int pxToTwips(qreal _px, bool _x = true)
{
    return MeasurementHelper::pxToTwips(_px, _x);
}

} // namespace

class AbstractDocxExporter::Implementation
{
public:
    explicit Implementation(AbstractDocxExporter* _q);

    /**
     * @brief Необходимо ли писать верхний колонтитул
     */
    bool needWriteHeader(const ExportOptions& _exportOptions) const;

    /**
     * @brief Необходимо ли писать нижний колонтитул
     */
    bool needWriteFooter(const ExportOptions& _exportOptions) const;

    /**
     * @brief Название типа параграфа в специальном формате
     */
    QString paragraphTypeName(TextParagraphType _type, const QString suffix = "",
                              const QString& _separator = "") const;

    /**
     * @brief Сформировать строку DOCX-формата для параметра выравнивания
     */
    QString docxAlignment(Qt::Alignment _alignment) const;

    /**
     * @brief Сформировать строку DOCX-стиля из стиля блока
     */
    QString docxBlockStyle(const TextBlockStyle& _style, const QString& _defaultFontFamily,
                           bool _onHalfPage = false) const;

    /**
     * @brief Является ли заданный формат открывающим комментарий
     */
    bool isCommentsRangeStart(const QTextBlock& _block,
                              const QTextLayout::FormatRange& _range) const;

    /**
     * @brief Является ли заданный формат закрывающим комментарий
     */
    bool isCommentsRangeEnd(const QTextBlock& _block, const QTextLayout::FormatRange& _range) const;

    /**
     * @brief Сформировать текст блока документа в зависимости от его стиля и оформления
     */
    QString docxText(QMap<int, QStringList>& _comments, const TextCursor& _cursor,
                     const ExportOptions& _exportOptions) const;
    void writeStaticData(QtZipWriter* _zip, const ExportOptions& _exportOptions) const;
    void writeStyles(QtZipWriter* _zip, const ExportOptions& _exportOptions) const;
    void writeHeader(QtZipWriter* _zip, const ExportOptions& _exportOptions) const;
    void writeFooter(QtZipWriter* _zip, const ExportOptions& _exportOptions) const;
    void writeDocument(QtZipWriter* _zip, TextDocument* _audioplayText,
                       QMap<int, QStringList>& _comments,
                       const ExportOptions& _exportOptions) const;
    void writeComments(QtZipWriter* _zip, const QMap<int, QStringList>& _comments) const;


    AbstractDocxExporter* q = nullptr;
};

AbstractDocxExporter::Implementation::Implementation(AbstractDocxExporter* _q)
    : q(_q)
{
}

bool AbstractDocxExporter::Implementation::needWriteHeader(
    const ExportOptions& _exportOptions) const
{
    return q->documentTemplate(_exportOptions).pageNumbersAlignment().testFlag(Qt::AlignTop)
        || !_exportOptions.header.isEmpty();
}

bool AbstractDocxExporter::Implementation::needWriteFooter(
    const ExportOptions& _exportOptions) const
{
    return q->documentTemplate(_exportOptions).pageNumbersAlignment().testFlag(Qt::AlignBottom)
        || !_exportOptions.footer.isEmpty();
}

QString AbstractDocxExporter::Implementation::paragraphTypeName(TextParagraphType _type,
                                                                const QString suffix,
                                                                const QString& _separator) const
{
    return (toString(_type) + suffix).toUpper().replace("_", _separator);
}

QString AbstractDocxExporter::Implementation::docxAlignment(Qt::Alignment _alignment) const
{
    QString alignment = "<w:jc w:val=\"";
    switch (_alignment) {
    case Qt::AlignRight: {
        alignment.append("right");
        break;
    }

    case Qt::AlignCenter:
    case Qt::AlignHCenter: {
        alignment.append("center");
        break;
    }

    case Qt::AlignJustify: {
        alignment.append("both");
        break;
    }

    default: {
        alignment.append("left");
        break;
    }
    }
    alignment.append("\"/>");
    return alignment;
}

QString AbstractDocxExporter::Implementation::docxBlockStyle(const TextBlockStyle& _style,
                                                             const QString& _defaultFontFamily,
                                                             bool _onHalfPage) const
{
    //
    // Для неопределённого стиля формируется простая заглушка
    //
    if (_style.type() == TextParagraphType::Undefined) {
        return QString("<w:style w:type=\"paragraph\" w:styleId=\"Normal\">"
                       "<w:name w:val=\"Normal\"/>"
                       "<w:pPr>"
                       "<w:widowControl w:val=\"0\"/><w:autoSpaceDE w:val=\"0\"/><w:autoSpaceDN "
                       "w:val=\"0\"/><w:adjustRightInd w:val=\"0\"/><w:spacing w:after=\"0\" "
                       "w:line=\"240\" w:lineRule=\"auto\"/>"
                       "</w:pPr>"
                       "<w:rPr>"
                       "<w:rFonts w:ascii=\"%1\" w:hAnsi=\"%1\" w:cs=\"%1\"/>"
                       "<w:sz w:val=\"24\"/>"
                       "</w:rPr>"
                       "</w:style>")
            .arg(_defaultFontFamily);
    }

    QString blockStyle;

    //
    // Стиль
    //
    const QString suffix = _onHalfPage ? "_splitted" : "";
    blockStyle.append(QString("<w:style w:type=\"paragraph\" w:styleId=\"%1\">")
                          .arg(paragraphTypeName(_style.type(), suffix)));
    //
    // Наименование
    //
    blockStyle.append(
        QString("<w:name w:val=\"%1\"/>").arg(paragraphTypeName(_style.type(), suffix, " ")));

    //
    // Свойства
    //
    blockStyle.append("<w:pPr>");
    //
    // ... разрешаем перенос абзацев между страницами и висячие строки
    //
    blockStyle.append("<w:widowControl w:val=\"0\"/><w:autoSpaceDE w:val=\"0\"/><w:autoSpaceDN "
                      "w:val=\"0\"/><w:adjustRightInd w:val=\"0\"/>");
    //
    // ... отступы
    //
    if (QLocale().textDirection() == Qt::LeftToRight) {
        blockStyle.append(QString("<w:ind w:left=\"%1\" w:right=\"%2\"/>")
                              .arg(pxToTwips(_style.blockFormat(_onHalfPage).leftMargin()))
                              .arg(pxToTwips(_style.blockFormat(_onHalfPage).rightMargin())));
    } else {
        blockStyle.append(QString("<w:ind w:left=\"%1\" w:right=\"%2\"/>")
                              .arg(pxToTwips(_style.blockFormat(_onHalfPage).rightMargin()))
                              .arg(pxToTwips(_style.blockFormat(_onHalfPage).leftMargin())));
    }
    //
    // ... интервалы
    //
    blockStyle.append(QString("<w:spacing w:before=\"%1\" w:after=\"%2\" ")
                          .arg(pxToTwips(_style.blockFormat(_onHalfPage).topMargin(), false))
                          .arg(pxToTwips(_style.blockFormat(_onHalfPage).bottomMargin(), false)));
    // ... межстрочный
    int lineSpacing = 240;
    QString lineSpacingType = "auto";
    switch (_style.lineSpacingType()) {
    default:
    case TextBlockStyle::LineSpacingType::SingleLineSpacing: {
        break;
    }

    case TextBlockStyle::LineSpacingType::OneAndHalfLineSpacing: {
        lineSpacing = 360;
        break;
    }

    case TextBlockStyle::LineSpacingType::DoubleLineSpacing: {
        lineSpacing = 480;
        break;
    }

    case TextBlockStyle::LineSpacingType::FixedLineSpacing: {
        lineSpacing = mmToTwips(_style.lineSpacingValue());
        lineSpacingType = "exact";
        break;
    }
    }
    blockStyle.append(
        QString("w:line=\"%1\" w:lineRule=\"%2\"/>").arg(lineSpacing).arg(lineSpacingType));
    //
    // ... выравнивание
    //
    blockStyle.append(docxAlignment(_style.blockFormat(_onHalfPage).alignment()));
    //
    // ... направление письма
    //
    blockStyle.append(QString("<w:bidi w:val=\"%1\"/>")
                          .arg(QLocale().textDirection() == Qt::RightToLeft ? "true" : "false"));
    //
    // ... начинать с новой страницы
    //
    if (_style.blockFormat().pageBreakPolicy() == QTextFormat::PageBreak_AlwaysBefore) {
        blockStyle.append("<w:pageBreakBefore/>");
    }
    //
    // ... конец свойств
    //
    blockStyle.append("</w:pPr>");

    //
    // Параметры шрифта
    //
    blockStyle.append("<w:rPr>");
    blockStyle.append(QString("<w:rFonts w:ascii=\"%1\" w:hAnsi=\"%1\" w:cs=\"%1\"/>")
                          .arg(_style.font().family()));
    //
    // ... размер
    //
    blockStyle.append(QString("<w:sz w:val=\"%1\"/><w:szCs w:val=\"%1\"/>")
                          .arg(MeasurementHelper::pxToPt(_style.font().pixelSize()) * 2));
    //
    // ... начертание
    //
    if (_style.font().bold()) {
        blockStyle.append("<w:b/><w:bCs/>");
    }
    if (_style.font().italic()) {
        blockStyle.append("<w:i/><w:iCs/>");
    }
    if (_style.font().underline()) {
        blockStyle.append("<w:u  w:val=\"single\"/>");
    }
    //
    // ... регистр
    //
    if (_style.font().capitalization() == QFont::AllUppercase) {
        blockStyle.append("<w:caps w:val=\"true\" />");
    }
    //
    // ... конец параметров шрифта
    //
    blockStyle.append("</w:rPr>");

    //
    // Конец стиля
    //
    blockStyle.append("</w:style>");


    return blockStyle;
}

bool AbstractDocxExporter::Implementation::isCommentsRangeStart(
    const QTextBlock& _block, const QTextLayout::FormatRange& _range) const
{
    //
    // Ищем предыдущий блок, до тех пор, пока не дойдём до текста или начала
    //
    auto prevBlock = _block.previous();
    while (prevBlock.isValid() && prevBlock.text().isEmpty()) {
        prevBlock = prevBlock.previous();
    }

    bool isStart = true;
    if (prevBlock.isValid()) {
        const auto textFormats = prevBlock.textFormats();
        for (const auto& range : textFormats) {
            if (range.format == _range.format) {
                isStart = false;
                break;
            }
        }
    }
    return isStart;
}

bool AbstractDocxExporter::Implementation::isCommentsRangeEnd(
    const QTextBlock& _block, const QTextLayout::FormatRange& _range) const
{
    //
    // Ищем следующий блок, до тех пор, пока не дойдём до текста или конца
    //
    QTextBlock nextBlock = _block.next();
    while (nextBlock.isValid() && nextBlock.text().isEmpty()) {
        nextBlock = nextBlock.next();
    }

    bool isEnd = true;
    if (nextBlock.isValid()) {
        const auto textFormats = nextBlock.textFormats();
        for (const auto& range : textFormats) {
            if (range.format == _range.format) {
                isEnd = false;
                break;
            }
        }
    }
    return isEnd;
}

QString AbstractDocxExporter::Implementation::docxText(QMap<int, QStringList>& _comments,
                                                       const TextCursor& _cursor,
                                                       const ExportOptions& _exportOptions) const
{
    //
    // Блокируем сигналы от документа - по ходу экспорта мы будем изменять документ,
    // но нам не нужно при этом корректировать переносы в нём
    //
    QSignalBlocker documentSignalBlocker(_cursor.document());

    const auto& audioplayTemplate = q->documentTemplate(_exportOptions);
    auto tableWidth = [&audioplayTemplate] {
        const QSizeF paperSize
            = QPageSize(audioplayTemplate.pageSizeId()).size(QPageSize::Millimeter);
        return mmToTwips(paperSize.width() - audioplayTemplate.pageMargins().left()
                         - audioplayTemplate.pageMargins().right());
    };

    QString documentXml;

    //
    // Получим стиль параграфа
    //
    const QTextBlock block = _cursor.block();
    const auto currentBlockType = TextBlockStyle::forBlock(block);
    const auto correctedBlockType = [currentBlockType] {
        switch (currentBlockType) {
        case TextParagraphType::SceneHeadingShadow:
        case TextParagraphType::SceneHeadingShadowTreatment: {
            return TextParagraphType::SceneHeading;
        }
        case TextParagraphType::BeatHeadingShadow: {
            return TextParagraphType::BeatHeading;
        }
        case TextParagraphType::PanelHeadingShadow: {
            return TextParagraphType::PanelHeading;
        }
        default: {
            return currentBlockType;
        }
        }
    }();

    //
    // Запишем параграф в документ
    //
    // ... неизвестный стиль параграфа
    //
    if (currentBlockType == TextParagraphType::Undefined) {
        //
        // ... настройки абзаца
        //
        documentXml = "<w:p><w:pPr><w:pStyle w:val=\"Normal\"/>";
        if (block.textDirection() == Qt::RightToLeft) {
            documentXml.append("<w:bidi/>");
        }
        documentXml.append(
            QString("<w:rPr><w:rFonts w:ascii=\"%1\" w:hAnsi=\"%1\"/><w:sz w:val=\"%2\"/><w:szCs "
                    "w:val=\"%2\"/></w:rPr>")
                .arg(_cursor.charFormat().font().family())
                .arg(MeasurementHelper::pxToPt(_cursor.charFormat().font().pixelSize()) * 2));
        documentXml.append(docxAlignment(_cursor.blockFormat().alignment()));
        if (_cursor.blockFormat().rightMargin() != 0 || _cursor.blockFormat().leftMargin() != 0) {
            documentXml.append(QString("<w:ind w:left=\"%1\" w:right=\"%2\" w:hanging=\"0\" />")
                                   .arg(pxToTwips(_cursor.blockFormat().leftMargin()))
                                   .arg(pxToTwips(_cursor.blockFormat().rightMargin())));
        }
        //
        // ... интервалы
        //
        if (_cursor.blockFormat().topMargin() != 0 || _cursor.blockFormat().bottomMargin() != 0) {
            documentXml.append(
                QString("<w:spacing w:before=\"%1\" w:after=\"%2\" w:lineRule=\"auto\"/>")
                    .arg(pxToTwips(_cursor.blockFormat().topMargin(), false))
                    .arg(pxToTwips(_cursor.blockFormat().bottomMargin(), false)));
        }
        documentXml.append("<w:rPr/></w:pPr>");
        //
        // ... текст блока
        //
        const auto textFormats = _cursor.block().textFormats();
        for (const auto& formatRange : textFormats) {
            auto formatRangeText
                = _cursor.block().text().mid(formatRange.start, formatRange.length);
            if (formatRange.format.fontCapitalization() == QFont::AllUppercase) {
                formatRangeText = TextHelper::smartToUpper(formatRangeText);
            }
            documentXml.append(
                QString("<w:r><w:rPr><w:rFonts w:ascii=\"%1\" w:hAnsi=\"%1\"/><w:b "
                        "w:val=\"%2\"/><w:bCs w:val=\"%2\"/><w:i w:val=\"%3\"/><w:iCs "
                        "w:val=\"%3\"/><w:sz w:val=\"%4\"/><w:szCs w:val=\"%4\"/><w:u "
                        "w:val=\"%5\"/><w:rtl val=\"%6\"/></w:rPr><w:t>%7</w:t></w:r>")
                    .arg(formatRange.format.font().family())
                    .arg(formatRange.format.font().bold() ? "true" : "false")
                    .arg(formatRange.format.font().italic() ? "true" : "false")
                    .arg(MeasurementHelper::pxToPt(formatRange.format.font().pixelSize()) * 2)
                    .arg(formatRange.format.font().underline() ? "single" : "none")
                    .arg(_cursor.block().textDirection() == Qt::RightToLeft ? "true" : "false")
                    .arg(TextHelper::toHtmlEscaped(formatRangeText)));
        }
        documentXml.append("</w:p>");
    }
    //
    // ... начало и конец таблицы
    //
    else if (currentBlockType == TextParagraphType::PageSplitter) {
        const auto blockData = static_cast<TextBlockData*>(block.userData());
        if (blockData != nullptr) {
            const auto splitterItem = static_cast<TextModelSplitterItem*>(blockData->item());
            if (splitterItem->splitterType() == TextModelSplitterItemType::Start) {
                documentXml.append("<w:tbl><w:tblPr>");
                const auto fullTableWidth = tableWidth();
                documentXml.append(
                    QString("<w:tblW w:w=\"%1\" w:type=\"dxa\"/>").arg(tableWidth()));
                documentXml.append(
                    "<w:jc w:val=\"left\"/><w:tblInd w:w=\"0\" "
                    "w:type=\"dxa\"/><w:tblCellMar><w:top w:w=\"0\" w:type=\"dxa\"/><w:left "
                    "w:w=\"0\" w:type=\"dxa\"/><w:bottom w:w=\"0\" w:type=\"dxa\"/><w:right "
                    "w:w=\"0\" w:type=\"dxa\"/></w:tblCellMar></w:tblPr><w:tblGrid>");
                const int middleColumnWidth = pxToTwips(audioplayTemplate.pageSplitterWidth());
                const int leftColumnWidth = (fullTableWidth - middleColumnWidth)
                    * audioplayTemplate.leftHalfOfPageWidthPercents() / 100.;
                documentXml.append(QString("<w:gridCol w:w=\"%1\"/>").arg(leftColumnWidth));
                documentXml.append(QString("<w:gridCol w:w=\"%1\"/>").arg(middleColumnWidth));
                const int rightColumnWidth = fullTableWidth - leftColumnWidth - middleColumnWidth;
                documentXml.append(QString("<w:gridCol w:w=\"%1\"/>").arg(rightColumnWidth));
                documentXml.append("</w:tblGrid><w:tr><w:trPr/><w:tc><w:tcPr>");
                documentXml.append(
                    QString("<w:tcW w:w=\"%1\" w:type=\"dxa\"/>").arg(leftColumnWidth));
                documentXml.append("<w:tcBorders/></w:tcPr>");
            } else {
                documentXml.append("</w:tc></w:tr></w:tbl>");
            }
        }
    }
    //
    // ... текстовые блоки сценария
    //
    else {
        //
        // ... если перешли во вторую колонку таблицы, то нужно закрыть предыдущую ячейку
        //
        if (_cursor.inTable() && !_cursor.inFirstColumn()) {
            TextCursor cursor(_cursor);
            cursor.movePosition(TextCursor::PreviousBlock);
            if (cursor.inFirstColumn()) {
                documentXml.append("</w:tc><w:tc><w:tcPr>");
                const auto fullTableWidth = tableWidth();
                const int middleColumnWidth = pxToTwips(audioplayTemplate.pageSplitterWidth());
                documentXml.append(
                    QString("<w:tcW w:w=\"%1\" w:type=\"dxa\"/>").arg(middleColumnWidth));
                documentXml.append(
                    "<w:tcBorders/></w:tcPr>"
                    "<w:p><w:pPr><w:pStyle w:val=\"Normal\"/><w:rPr/></w:pPr>"
                    "<w:r><w:rPr><w:b w:val=\"false\"/><w:i w:val=\"false\"/><w:u "
                    "w:val=\"none\"/><w:rtl w:val=\"false\"/></w:rPr><w:t "
                    "xml:space=\"preserve\"></w:t></w:r></w:p></w:tc><w:tc><w:tcPr>");
                const int leftColumnWidth = (fullTableWidth - middleColumnWidth)
                    * audioplayTemplate.leftHalfOfPageWidthPercents() / 100.;
                const int rightColumnWidth = fullTableWidth - leftColumnWidth - middleColumnWidth;
                documentXml.append(
                    QString("<w:tcW w:w=\"%1\" w:type=\"dxa\"/>").arg(rightColumnWidth));
                documentXml.append("<w:tcBorders/></w:tcPr>");
            }
        }

        //
        // ... пишем стиль блока
        //
        const QString suffix = _cursor.inTable() ? "_splitted" : "";
        documentXml.append(QString("<w:p><w:pPr><w:pStyle w:val=\"%1\"/>")
                               .arg(paragraphTypeName(correctedBlockType, suffix)));
        //
        // ... признак RTL
        //
        if (block.textDirection() == Qt::RightToLeft) {
            documentXml.append("<w:bidi/>");
        }
        //
        // ... начинать с новой страницы
        //
        if (_cursor.blockFormat().pageBreakPolicy() == QTextFormat::PageBreak_AlwaysBefore) {
            documentXml.append("<w:spacing w:before=\"0\"/>");
            documentXml.append("<w:pageBreakBefore/>");
        }
        //
        // ... если это самый первый блок в документе,
        //     или блок с текстом ПРОД., вставляемый на обрыве реплики,
        //     принудительно убираем отступ сверху у абзаца
        //
        else if (_cursor.atStart()
                 || _cursor.blockFormat().hasProperty(
                     TextBlockStyle::PropertyIsCorrectionContinued)) {
            documentXml.append("<w:spacing w:before=\"0\"/>");
        }

        //
        // ... если в блоке кастомное выравнивание, то добавим эту информацию
        //
        if (block.blockFormat().alignment()
            != q->documentTemplate(_exportOptions).paragraphStyle(currentBlockType).align()) {
            documentXml.append(docxAlignment(block.blockFormat().alignment()));
        }

        //
        // ... обработаем блок в наследнике
        //
        q->processBlock(_cursor, _exportOptions, documentXml);

        //
        //  ... текст абзаца
        //
        const QString blockText = block.text();
        documentXml.append("<w:rPr/></w:pPr>");
        const auto textFormats = block.textFormats();
        for (const auto& range : textFormats) {
            const auto formatRangeSourceText = blockText.mid(range.start, range.length);
            auto formatRangeText = formatRangeSourceText;
            if (range.format.fontCapitalization() == QFont::AllUppercase) {
                formatRangeText = TextHelper::smartToUpper(formatRangeText);
            }
            formatRangeText = QString("<w:t xml:space=\"preserve\">%2</w:t>")
                                  .arg(TextHelper::toHtmlEscaped(formatRangeText)
                                           .replace(QChar::LineSeparator,
                                                    "</w:t><w:br/><w:t xml:space=\"preserve\">"));

            //
            // ... не является редакторской заметкой
            //
            if (range.format.boolProperty(TextBlockStyle::PropertyIsReviewMark) == false) {
                //
                // ... стандартный для абзаца
                //
                if (range.format == block.charFormat()) {
                    documentXml.append("<w:r>");
                    if (formatRangeSourceText.isRightToLeft()) {
                        documentXml.append("<w:rPr><w:rtl/></w:rPr>");
                    }
                    documentXml.append(formatRangeText);
                    documentXml.append("</w:r>");

                }
                //
                // ... не стандартный
                //
                else {
                    documentXml.append("<w:r>");
                    documentXml.append("<w:rPr>");
                    if (range.format.font().bold()) {
                        documentXml.append("<w:b/><w:bCs/>");
                    }
                    if (range.format.font().italic()) {
                        documentXml.append("<w:i/><w:iCs/>");
                    }
                    if (range.format.font().underline()) {
                        documentXml.append("<w:u w:val=\"single\"/>");
                    }
                    if (formatRangeSourceText.isRightToLeft()) {
                        documentXml.append("<w:rtl/>");
                    }
                    documentXml.append("</w:rPr>");
                    //
                    // Сам текст
                    //
                    documentXml.append(formatRangeText);
                    documentXml.append("</w:r>");
                }
            }
            //
            // ... редакторская заметка
            //
            else {
                const QStringList comments
                    = range.format.property(TextBlockStyle::PropertyComments).toStringList();
                const bool hasComments = !comments.isEmpty() && !comments.first().isEmpty();
                int lastCommentIndex = _comments.isEmpty() ? 0 : _comments.lastKey();
                //
                // Комментарий
                //
                if (hasComments && isCommentsRangeStart(block, range)) {
                    const QStringList authors
                        = range.format.property(TextBlockStyle::PropertyCommentsAuthors)
                              .toStringList();
                    const QStringList dates
                        = range.format.property(TextBlockStyle::PropertyCommentsDates)
                              .toStringList();

                    for (int commentIndex = 0; commentIndex < comments.size(); ++commentIndex) {
                        if (!_comments.isEmpty()) {
                            lastCommentIndex = _comments.lastKey() + 1;
                        }
                        _comments.insert(lastCommentIndex,
                                         QStringList()
                                             << comments.at(commentIndex)
                                             << authors.at(commentIndex) << dates.at(commentIndex));

                        documentXml.append(
                            QString("<w:commentRangeStart w:id=\"%1\"/>").arg(lastCommentIndex));
                    }
                }
                documentXml.append("<w:r>");
                documentXml.append("<w:rPr>");
                //
                // Заливка
                //
                if (!hasComments && range.format.hasProperty(QTextFormat::BackgroundBrush)) {
                    documentXml.append(QString("<w:shd w:fill=\"%1\" w:val=\"clear\"/>")
                                           // код цвета без решётки
                                           .arg(range.format.background().color().name().mid(1)));
                }
                //
                // Цвет текста
                //
                if (!hasComments && range.format.hasProperty(QTextFormat::ForegroundBrush)) {
                    documentXml.append(QString("<w:color w:val=\"%1\"/>")
                                           // код цвета без решётки
                                           .arg(range.format.foreground().color().name().mid(1)));
                }
                if (range.format.font().bold()) {
                    documentXml.append("<w:b/><w:bCs/>");
                }
                if (range.format.font().italic()) {
                    documentXml.append("<w:i/><w:iCs/>");
                }
                if (range.format.font().underline()) {
                    documentXml.append("<w:u w:val=\"single\"/>");
                }
                if (formatRangeSourceText.isRightToLeft()) {
                    documentXml.append("<w:rtl/>");
                }
                documentXml.append("</w:rPr>");
                //
                // Сам текст
                //
                documentXml.append(formatRangeText);
                documentXml.append("</w:r>");
                //
                // Текст комментария
                //
                if (hasComments && isCommentsRangeEnd(block, range)) {
                    for (int commentIndex = lastCommentIndex - comments.size() + 1;
                         commentIndex <= lastCommentIndex; ++commentIndex) {
                        documentXml.append(
                            QString("<w:commentRangeEnd w:id=\"%1\"/>"
                                    "<w:r><w:rPr/><w:commentReference w:id=\"%1\"/></w:r>")
                                .arg(commentIndex));
                    }
                }
            }
        }
        //
        // ... закрываем абзац
        //
        documentXml.append("</w:p>");
    }

    return documentXml;
}

void AbstractDocxExporter::Implementation::writeStaticData(
    QtZipWriter* _zip, const ExportOptions& _exportOptions) const
{
    //
    // Перечисление всех компонентов архива
    //
    QString contentTypesXml
        = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
          "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
          "<Default Extension=\"rels\" "
          "ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
          "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
          "<Override PartName=\"/_rels/.rels\" "
          "ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
          "<Override PartName=\"/word/_rels/document.xml.rels\" "
          "ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
          "<Override PartName=\"/word/styles.xml\" "
          "ContentType=\"application/"
          "vnd.openxmlformats-officedocument.wordprocessingml.styles+xml\"/>"
          "<Override PartName=\"/word/comments.xml\" "
          "ContentType=\"application/"
          "vnd.openxmlformats-officedocument.wordprocessingml.comments+xml\"/>";
    //
    // ... необходимы ли колонтитулы
    //
    if (needWriteHeader(_exportOptions)) {
        contentTypesXml.append("<Override PartName=\"/word/header1.xml\" "
                               "ContentType=\"application/"
                               "vnd.openxmlformats-officedocument.wordprocessingml.header+xml\"/>");
    }
    if (needWriteFooter(_exportOptions)) {
        contentTypesXml.append("<Override PartName=\"/word/footer1.xml\" "
                               "ContentType=\"application/"
                               "vnd.openxmlformats-officedocument.wordprocessingml.footer+xml\"/>");
    }
    contentTypesXml.append(
        "<Override PartName=\"/word/document.xml\" "
        "ContentType=\"application/"
        "vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
        "<Override PartName=\"/word/settings.xml\" "
        "ContentType=\"application/"
        "vnd.openxmlformats-officedocument.wordprocessingml.settings+xml\"/>"
        "</Types>");
    _zip->addFile(QString::fromLatin1("[Content_Types].xml"), contentTypesXml.toUtf8());

    //
    // Связи пакета
    //
    _zip->addFile(
        QString::fromLatin1("_rels/.rels"),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Target=\"word/document.xml\" Id=\"pkgRId0\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
        "officeDocument\"/>"
        "</Relationships>");

    //
    // Связи документа
    //
    QString documentXmlRels
        = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
          "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
          "<Relationship Id=\"docRId0\" "
          "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" "
          "Target=\"styles.xml\"/>"
          "<Relationship Id=\"docRId1\" "
          "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments\" "
          "Target=\"comments.xml\"/>";
    //
    // ... необходимы ли колонтитулы
    //
    if (needWriteHeader(_exportOptions)) {
        documentXmlRels.append("<Relationship Id=\"docRId2\" "
                               "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/"
                               "relationships/header\" Target=\"header1.xml\"/>");
    }
    if (needWriteFooter(_exportOptions)) {
        documentXmlRels.append("<Relationship Id=\"docRId3\" "
                               "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/"
                               "relationships/footer\" Target=\"footer1.xml\"/>");
    }
    documentXmlRels.append("<Relationship Id=\"docRId4\" "
                           "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/"
                           "relationships/settings\" Target=\"settings.xml\"/>");
    documentXmlRels.append("</Relationships>");
    _zip->addFile(QString::fromLatin1("word/_rels/document.xml.rels"), documentXmlRels.toUtf8());

    //
    // Настройки документа
    //
    QString wordSettings
        = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
          "<w:settings xmlns:o=\"urn:schemas-microsoft-com:office:office\" "
          "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
          "xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\" "
          "xmlns:v=\"urn:schemas-microsoft-com:vml\" "
          "xmlns:w10=\"urn:schemas-microsoft-com:office:word\" "
          "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
          "xmlns:sl=\"http://schemas.openxmlformats.org/schemaLibrary/2006/main\">";
    wordSettings += "<w:characterSpacingControl w:val=\"doNotCompress\"/>"
                    "<w:footnotePr></w:footnotePr>"
                    "<w:endnotePr></w:endnotePr>"
                    "</w:settings>";
    _zip->addFile(QString::fromLatin1("word/settings.xml"), wordSettings.toUtf8());
}

void AbstractDocxExporter::Implementation::writeStyles(QtZipWriter* _zip,
                                                       const ExportOptions& _exportOptions) const
{
    //
    // Сформируем xml стилей
    //
    const QString languageCode = QLocale().uiLanguages().value(1, "en-US");
    QString styleXml
        = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
          "<w:styles xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
          "<w:docDefaults>"
          "<w:rPrDefault>"
          "<w:rPr>"
          "<w:rFonts w:asciiTheme=\"minorHAnsi\" w:eastAsiaTheme=\"minorEastAsia\" "
          "w:hAnsiTheme=\"minorHAnsi\" w:cstheme=\"minorBidi\"/>"
          "<w:sz w:val=\"22\"/>"
          "<w:szCs w:val=\"22\"/>"
          "<w:lang w:val=\""
        + languageCode + "\" w:eastAsia=\"" + languageCode + "\" w:bidi=\""
        + (QLocale().language() == QLocale::Hebrew ? "he-IL" : "ar-SA")
        + "\"/>"
          "</w:rPr>"
          "</w:rPrDefault>"
          "<w:pPrDefault><w:pPr><w:spacing w:after=\"200\" w:line=\"276\" "
          "w:lineRule=\"auto\"/></w:pPr></w:pPrDefault>"
          "</w:docDefaults>";

    //
    // Настройки в соответсвии со стилем
    //
    const auto& audioplayTemplate = q->documentTemplate(_exportOptions);
    const QString defaultFontFamily
        = audioplayTemplate.paragraphStyle(TextParagraphType::Description).font().family();
    for (const auto& paragraphType : q->paragraphTypes()) {
        const auto blockStyle = audioplayTemplate.paragraphStyle(paragraphType);
        styleXml.append(docxBlockStyle(blockStyle, defaultFontFamily));
        const auto onHalfPage = true;
        styleXml.append(docxBlockStyle(blockStyle, defaultFontFamily, onHalfPage));
    }

    styleXml.append("</w:styles>");

    //
    // Запишем стили в архив
    //
    _zip->addFile(QString::fromLatin1("word/styles.xml"), styleXml.toUtf8());
}

void AbstractDocxExporter::Implementation::writeHeader(QtZipWriter* _zip,
                                                       const ExportOptions& _exportOptions) const
{
    const auto& audioplayTemplate = q->documentTemplate(_exportOptions);

    //
    // Если нужна нумерация вверху
    //
    const bool needPrintPageNumbers
        = audioplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignTop);
    if (!needPrintPageNumbers && _exportOptions.header.isEmpty()) {
        return;
    }

    QString headerXml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
                        "<w:hdr xmlns:o=\"urn:schemas-microsoft-com:office:office\" "
                        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/"
                        "relationships\" xmlns:v=\"urn:schemas-microsoft-com:vml\" "
                        "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/"
                        "main\" xmlns:w10=\"urn:schemas-microsoft-com:office:word\" "
                        "xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/"
                        "wordprocessingDrawing\">";

    headerXml.append("<w:p><w:pPr><w:jc w:val=\"");
    if (audioplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignLeft) || !needPrintPageNumbers) {
        headerXml.append("left");
    } else if (audioplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignCenter)) {
        headerXml.append("center");
    } else {
        headerXml.append("right");
    }
    headerXml.append("\"/><w:rPr/></w:pPr>");

    const QString pageNumbersXml = needPrintPageNumbers
        ? "<w:r><w:rPr/><w:fldChar w:fldCharType=\"begin\"/></w:r><w:r><w:instrText> PAGE "
          "</w:instrText></w:r><w:r><w:fldChar "
          "w:fldCharType=\"separate\"/></w:r><w:r><w:t>0</w:t></w:r><w:r><w:fldChar "
          "w:fldCharType=\"end\"/></w:r>"
        : "";
    const QString pageHeaderXml
        = QString("<w:r><w:rPr/><w:t>%1</w:t></w:r>").arg(_exportOptions.header);
    const QString pageHeaderSeparatorXml = "<w:r><w:rPr/><w:t> </w:t></w:r>";
    if (audioplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignRight)) {
        headerXml.append(pageHeaderXml);
        headerXml.append(pageHeaderSeparatorXml);
        headerXml.append(pageNumbersXml);
    } else {
        headerXml.append(pageNumbersXml);
        headerXml.append(pageHeaderSeparatorXml);
        headerXml.append(pageHeaderXml);
    }

    headerXml.append("</w:p></w:hdr>");

    //
    // Запишем верхний колонтитул в архив
    //
    _zip->addFile(QString::fromLatin1("word/header1.xml"), headerXml.toUtf8());
}

void AbstractDocxExporter::Implementation::writeFooter(QtZipWriter* _zip,
                                                       const ExportOptions& _exportOptions) const
{
    const auto& audioplayTemplate = q->documentTemplate(_exportOptions);

    //
    // Если нужна нумерация внизу
    //
    const bool needPrintPageNumbers
        = audioplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignBottom);
    if (!needPrintPageNumbers && _exportOptions.footer.isEmpty()) {
        return;
    }

    QString footerXml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
                        "<w:ftr xmlns:o=\"urn:schemas-microsoft-com:office:office\" "
                        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/"
                        "relationships\" xmlns:v=\"urn:schemas-microsoft-com:vml\" "
                        "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/"
                        "main\" xmlns:w10=\"urn:schemas-microsoft-com:office:word\" "
                        "xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/"
                        "wordprocessingDrawing\">";

    footerXml.append("<w:p><w:pPr><w:jc w:val=\"");
    if (audioplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignLeft) || !needPrintPageNumbers) {
        footerXml.append("left");
    } else if (audioplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignCenter)) {
        footerXml.append("center");
    } else {
        footerXml.append("right");
    }
    footerXml.append("\"/><w:rPr/></w:pPr>");

    const QString pageNumbersXml = needPrintPageNumbers
        ? "<w:r><w:rPr/><w:fldChar w:fldCharType=\"begin\"/></w:r><w:r><w:instrText> PAGE "
          "</w:instrText></w:r><w:r><w:fldChar "
          "w:fldCharType=\"separate\"/></w:r><w:r><w:t>0</w:t></w:r><w:r><w:fldChar "
          "w:fldCharType=\"end\"/></w:r>"
        : "";
    const QString pageFooterXml
        = QString("<w:r><w:rPr/><w:t>%1</w:t></w:r>").arg(_exportOptions.footer);
    const QString pageFooterSeparatorXml = "<w:r><w:rPr/><w:t> </w:t></w:r>";
    if (audioplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignRight)) {
        footerXml.append(pageFooterXml);
        footerXml.append(pageFooterSeparatorXml);
        footerXml.append(pageNumbersXml);
    } else {
        footerXml.append(pageNumbersXml);
        footerXml.append(pageFooterSeparatorXml);
        footerXml.append(pageFooterXml);
    }

    footerXml.append("</w:p></w:ftr>");

    //
    // Запишем верхний колонтитул в архив
    //
    _zip->addFile(QString::fromLatin1("word/footer1.xml"), footerXml.toUtf8());
}

void AbstractDocxExporter::Implementation::writeDocument(QtZipWriter* _zip,
                                                         TextDocument* _audioplayText,
                                                         QMap<int, QStringList>& _comments,
                                                         const ExportOptions& _exportOptions) const
{
    QString documentXml
        = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
          "<w:document xmlns:o=\"urn:schemas-microsoft-com:office:office\" "
          "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
          "xmlns:v=\"urn:schemas-microsoft-com:vml\" "
          "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
          "xmlns:w10=\"urn:schemas-microsoft-com:office:word\" "
          "xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\">"
          "<w:body>";

    //
    // Данные считываются из исходного документа, определяется тип блока
    // и записываются прямо в файл
    //
    TextCursor documentCursor(_audioplayText);
    do {
        documentXml.append(docxText(_comments, documentCursor, _exportOptions));

        //
        // Переходим к следующему параграфу
        //
    } while (documentCursor.movePosition(QTextCursor::NextBlock));

    //
    // В конце идёт блок настроек страницы
    //
    documentXml.append("<w:sectPr>");
    //
    // ... колонтитулы
    //
    if (needWriteHeader(_exportOptions)) {
        documentXml.append("<w:headerReference w:type=\"default\" r:id=\"docRId2\"/>");
    }
    if (needWriteFooter(_exportOptions)) {
        documentXml.append("<w:footerReference w:type=\"default\" r:id=\"docRId3\"/>");
    }
    //
    // ... размер страницы
    //
    const auto& audioplayTemplate = q->documentTemplate(_exportOptions);
    QSizeF paperSize = QPageSize(audioplayTemplate.pageSizeId()).size(QPageSize::Millimeter);
    documentXml.append(QString("<w:pgSz w:w=\"%1\" w:h=\"%2\"/>")
                           .arg(mmToTwips(paperSize.width()))
                           .arg(mmToTwips(paperSize.height())));
    //
    // ... поля документа
    //
    documentXml.append(QString("<w:pgMar w:left=\"%1\" w:right=\"%2\" w:top=\"%3\" w:bottom=\"%4\" "
                               "w:header=\"%5\" w:footer=\"%6\" w:gutter=\"0\"/>")
                           .arg(mmToTwips(audioplayTemplate.pageMargins().left()))
                           .arg(mmToTwips(audioplayTemplate.pageMargins().right()))
                           .arg(mmToTwips(audioplayTemplate.pageMargins().top()))
                           .arg(mmToTwips(audioplayTemplate.pageMargins().bottom()))
                           .arg(mmToTwips(audioplayTemplate.pageMargins().top() / 2))
                           .arg(mmToTwips(audioplayTemplate.pageMargins().bottom() / 2)));
    //
    // ... нужна ли титульная страница
    //
    if (_exportOptions.includeTiltePage) {
        documentXml.append("<w:titlePg/>");
    }
    //
    // ... нумерация страниц
    //
    int pageNumbersStartFrom = _exportOptions.includeTiltePage ? 0 : 1;
    documentXml.append(
        QString("<w:pgNumType w:fmt=\"decimal\" w:start=\"%1\"/>").arg(pageNumbersStartFrom));
    //
    // ... конец блока настроек страницы
    //
    documentXml.append("<w:textDirection w:val=\"lrTb\"/>"
                       "</w:sectPr>");

    documentXml.append("</w:body></w:document>");

    //
    // Запишем документ в архив
    //
    _zip->addFile(QString::fromLatin1("word/document.xml"), documentXml.toUtf8());
}

void AbstractDocxExporter::Implementation::writeComments(
    QtZipWriter* _zip, const QMap<int, QStringList>& _comments) const
{
    QString headerXml
        = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
          "<w:comments xmlns:o=\"urn:schemas-microsoft-com:office:office\" "
          "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
          "xmlns:v=\"urn:schemas-microsoft-com:vml\" "
          "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
          "xmlns:w10=\"urn:schemas-microsoft-com:office:word\" "
          "xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\">";

    int commentIndex = 0;
    const int commentTextIndex = 0;
    const int commentAuthorIndex = 1;
    const int commentDateIndex = 2;
    for (const auto& comment : _comments) {
        headerXml.append(
            QString("<w:comment w:id=\"%1\" w:author=\"%2\" w:date=\"%3\" w:initials=\"\">"
                    "<w:p><w:r><w:rPr></w:rPr><w:t>%4</w:t></w:r></w:p>"
                    "</w:comment>")
                .arg(commentIndex++)
                .arg(comment.at(commentAuthorIndex), comment.at(commentDateIndex),
                     TextHelper::toHtmlEscaped(comment.at(commentTextIndex))));
    }

    headerXml.append("</w:comments>");

    //
    // Запишем комментарии в архив
    //
    _zip->addFile(QString::fromLatin1("word/comments.xml"), headerXml.toUtf8());
}


// ****


AbstractDocxExporter::AbstractDocxExporter()
    : d(new Implementation(this))
{
}

AbstractDocxExporter::~AbstractDocxExporter() = default;

void AbstractDocxExporter::exportTo(TextModel* _model, ExportOptions& _exportOptions) const
{
    //
    // Открываем документ на запись
    //
    QFile docxFile(_exportOptions.filePath);
    if (!docxFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    //
    // Пишем все статичные данные в файл
    //
    QtZipWriter zip(&docxFile);
    if (zip.status() == QtZipWriter::NoError) {
        //
        // Конвертируем документ в DOCX
        //
        // ... статичные данные
        //
        d->writeStaticData(&zip, _exportOptions);
        //
        // ... стили
        //
        d->writeStyles(&zip, _exportOptions);
        //
        // ... колонтитулы
        //
        d->writeHeader(&zip, _exportOptions);
        d->writeFooter(&zip, _exportOptions);
        //
        // ... документ
        //
        QMap<int, QStringList> comments;
        QScopedPointer<TextDocument> document(prepareDocument(_model, _exportOptions));
        d->writeDocument(&zip, document.data(), comments, _exportOptions);
        //
        // ... комментарии
        //
        d->writeComments(&zip, comments);
    }
    zip.close();
    docxFile.close();
}

void AbstractDocxExporter::processBlock(const TextCursor& _cursor,
                                        const ExportOptions& _exportOptions,
                                        QString& _documentXml) const
{
    Q_UNUSED(_cursor)
    Q_UNUSED(_exportOptions)
    Q_UNUSED(_documentXml)
}

} // namespace BusinessLayer
