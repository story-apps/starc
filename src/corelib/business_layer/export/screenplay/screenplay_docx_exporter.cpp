#include "screenplay_docx_exporter.h"

#include "qtzip/QtZipWriter"
#include "screenplay_export_options.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/text/text_model_splitter_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
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
    return 56.692913386 * _mm;
}

/**
 * @brief Перевести пиксели в твипсы
 */
int pxToTwips(qreal _px)
{
    return mmToTwips(MeasurementHelper::pxToMm(_px));
}

/**
 * @brief Пронумерованный список типов блоков
 */
const QMap<int, TextParagraphType>& paragraphTypes()
{
    static QMap<int, TextParagraphType> s_types;
    if (s_types.isEmpty()) {
        int i = 0;
        s_types.insert(i++, TextParagraphType::Undefined);
        s_types.insert(i++, TextParagraphType::UnformattedText);
        s_types.insert(i++, TextParagraphType::SceneHeading);
        s_types.insert(i++, TextParagraphType::SceneCharacters);
        s_types.insert(i++, TextParagraphType::Action);
        s_types.insert(i++, TextParagraphType::Character);
        s_types.insert(i++, TextParagraphType::Parenthetical);
        s_types.insert(i++, TextParagraphType::Dialogue);
        s_types.insert(i++, TextParagraphType::Lyrics);
        s_types.insert(i++, TextParagraphType::Transition);
        s_types.insert(i++, TextParagraphType::Shot);
        s_types.insert(i++, TextParagraphType::InlineNote);
        s_types.insert(i++, TextParagraphType::SequenceHeading);
        s_types.insert(i++, TextParagraphType::SequenceFooter);
    }
    return s_types;
}

/**
 * @brief Шаблон экспорта
 */
const ScreenplayTemplate& exportTemplate(const ScreenplayExportOptions& _exportOptions)
{
    return TemplatesFacade::screenplayTemplate(_exportOptions.templateId);
}

/**
 * @brief Необходимо ли писать верхний колонтитул
 */
bool needWriteHeader(const ScreenplayExportOptions& _exportOptions)
{
    return exportTemplate(_exportOptions).pageNumbersAlignment().testFlag(Qt::AlignTop)
        || !_exportOptions.header.isEmpty();
}

/**
 * @brief Необходимо ли писать нижний колонтитул
 */
bool needWriteFooter(const ScreenplayExportOptions& _exportOptions)
{
    return exportTemplate(_exportOptions).pageNumbersAlignment().testFlag(Qt::AlignBottom)
        || !_exportOptions.footer.isEmpty();
}

/**
 * @brief Название типа параграфа в специальном формате
 */
QString paragraphTypeName(TextParagraphType _type, const QString suffix = "",
                          const QString& _separator = "")
{
    return (toString(_type) + suffix).toUpper().replace("_", _separator);
}

/**
 * @brief Сформировать строку DOCX-формата для параметра выравнивания
 */
QString docxAlignment(Qt::Alignment _alignment)
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

/**
 * @brief Сформировать строку DOCX-стиля из стиля блока
 */
QString docxBlockStyle(const TextBlockStyle& _style, const QString& _defaultFontFamily,
                       bool _onHalfPage = false)
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
                          .arg(pxToTwips(_style.blockFormat(_onHalfPage).topMargin()))
                          .arg(pxToTwips(_style.blockFormat(_onHalfPage).bottomMargin())));
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
        blockStyle.append("<w:caps/>");
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

/**
 * @brief Является ли заданный формат открывающим комментарий
 */
bool isCommentsRangeStart(const QTextBlock& _block, const QTextLayout::FormatRange& _range)
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

/**
 * @brief Является ли заданный формат закрывающим комментарий
 */
bool isCommentsRangeEnd(const QTextBlock& _block, const QTextLayout::FormatRange& _range)
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

/**
 * @brief Сформировать текст блока документа в зависимости от его стиля и оформления
 */
QString docxText(QMap<int, QStringList>& _comments, const TextCursor& _cursor,
                 const ScreenplayExportOptions& _exportOptions)
{
    //
    // Блокируем сигналы от документа - по ходу экспорта мы будем изменять документ,
    // но нам не нужно при этом корректировать переносы в нём
    //
    QSignalBlocker documentSignalBlocker(_cursor.document());

    const auto& screenplayTemplate = exportTemplate(_exportOptions);
    auto tableWidth = [&screenplayTemplate] {
        const QSizeF paperSize
            = QPageSize(screenplayTemplate.pageSizeId()).size(QPageSize::Millimeter);
        return mmToTwips(paperSize.width() - screenplayTemplate.pageMargins().left()
                         - screenplayTemplate.pageMargins().right());
    };

    QString documentXml;

    //
    // Получим стиль параграфа
    //
    const QTextBlock block = _cursor.block();
    const auto currentBlockType = TextBlockStyle::forBlock(block);
    const auto correctedBlockType = currentBlockType == TextParagraphType::SceneHeadingShadow
        ? TextParagraphType::SceneHeading
        : currentBlockType;

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
                    .arg(pxToTwips(_cursor.blockFormat().topMargin()))
                    .arg(pxToTwips(_cursor.blockFormat().bottomMargin())));
        }
        documentXml.append("<w:rPr/></w:pPr>");
        //
        // ... текст блока
        //
        const auto textFormats = _cursor.block().textFormats();
        for (const auto& formatRange : textFormats) {
            documentXml.append(
                QString("<w:r><w:rPr><w:rFonts w:ascii=\"%1\" w:hAnsi=\"%1\"/><w:b "
                        "w:val=\"%2\"/><w:i w:val=\"%3\"/><w:sz w:val=\"%4\"/><w:szCs "
                        "w:val=\"%4\"/><w:u w:val=\"%5\"/></w:rPr><w:t>%6</w:t></w:r>")
                    .arg(formatRange.format.font().family())
                    .arg(formatRange.format.font().bold() ? "true" : "false")
                    .arg(formatRange.format.font().italic() ? "true" : "false")
                    .arg(MeasurementHelper::pxToPt(formatRange.format.font().pixelSize()) * 2)
                    .arg(formatRange.format.font().underline() ? "single" : "none")
                    .arg(TextHelper::toHtmlEscaped(
                        _cursor.block().text().mid(formatRange.start, formatRange.length))));
        }
        documentXml.append("</w:p>");
    }
    //
    // ... начало и конец таблицы
    //
    else if (currentBlockType == TextParagraphType::PageSplitter) {
        const auto blockData = dynamic_cast<TextBlockData*>(block.userData());
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
                const int middleColumnWidth = pxToTwips(screenplayTemplate.pageSplitterWidth());
                const int leftColumnWidth = (fullTableWidth - middleColumnWidth)
                    * screenplayTemplate.leftHalfOfPageWidthPercents() / 100.;
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
                const int middleColumnWidth = pxToTwips(screenplayTemplate.pageSplitterWidth());
                documentXml.append(
                    QString("<w:tcW w:w=\"%1\" w:type=\"dxa\"/>").arg(middleColumnWidth));
                documentXml.append(
                    "<w:tcBorders/></w:tcPr>"
                    "<w:p><w:pPr><w:pStyle w:val=\"Normal\"/><w:rPr/></w:pPr>"
                    "<w:r><w:rPr><w:b w:val=\"false\"/><w:i w:val=\"false\"/><w:u "
                    "w:val=\"none\"/><w:rtl w:val=\"false\"/></w:rPr><w:t "
                    "xml:space=\"preserve\"></w:t></w:r></w:p></w:tc><w:tc><w:tcPr>");
                const int leftColumnWidth = (fullTableWidth - middleColumnWidth)
                    * screenplayTemplate.leftHalfOfPageWidthPercents() / 100.;
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
            != exportTemplate(_exportOptions).paragraphStyle(currentBlockType).align()) {
            documentXml.append(docxAlignment(block.blockFormat().alignment()));
        }

        //
        // ... если необходимо, добавляем номер сцены
        //
        if (currentBlockType == TextParagraphType::SceneHeading
            && _exportOptions.showScenesNumbers) {
            const auto blockData = dynamic_cast<TextBlockData*>(block.userData());
            if (blockData != nullptr) {
                const auto sceneItem
                    = static_cast<ScreenplayTextModelSceneItem*>(blockData->item()->parent());
                const auto sceneNumber = sceneItem->number()->text + " ";
                const QFontMetrics fontMetrics(block.charFormat().font());
                documentXml.append(
                    QString("<w:ind w:left=\"%1\" w:right=\"%2\" w:hanging=\"%3\" />")
                        .arg(pxToTwips(block.blockFormat().leftMargin()))
                        .arg(pxToTwips(block.blockFormat().rightMargin()))
                        .arg(pxToTwips(fontMetrics.horizontalAdvance(sceneNumber))));

                auto cursor = _cursor;
                cursor.setPosition(block.position());
                cursor.insertText(sceneNumber, block.charFormat());
            }
        }
        //
        // ... для ремарки подхачиваем отступ перед блоком и ширину самого блока
        //
        else if (currentBlockType == TextParagraphType::Parenthetical && !block.text().isEmpty()) {
            const QLatin1String prefix("(");
            const QLatin1String postfix(")");
            const QFontMetrics fontMetrics(block.charFormat().font());
            documentXml.append(QString("<w:ind w:left=\"%1\" w:right=\"%2\" w:hanging=\"%3\" />")
                                   .arg(pxToTwips(block.blockFormat().leftMargin()))
                                   .arg(pxToTwips(block.blockFormat().rightMargin()
                                                  - fontMetrics.horizontalAdvance(postfix)))
                                   .arg(pxToTwips(fontMetrics.horizontalAdvance(prefix))));

            auto cursor = _cursor;
            cursor.setPosition(block.position());
            cursor.insertText(prefix, block.charFormat());
            cursor.movePosition(TextCursor::EndOfBlock);
            cursor.insertText(postfix, block.charFormat());
        }


        //
        //  ... текст абзаца
        //
        const QString blockText = block.text();
        documentXml.append("<w:rPr/></w:pPr>");
        const auto textFormats = block.textFormats();
        for (const auto& range : textFormats) {
            //
            // ... не является редакторской заметкой
            //
            if (range.format.boolProperty(TextBlockStyle::PropertyIsReviewMark) == false) {
                //
                // ... стандартный для абзаца
                //
                if (range.format == block.charFormat()) {
                    documentXml.append("<w:r><w:rPr>");
                    documentXml.append(
                        QString("<w:rtl w:val=\"%1\"/>")
                            .arg(QLocale().textDirection() == Qt::RightToLeft ? "true" : "false"));
                    documentXml.append(QString("</w:rPr><w:t xml:space=\"preserve\">%2</w:t></w:r>")
                                           .arg(TextHelper::toHtmlEscaped(
                                               blockText.mid(range.start, range.length))));
                }
                //
                // ... не стандартный
                //
                else {
                    documentXml.append("<w:r>");
                    documentXml.append("<w:rPr>");
                    documentXml.append(QString("<w:b w:val=\"%1\"/>")
                                           .arg(range.format.font().bold() ? "true" : "false"));
                    documentXml.append(QString("<w:i w:val=\"%1\"/>")
                                           .arg(range.format.font().italic() ? "true" : "false"));
                    documentXml.append(
                        QString("<w:u w:val=\"%1\"/>")
                            .arg(range.format.font().underline() ? "single" : "none"));
                    documentXml.append(
                        QString("<w:rtl w:val=\"%1\"/>")
                            .arg(QLocale().textDirection() == Qt::RightToLeft ? "true" : "false"));
                    documentXml.append("</w:rPr>");
                    //
                    // Сам текст
                    //
                    documentXml.append(QString("<w:t xml:space=\"preserve\">%2</w:t>")
                                           .arg(TextHelper::toHtmlEscaped(
                                               blockText.mid(range.start, range.length))));
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
                documentXml.append(QString("<w:b w:val=\"%1\"/>")
                                       .arg(range.format.font().bold() ? "true" : "false"));
                documentXml.append(QString("<w:i w:val=\"%1\"/>")
                                       .arg(range.format.font().italic() ? "true" : "false"));
                documentXml.append(QString("<w:u w:val=\"%1\"/>")
                                       .arg(range.format.font().underline() ? "single" : "none"));
                documentXml.append(
                    QString("<w:rtl w:val=\"%1\"/>")
                        .arg(QLocale().textDirection() == Qt::RightToLeft ? "true" : "false"));
                documentXml.append("</w:rPr>");
                //
                // Сам текст
                //
                documentXml.append(
                    QString("<w:t xml:space=\"preserve\">%2</w:t>")
                        .arg(TextHelper::toHtmlEscaped(blockText.mid(range.start, range.length))));
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

void writeStaticData(QtZipWriter* _zip, const ScreenplayExportOptions& _exportOptions)
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

void writeStyles(QtZipWriter* _zip, const ScreenplayExportOptions& _exportOptions)
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
    const auto& screenplayTemplate = exportTemplate(_exportOptions);
    const QString defaultFontFamily
        = screenplayTemplate.paragraphStyle(TextParagraphType::Action).font().family();
    for (const auto& paragraphType : paragraphTypes()) {
        const auto blockStyle = screenplayTemplate.paragraphStyle(paragraphType);
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

void writeHeader(QtZipWriter* _zip, const ScreenplayExportOptions& _exportOptions)
{
    const auto& screenplayTemplate = exportTemplate(_exportOptions);

    //
    // Если нужна нумерация вверху
    //
    const bool needPrintPageNumbers
        = screenplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignTop);
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
    if (screenplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignLeft)
        || !needPrintPageNumbers) {
        headerXml.append("left");
    } else if (screenplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignCenter)) {
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
    if (screenplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignRight)) {
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

void writeFooter(QtZipWriter* _zip, const ScreenplayExportOptions& _exportOptions)
{
    const auto& screenplayTemplate = exportTemplate(_exportOptions);

    //
    // Если нужна нумерация внизу
    //
    const bool needPrintPageNumbers
        = screenplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignBottom);
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
    if (screenplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignLeft)
        || !needPrintPageNumbers) {
        footerXml.append("left");
    } else if (screenplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignCenter)) {
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
    if (screenplayTemplate.pageNumbersAlignment().testFlag(Qt::AlignRight)) {
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

void writeDocument(QtZipWriter* _zip, ScreenplayTextDocument* _screenplayText,
                   QMap<int, QStringList>& _comments, const ScreenplayExportOptions& _exportOptions)
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
    TextCursor documentCursor(_screenplayText);
    while (!documentCursor.atEnd()) {
        documentXml.append(docxText(_comments, documentCursor, _exportOptions));

        //
        // Переходим к следующему параграфу
        //
        documentCursor.movePosition(QTextCursor::EndOfBlock);
        documentCursor.movePosition(QTextCursor::NextBlock);
    }

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
    const auto& screenplayTemplate = exportTemplate(_exportOptions);
    QSizeF paperSize = QPageSize(screenplayTemplate.pageSizeId()).size(QPageSize::Millimeter);
    documentXml.append(QString("<w:pgSz w:w=\"%1\" w:h=\"%2\"/>")
                           .arg(mmToTwips(paperSize.width()))
                           .arg(mmToTwips(paperSize.height())));
    //
    // ... поля документа
    //
    documentXml.append(QString("<w:pgMar w:left=\"%1\" w:right=\"%2\" w:top=\"%3\" w:bottom=\"%4\" "
                               "w:header=\"%5\" w:footer=\"%6\" w:gutter=\"0\"/>")
                           .arg(mmToTwips(screenplayTemplate.pageMargins().left()))
                           .arg(mmToTwips(screenplayTemplate.pageMargins().right()))
                           .arg(mmToTwips(screenplayTemplate.pageMargins().top()))
                           .arg(mmToTwips(screenplayTemplate.pageMargins().bottom()))
                           .arg(mmToTwips(screenplayTemplate.pageMargins().top() / 2))
                           .arg(mmToTwips(screenplayTemplate.pageMargins().bottom() / 2)));
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

void writeComments(QtZipWriter* _zip, const QMap<int, QStringList>& _comments)
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

} // namespace

void ScreenplayDocxExporter::exportTo(ScreenplayTextModel* _model,
                                      const ScreenplayExportOptions& _exportOptions) const
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
        writeStaticData(&zip, _exportOptions);
        //
        // ... стили
        //
        writeStyles(&zip, _exportOptions);
        //
        // ... колонтитулы
        //
        writeHeader(&zip, _exportOptions);
        writeFooter(&zip, _exportOptions);
        //
        // ... документ
        //
        QMap<int, QStringList> comments;
        QScopedPointer<ScreenplayTextDocument> document(prepareDocument(_model, _exportOptions));
        writeDocument(&zip, document.data(), comments, _exportOptions);
        //
        // ... комментарии
        //
        writeComments(&zip, comments);
    }
    zip.close();
    docxFile.close();
}

} // namespace BusinessLayer
