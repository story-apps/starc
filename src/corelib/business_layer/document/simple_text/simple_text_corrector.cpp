#include "simple_text_corrector.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/text/text_model_item.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/run_once.h>

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextDocument>

#include <cmath>

using BusinessLayer::TemplatesFacade;
using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;


namespace BusinessLayer {

namespace {
/**
 * @brief Список символов пунктуации, разделяющие предложения
 */
const QRegularExpression kPunctuationCharacter("([.]|[!]|[?]|[:]|[;]|[…])");

/**
 * @brief Обновить компановку текста для блока
 */
void updateBlockLayout(qreal _pageWidth, const QTextBlock& _block)
{
    _block.layout()->setText(_block.text());
    _block.layout()->beginLayout();
    forever
    {
        QTextLine line = _block.layout()->createLine();
        if (!line.isValid()) {
            break;
        }

        line.setLineWidth(_pageWidth - _block.blockFormat().leftMargin()
                          - _block.blockFormat().rightMargin());
    }
    _block.layout()->endLayout();
}

/**
 * @brief Вставить блок, обновив при этом лэйаут вставленного блока
 */
void insertBlock(int _pageWidth, QTextCursor& _cursor)
{
    _cursor.insertBlock();
    updateBlockLayout(_pageWidth, _cursor.block());
}
} // namespace


class SimpleTextCorrector::Implementation
{
public:
    explicit Implementation(SimpleTextCorrector* _q);

    /**
     * @brief Корректируемый документ
     */
    QTextDocument* document() const;

    /**
     * @brief Обновить видимость блоков в заданном интервале
     */
    void updateBlocksVisibility(int _position);

    /**
     * @brief Обновить высоту блоков при необходимости
     */
    void updateBlocksHeight(int _position, int _charsChanged);

    /**
     * @brief Скорректировать текст сценария
     */
    void correctPageBreaks(int _position = -1);

    //
    // Функции работающие в рамках текущей коррекции
    //

    /**
     * @brief Сместить текущий блок вместе с тремя предыдущими на следующую страницу
     */
    void moveCurrentBlockWithThreePreviousToNextPage(const QTextBlock& _prePrePreviousBlock,
                                                     const QTextBlock& _prePreviousBlock,
                                                     const QTextBlock& _previousBlock,
                                                     qreal _pageHeight, qreal _pageWidth,
                                                     TextCursor& _cursor, QTextBlock& _block,
                                                     qreal& _lastBlockHeight);

    /**
     * @brief Сместить текущий блок вместе с двумя предыдущими на следующую страницу
     */
    void moveCurrentBlockWithTwoPreviousToNextPage(const QTextBlock& _prePreviousBlock,
                                                   const QTextBlock& _previousBlock,
                                                   qreal _pageHeight, qreal _pageWidth,
                                                   TextCursor& _cursor, QTextBlock& _block,
                                                   qreal& _lastBlockHeight);

    /**
     * @brief Сместить текущий блок вместе с предыдущим на следующую страницу
     */
    void moveCurrentBlockWithPreviousToNextPage(const QTextBlock& _previousBlock, qreal _pageHeight,
                                                qreal _pageWidth, TextCursor& _cursor,
                                                QTextBlock& _block, qreal& _lastBlockHeight);

    /**
     * @brief Сместить текущий блок на следующую страницу
     */
    void moveCurrentBlockToNextPage(const QTextBlockFormat& _blockFormat, qreal _blockHeight,
                                    qreal _pageHeight, qreal _pageWidth, TextCursor& _cursor,
                                    QTextBlock& _block, qreal& _lastBlockHeight);

    //
    // Вспомогательные функции
    //

    /**
     * @brief Найти предыдущий блок
     */
    QTextBlock findPreviousBlock(const QTextBlock& _block);

    /**
     * @brief Найти следующий блок, который не является декорацией
     */
    QTextBlock findNextBlock(const QTextBlock& _block);

    /**
     * @brief Сместить блок в начало следующей страницы
     * @param _block - блок для смещения
     * @param _spaceToPageEnd - количество места до конца страницы
     * @param _pageHeight - высота страницы
     * @param _cursor - курсор редактироуемого документа
     */
    void moveBlockToNextPage(const QTextBlock& _block, qreal _spaceToPageEnd, qreal _pageHeight,
                             qreal _pageWidth, TextCursor& _cursor);

    //
    // Данные
    //

    SimpleTextCorrector* q = nullptr;

    /**
     * @brief Необходимо ли корректировать текст на разрывах страниц
     */
    bool needToCorrectPageBreaks = true;

    /**
     * @brief Размер документа при последней проверке
     */
    QSizeF lastDocumentSize;

    /**
     * @brief Вспомогательная информация о текущем блоке
     */
    struct {
        /**
         * @brief Номер текущего блока при корректировке
         * @note Используем собственный счётчик номеров, т.к. во время
         *       коррекций номера блоков могут скакать в QTextBlock
         */
        int number = 0;

        /**
         * @brief Находится ли блок в таблице
         */
        bool inTable = false;

        /**
         * @brief Находится ли блок в первой колонке таблицы
         */
        bool inFirstColumn = false;

        /**
         * @brief Положение верхней и нижней границ таблицы
         */
        qreal tableTop = std::numeric_limits<qreal>::min();
        qreal tableBottom = std::numeric_limits<qreal>::min();
    } currentBlockInfo;

    /**
     * @brief Структура элемента модели блоков
     */
    struct BlockInfo {
        BlockInfo() = default;
        explicit BlockInfo(qreal _height, qreal _top, TextParagraphType _type)
            : height(_height)
            , top(_top)
            , type(_type)
        {
        }

        /**
         * @brief Валиден ли блок
         */
        bool isValid() const
        {
            return !std::isnan(height) && !std::isnan(top);
        }

        /**
         * @brief Высота блока
         */
        qreal height = std::numeric_limits<qreal>::quiet_NaN();

        /**
         * @brief Позиция блока от начала страницы
         */
        qreal top = std::numeric_limits<qreal>::quiet_NaN();

        /**
         * @brief Тип блока
         */
        TextParagraphType type = TextParagraphType::Undefined;
    };

    /**
     * @brief Модель параметров блоков
     */
    QVector<BlockInfo> blockItems;
};

SimpleTextCorrector::Implementation::Implementation(SimpleTextCorrector* _q)
    : q(_q)
{
}

QTextDocument* SimpleTextCorrector::Implementation::document() const
{
    return q->document();
}

void SimpleTextCorrector::Implementation::updateBlocksVisibility(int _position)
{
    //
    // Пробегаем документ и настраиваем видимые и невидимые блоки в соответствии с шаблоном,
    //

    const auto& currentTemplate = TemplatesFacade::simpleTextTemplate(q->templateId());
    TextCursor cursor(document());
    cursor.setPosition(std::max(0, _position));
    bool isTextChanged = false;

    bool isFirstVisibleBlock = cursor.block() == document()->begin();
    bool isFirstBlockAfterInvisible = true;
    auto block = cursor.block();
    while (block.isValid()) {
        const auto blockType = TextBlockStyle::forBlock(block);

        //
        // В некоторых случаях, мы попадаем сюда, когда документ не до конца настроен, поэтому
        // когда обнаруживается такая ситация, завершаем выполнение
        //
        if (blockType == TextParagraphType::Undefined) {
            break;
        }

        //
        // При необходимости корректируем видимость блока
        //
        const auto isBlockShouldBeVisible = [this, block] {
            //
            // Если не задан верхнеуровневый видимый элемент, то покажем
            //
            if (q->visibleTopLevelItem() == nullptr) {
                return true;
            }

            //
            // Если верхнеуровневый элемент задан и текущий элемент не является его дитём, то скроем
            //
            if (auto screenplayBlockData
                = static_cast<BusinessLayer::TextBlockData*>(block.userData());
                screenplayBlockData == nullptr || screenplayBlockData->item() == nullptr
                || !screenplayBlockData->item()->isChildOf(q->visibleTopLevelItem())) {
                return false;
            }

            //
            // А если является дитём, то покажем
            //
            return true;
        }();

        cursor.setPosition(block.position());
        auto blockFormat = cursor.blockFormat();
        const auto& paragraphStyleBlockFormat
            = currentTemplate.paragraphStyle(TextBlockStyle::forBlock(block))
                  .blockFormat(cursor.inTable());

        //
        // Корректируем параметры в кейсах
        // - сменилась видимость блока
        // - это первый видимый блок (у него не должно быть дополнительных отступов сверху)
        // - это первый блок который шёл после невидимых (он был первым видимым в предыдущем проходе
        //   и поэтому у него сброшены отступы)
        // - это не первый видимый блок и его высота отличается от канонической
        //
        if (block.isVisible() != isBlockShouldBeVisible
            || (isBlockShouldBeVisible && isFirstVisibleBlock)
            || (block.isVisible() && isBlockShouldBeVisible && isFirstBlockAfterInvisible)
            || (block.isVisible() && !isFirstVisibleBlock
                && !qFuzzyCompare(blockFormat.topMargin(),
                                  paragraphStyleBlockFormat.topMargin()))) {
            //
            // ... если это кейс с обновлением формата первого блока следующего за невидимым,
            //     то запомним, что мы его выполнили
            //
            if (block.isVisible() && isBlockShouldBeVisible && isFirstBlockAfterInvisible) {
                isFirstBlockAfterInvisible = false;
            }
            //
            // ... если блоку нужно настроить видимость, запустим операцию изменения
            //
            if (isTextChanged == false) {
                isTextChanged = true;
                cursor.beginEditBlock();
            }
            //
            // ... уберём отступы у скрытых блоков, чтобы они не ломали компоновку документа
            //
            if (!isBlockShouldBeVisible) {
                blockFormat.setTopMargin(0);
                blockFormat.setBottomMargin(0);
                blockFormat.setPageBreakPolicy(QTextFormat::PageBreak_Auto);
            }
            //
            // ... а для блоков, которые возвращаются для отображения, настроим отступы
            //
            else {
                blockFormat.setTopMargin(
                    isFirstVisibleBlock ? 0 : paragraphStyleBlockFormat.topMargin());
                blockFormat.setBottomMargin(paragraphStyleBlockFormat.bottomMargin());
                blockFormat.setPageBreakPolicy(isFirstVisibleBlock
                                                   ? QTextFormat::PageBreak_Auto
                                                   : paragraphStyleBlockFormat.pageBreakPolicy());
            }
            //
            // ... применим настроенный стиль блока
            //
            cursor.setBlockFormat(blockFormat);
            //
            // ... собственно настраиваем видимость
            //
            block.setVisible(isBlockShouldBeVisible);
        }
        //
        if (isFirstVisibleBlock && isBlockShouldBeVisible) {
            isFirstVisibleBlock = false;
        }

        block = block.next();
    }

    if (isTextChanged) {
        cursor.endEditBlock();
    }
}

void SimpleTextCorrector::Implementation::updateBlocksHeight(int _position, int _charsChanged)
{
    //
    // Пробегаем документ и настраиваем  высоту строк, в зависимости от используемых шрифтов в них
    //

    const auto& currentTemplate = TemplatesFacade::simpleTextTemplate(q->templateId());
    TextCursor cursor(document());
    cursor.setPosition(std::max(0, _position));
    bool isTextChanged = false;

    const int endPosition
        = _charsChanged > 0 ? cursor.position() + _charsChanged : document()->characterCount();
    auto block = cursor.block();
    while (block.isValid() && block.position() < endPosition) {
        const auto blockStyle = currentTemplate.paragraphStyle(block);

        //
        // В некоторых случаях, мы попадаем сюда, когда документ не до конца настроен, поэтому
        // когда обнаруживается такая ситация, завершаем выполнение
        //
        if (blockStyle.type() == TextParagraphType::Undefined) {
            break;
        }

        //
        // Корректируем высоту строк
        //
        if (const auto formats = block.textFormats(); !formats.isEmpty()) {
            //
            // ... определяем высоту максимально высокой строки
            //
            qreal lineHeight = 0.0;
            if (!formats.isEmpty()) {
                for (const auto& format : formats) {
                    lineHeight
                        = std::max(lineHeight, TextHelper::fineLineSpacing(format.format.font()));
                }
            } else {
                lineHeight = TextHelper::fineLineSpacing(cursor.blockCharFormat().font());
            }
            //
            // ... учитываем межстрочный интервал стиля блока
            //
            switch (blockStyle.lineSpacingType()) {
            case TextBlockStyle::LineSpacingType::FixedLineSpacing: {
                lineHeight = MeasurementHelper::mmToPx(blockStyle.lineSpacingValue());
                break;
            }
            case TextBlockStyle::LineSpacingType::DoubleLineSpacing: {
                lineHeight *= 2.0;
                break;
            }
            case TextBlockStyle::LineSpacingType::OneAndHalfLineSpacing: {
                lineHeight *= 1.5;
                break;
            }
            case TextBlockStyle::LineSpacingType::SingleLineSpacing:
            default: {
                break;
            }
            }
            //
            // ... если высота линии изменилась, то обновим
            //
            if (!qFuzzyCompare(cursor.blockFormat().lineHeight(), lineHeight)) {
                if (!isTextChanged) {
                    isTextChanged = true;
                    cursor.beginEditBlock();
                }

                cursor.setPosition(block.position());
                auto blockFormat = cursor.blockFormat();
                blockFormat.setLineHeight(lineHeight, QTextBlockFormat::FixedHeight);
                cursor.setBlockFormat(blockFormat);
            }
        }

        block = block.next();
    }

    if (isTextChanged) {
        cursor.endEditBlock();
    }
}

void SimpleTextCorrector::Implementation::correctPageBreaks(int _position)
{
    //
    // Определим высоту страницы
    //
    const QTextFrameFormat rootFrameFormat = document()->rootFrame()->frameFormat();
    //
    const qreal pageWidth = document()->pageSize().width() - rootFrameFormat.leftMargin()
        - rootFrameFormat.rightMargin();
    if (pageWidth < 0) {
        return;
    }
    qreal leftHalfWidth = 0.0;
    qreal rightHalfWidth = 0.0;
    {
        const auto& currentTemplate = TemplatesFacade::simpleTextTemplate(q->templateId());
        leftHalfWidth = pageWidth * currentTemplate.leftHalfOfPageWidthPercents() / 100.0
            - currentTemplate.pageSplitterWidth() / 2;
        rightHalfWidth = pageWidth - leftHalfWidth - currentTemplate.pageSplitterWidth();
    }
    auto currentBlockWidth = [this, pageWidth, leftHalfWidth, rightHalfWidth] {
        if (!currentBlockInfo.inTable) {
            return pageWidth;
        }

        if (currentBlockInfo.inFirstColumn) {
            return leftHalfWidth;
        }

        return rightHalfWidth;
    };
    //
    const qreal pageHeight = document()->pageSize().height() - rootFrameFormat.topMargin()
        - rootFrameFormat.bottomMargin();
    if (pageHeight < 0) {
        return;
    }
    //
    // Если размер изменился, необходимо пересчитать все блоки
    //
    if (!qFuzzyCompare(lastDocumentSize.width(), pageWidth)
        || !qFuzzyCompare(lastDocumentSize.height(), pageHeight)) {
        blockItems.clear();
        lastDocumentSize = QSizeF(pageWidth, pageHeight);
    }

    //
    // При необходимости скорректируем размер модели параметров блоков для дальнейшего использования
    //
    {
        //
        // ... для сравнения, используем минимальный запас в 10 процентов
        //
        const int blocksCount = document()->blockCount() * 1.1;
        if (blockItems.size() <= blocksCount) {
            blockItems.resize(blocksCount * 2);
        }
    }

    //
    // Определим список блоков для принудительной ручной проверки для случая, когда пользователь
    // редактирует текст на границе страниц в разорванном блоке. Это необходимо для того,
    // чтобы корректно обрабатывать изменение текста в предыдущих и следующих за переносом блоках
    //
    if (_position != -1) {
        auto block = document()->findBlock(_position);
        if (block.blockFormat().boolProperty(TextBlockStyle::PropertyIsBreakCorrectionStart)
            || block.blockFormat().boolProperty(TextBlockStyle::PropertyIsBreakCorrectionEnd)) {
            //
            // ... два вперёд и два назад, включая текущий получается 5
            //
            auto replies = 3;
            for (; replies < 5; ++replies) {
                if (!block.next().isValid()) {
                    break;
                }
                block = block.next();
            }
            //
            // ... собственно сбрасываем рассчитанные параметры блоков для перепроверки
            //
            do {
                blockItems[block.blockNumber()] = {};
                block = block.previous();
            } while (block.isValid()
                     && (replies-- > 0
                         || block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
                         || block.blockFormat().boolProperty(
                             TextBlockStyle::PropertyIsBreakCorrectionStart)
                         || block.blockFormat().boolProperty(
                             TextBlockStyle::PropertyIsBreakCorrectionEnd)));
        }
    }

    //
    // Начинаем работу с документом
    //
    TextCursor cursor(document());
    cursor.beginEditBlock();

    //
    // Идём по каждому блоку документа с самого начала
    //
    QTextBlock block = document()->begin();
    //
    // ... значение нижней позиции последнего блока относительно начала страницы
    //
    qreal lastBlockHeight = 0.0;
    currentBlockInfo = {};
    bool isFirstChangedBlock = true;
    while (block.isValid()) {
        //
        // Пропускаем невидимые блоки
        //
        if (!block.isVisible()) {
            blockItems[currentBlockInfo.number] = {};

            block = block.next();
            ++currentBlockInfo.number;
            continue;
        }

        //
        // Запомним самый нижний блок, когда находимся в таблице
        //
        if (currentBlockInfo.inTable) {
            currentBlockInfo.tableBottom = std::max(currentBlockInfo.tableBottom, lastBlockHeight);
        }

        const auto blockType = TextBlockStyle::forBlock(block);

        //
        // Если вошли в таблицу, или вышли из неё
        //
        if (blockType == TextParagraphType::PageSplitter) {
            if (!currentBlockInfo.inTable) {
                currentBlockInfo.inTable = true;
                currentBlockInfo.inFirstColumn = true;
                currentBlockInfo.tableTop = lastBlockHeight;
            }
            //
            // При выходе из таблицы восстанавливаем значение высоты последнего блока
            // и очищаем параметры относящиеся к таблице
            //
            else {
                lastBlockHeight = currentBlockInfo.tableBottom;
                currentBlockInfo.inTable = false;
                currentBlockInfo.inFirstColumn = false;
                currentBlockInfo.tableTop = std::numeric_limits<qreal>::min();
                currentBlockInfo.tableBottom = std::numeric_limits<qreal>::min();
            }

            blockItems[currentBlockInfo.number] = {};

            block = block.next();
            ++currentBlockInfo.number;
            continue;
        }

        //
        // Если в первой колонке внутри таблицы
        //
        if (currentBlockInfo.inTable && currentBlockInfo.inFirstColumn) {
            //
            // Когда перешли во вторую колонку, восстановим высоту последнего блока и запомним
            // смещение
            //
            cursor.setPosition(block.position());
            if (!cursor.inFirstColumn()) {
                currentBlockInfo.inFirstColumn = false;
                lastBlockHeight = currentBlockInfo.tableTop;
            }
        }

        //
        // Для блока, который всегда находится в начале страницы очищаем информацию
        // о высоте предыдущего блока, какой бы она ни была
        //
        if (block.blockFormat().pageBreakPolicy() == QTextFormat::PageBreak_AlwaysBefore) {
            lastBlockHeight = 0;
        }

        //
        // Определить высоту текущего блока
        //
        const QTextBlockFormat blockFormat = block.blockFormat();
        const qreal blockLineHeight = blockFormat.lineHeight();
        const int blockLineCount = block.layout()->lineCount();
        //
        // ... если блок первый на странице, то для него не нужно учитывать верхний отступ
        //
        const qreal blockHeight = qFuzzyCompare(lastBlockHeight, 0.0)
            ? blockLineHeight * blockLineCount + blockFormat.bottomMargin()
            : blockLineHeight * blockLineCount + blockFormat.topMargin()
                + blockFormat.bottomMargin();
        //
        // ... и высоту одной строки следующего
        //
        qreal nextBlockOneLineHeight = 0;
        if (block.next().isValid()) {
            const qreal nextBlockLineHeight = block.next().blockFormat().lineHeight();
            const QTextBlockFormat nextBlockFormat = block.next().blockFormat();
            nextBlockOneLineHeight = nextBlockLineHeight + nextBlockFormat.topMargin();
        }


        //
        // Определим, заканчивается ли блок на последней строке страницы
        //
        const bool atPageEnd =
            // сам блок влезает
            (lastBlockHeight + blockHeight <= pageHeight)
            // и
            && (
                // добавление одной строки перекинет его уже на новую страницу
                lastBlockHeight + blockHeight + blockLineHeight > pageHeight
                // или 1 строка следующего блока уже не влезет на эту страницу
                || lastBlockHeight + blockHeight + nextBlockOneLineHeight > pageHeight);
        //
        // ... или, может попадает на разрыв
        //
        const bool atPageBreak =
            // сам блок не влезает
            (lastBlockHeight + blockHeight > pageHeight)
            // но влезает хотя бы одна строка
            && (lastBlockHeight + blockFormat.topMargin() + blockLineHeight < pageHeight);

        //
        // Проверяем, изменилась ли позиция блока,
        // и что текущий блок это не изменённый блок под курсором
        // и что текущий блок это не пустая декорация в начале страницы
        //
        const bool isBlockEmptyDecorationOnTopOfThePage
            = blockFormat.boolProperty(TextBlockStyle::PropertyIsCorrection)
            && block.text().isEmpty()
            && qFuzzyCompare(blockItems[currentBlockInfo.number].top, 0.0);
        if (blockItems[currentBlockInfo.number].isValid() && !isBlockEmptyDecorationOnTopOfThePage
            && qFuzzyCompare(blockItems[currentBlockInfo.number].height, blockHeight)
            && qFuzzyCompare(blockItems[currentBlockInfo.number].top, lastBlockHeight)
            && blockItems[currentBlockInfo.number].type == blockType) {
            //
            // Если не изменилась
            //
            // ... в данном случае блок не может быть на разрыве
            //
            Q_ASSERT_X(atPageBreak == false, Q_FUNC_INFO,
                       "Normally cached blocks can't be placed on page breaks");
            //
            // ... то корректируем позицию
            //
            if (atPageEnd) {
                lastBlockHeight = 0;
            } else if (atPageBreak) {
                lastBlockHeight += blockHeight - pageHeight;
            } else {
                lastBlockHeight += blockHeight;
            }
            //
            // ... и переходим к следующему блоку
            //
            block = block.next();
            ++currentBlockInfo.number;
            continue;
        }


        //
        // Если позиция блока изменилась, то работаем по алгоритму корректировки текста
        //


        //
        // Если это первый из блоков, в котором сменилось расположение или высота, проверяем также
        // ближайшие блоки, чтобы корректно обработать ситуацию, когда в блоке удалили текст
        // и теперь он может быть помещён на предыдущей странице
        //
        if (isFirstChangedBlock && blockItems[currentBlockInfo.number].isValid()) {
            isFirstChangedBlock = false;
            int topIndex = currentBlockInfo.number;
            const auto maxDecorationBlocks = 2;
            for (int i = 0; i < maxDecorationBlocks; ++i) {
                if (topIndex == 0) {
                    lastBlockHeight = 0;
                    break;
                }

                //
                // Контролируем, чтобы расположение блоков не переходило через границу таблицы
                //
                if (TextBlockStyle::forBlock(block.previous()) == TextParagraphType::PageSplitter) {
                    break;
                }

                block = block.previous();
                --topIndex;
                lastBlockHeight = blockItems[topIndex].top;
            }
            currentBlockInfo.number = topIndex;

            const int bottomIndex = currentBlockInfo.number + maxDecorationBlocks;
            for (int index = topIndex; index <= bottomIndex; ++index) {
                if (index >= blockItems.size()) {
                    break;
                }

                blockItems[index] = {};
            }

            continue;
        }


        //
        // Работаем с декорациями
        //
        // ... если блок декорация, то удаляем его
        //
        if (blockFormat.boolProperty(TextBlockStyle::PropertyIsCorrection)) {
            blockItems[currentBlockInfo.number] = {};
            cursor.setPosition(block.position());
            if (cursor.block().next() != cursor.document()->end()) {
                cursor.movePosition(TextCursor::EndOfBlock, TextCursor::KeepAnchor);
                cursor.movePosition(TextCursor::NextCharacter, TextCursor::KeepAnchor);
            } else {
                cursor.movePosition(TextCursor::PreviousCharacter);
                cursor.movePosition(TextCursor::NextBlock, TextCursor::KeepAnchor);
                cursor.movePosition(TextCursor::EndOfBlock, TextCursor::KeepAnchor);
            }
            if (cursor.hasSelection()) {
                cursor.deleteChar();
                //
                // ... если текст в получившемся блоке нет, то восстанавливаем его формат, т.к. в
                //     этом случае в блоке будет сохранятся форматирование удалённого блока
                //
                if (cursor.block().text().isEmpty()) {
                    const auto blockType = TextBlockStyle::forBlock(cursor);
                    const auto& blockStyle = TemplatesFacade::simpleTextTemplate(q->templateId())
                                                 .paragraphStyle(blockType);
                    cursor.setBlockCharFormat(blockStyle.charFormat());
                }
            } else {
                QTextBlockFormat breakStartFormat = cursor.blockFormat();
                breakStartFormat.clearProperty(TextBlockStyle::PropertyIsCorrection);
                cursor.setBlockFormat(breakStartFormat);
            }
            //
            // ... и продолжим со следующего блока
            //
            block = cursor.block();
            continue;
        }
        //
        // ... если в текущем блоке есть разрыв, пробуем его вернуть
        //
        else if (blockFormat.boolProperty(TextBlockStyle::PropertyIsBreakCorrectionStart)
                 || blockFormat.boolProperty(TextBlockStyle::PropertyIsBreakCorrectionEnd)) {
            cursor.setPosition(block.position());

            //
            // Если в конце разрыва, вернёмся к началу
            //
            if (blockFormat.boolProperty(TextBlockStyle::PropertyIsBreakCorrectionEnd)) {
                //
                // Началом может быть элемент с соответствующим флагом, либо простой элемент,
                // в случае, когда было удаление переноса строки между абзацем с переносом и
                // предыдущим
                //
                do {
                    blockItems[currentBlockInfo.number] = {};
                    cursor.movePosition(TextCursor::PreviousBlock);
                    --currentBlockInfo.number;
                    //
                    // ... восстанавливаем последнюю высоту от предыдущего элемента
                    //
                    lastBlockHeight = blockItems[currentBlockInfo.number].top;
                } while (cursor.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
                         && !cursor.blockFormat().boolProperty(
                             TextBlockStyle::PropertyIsBreakCorrectionStart));
            }

            //
            // Начинаем склеивать разрыв
            //
            cursor.movePosition(TextCursor::EndOfBlock);
            do {
                cursor.movePosition(TextCursor::NextBlock, TextCursor::KeepAnchor);
            } while (cursor.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection));
            //
            // ... если дошли до конца разрыва, то сшиваем его
            //
            if (cursor.blockFormat().boolProperty(TextBlockStyle::PropertyIsBreakCorrectionEnd)) {
                cursor.insertText(" ");
            }
            //
            // ... а если после начала разрыва идёт другой блок
            //
            else {
                cursor.clearSelection();
                if (cursor.movePosition(QTextCursor::NextBlock)) {
                    //
                    // ... если последующий блок корректировка или конец разрыва, то это кейс,
                    //     когда был нажат энтер в блоке начала разрыва и нужно перенести флаг
                    //     разрыва в следующий за текущим блок
                    //
                    if (cursor.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
                        || cursor.blockFormat().boolProperty(
                            TextBlockStyle::PropertyIsBreakCorrectionEnd)) {
                        cursor.movePosition(QTextCursor::PreviousBlock);
                        QTextBlockFormat breakStartFormat = cursor.blockFormat();
                        breakStartFormat.setProperty(TextBlockStyle::PropertyIsBreakCorrectionStart,
                                                     true);
                        cursor.setBlockFormat(breakStartFormat);
                    }
                    //
                    // ... в противном случае это был кейс объединения двух блоков и нужно просто
                    // убрать флаги разрыва
                    //
                    else {
                        cursor.movePosition(QTextCursor::PreviousBlock);
                    }

                    cursor.movePosition(TextCursor::PreviousBlock);
                }

                Q_ASSERT(cursor.blockFormat().boolProperty(
                    TextBlockStyle::PropertyIsBreakCorrectionStart));
            }
            //
            // ... очищаем значения обрывов
            //
            QTextBlockFormat cleanFormat = blockFormat;
            cleanFormat.clearProperty(PageTextEdit::PropertyDontShowCursor);
            cleanFormat.clearProperty(TextBlockStyle::PropertyIsBreakCorrectionStart);
            cleanFormat.clearProperty(TextBlockStyle::PropertyIsBreakCorrectionEnd);
            cursor.setBlockFormat(cleanFormat);
            //
            // ... и проработаем текущий блок с начала
            //
            block = cursor.block();
            updateBlockLayout(currentBlockWidth(), block);
            continue;
        }


        //
        // Работаем с переносами
        //
        if (needToCorrectPageBreaks && (atPageEnd || atPageBreak)) {
            switch (blockType) {
            //
            // Если это время и место, бит, кадр или начало папки
            //
            case TextParagraphType::ChapterHeading1:
            case TextParagraphType::ChapterHeading2:
            case TextParagraphType::ChapterHeading3:
            case TextParagraphType::ChapterHeading4:
            case TextParagraphType::ChapterHeading5:
            case TextParagraphType::ChapterHeading6: {
                //
                // Переносим на следующую страницу
                //
                moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight,
                                           currentBlockWidth(), cursor, block, lastBlockHeight);

                break;
            }

            //
            // Конец папки распологаем либо только в конце страницы, либо целиком переносим на
            // следующую страницу
            //
            case TextParagraphType::SequenceFooter: {
                //
                // Если в конце страницы, оставляем как есть
                //
                if (atPageEnd) {
                    //
                    // Запоминаем параметры текущего блока
                    //
                    blockItems[currentBlockInfo.number++]
                        = BlockInfo{ blockHeight, lastBlockHeight, blockType };
                    //
                    // и идём дальше
                    //
                    lastBlockHeight = 0;
                }
                //
                // В противном случае, просто переносим блок на следующую страницу
                //
                else {
                    moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, pageWidth,
                                               cursor, block, lastBlockHeight);
                }

                break;
            }

            //
            // Для остальных блоков не делаем никакой работы
            //
            default: {
                break;
            }
            }
        }
        //
        // Если блок находится посередине страницы, просто переходим к следующему
        //
        else {
            //
            // Запоминаем параметры текущего блока
            //
            blockItems[currentBlockInfo.number++]
                = BlockInfo{ blockHeight, lastBlockHeight, blockType };
            //
            // и идём дальше
            //
            lastBlockHeight += blockHeight;
        }

        block = block.next();
    }

    cursor.endEditBlock();
}

void SimpleTextCorrector::Implementation::moveCurrentBlockWithThreePreviousToNextPage(
    const QTextBlock& _prePrePreviousBlock, const QTextBlock& _prePreviousBlock,
    const QTextBlock& _previousBlock, qreal _pageHeight, qreal _pageWidth, TextCursor& _cursor,
    QTextBlock& _block, qreal& _lastBlockHeight)
{
    --currentBlockInfo.number;
    --currentBlockInfo.number;
    --currentBlockInfo.number;

    const QTextBlockFormat previousBlockFormat = _previousBlock.blockFormat();
    const qreal previousBlockHeight
        = previousBlockFormat.lineHeight() * _previousBlock.layout()->lineCount()
        + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
    const QTextBlockFormat prePreviousBlockFormat = _prePreviousBlock.blockFormat();
    const qreal prePreviousBlockHeight
        = prePreviousBlockFormat.lineHeight() * _prePreviousBlock.layout()->lineCount()
        + prePreviousBlockFormat.topMargin() + prePreviousBlockFormat.bottomMargin();
    const QTextBlockFormat prePrePreviousBlockFormat = _prePrePreviousBlock.blockFormat();
    const qreal prePrePreviousBlockHeight
        = prePrePreviousBlockFormat.lineHeight() * _prePrePreviousBlock.layout()->lineCount()
        + prePrePreviousBlockFormat.topMargin() + prePrePreviousBlockFormat.bottomMargin();
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight + previousBlockHeight
        + prePreviousBlockHeight + prePrePreviousBlockHeight;
    moveBlockToNextPage(_prePrePreviousBlock, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
    _block = _cursor.block();
    //
    // Запоминаем параметры блока время и места
    //
    blockItems[currentBlockInfo.number++]
        = BlockInfo{ prePreviousBlockHeight - prePreviousBlockFormat.topMargin(), 0,
                     TextBlockStyle::forBlock(_block) };
    //
    // Обозначаем последнюю высоту, как высоту предпредпредыдущего блока
    //
    _lastBlockHeight = prePrePreviousBlockHeight - prePrePreviousBlockFormat.topMargin();
    //
    // Текущий блок будет обработан, как очередной блок посередине страницы при следующем проходе
    //
}

void SimpleTextCorrector::Implementation::moveCurrentBlockWithTwoPreviousToNextPage(
    const QTextBlock& _prePreviousBlock, const QTextBlock& _previousBlock, qreal _pageHeight,
    qreal _pageWidth, TextCursor& _cursor, QTextBlock& _block, qreal& _lastBlockHeight)
{
    --currentBlockInfo.number;
    --currentBlockInfo.number;

    const QTextBlockFormat previousBlockFormat = _previousBlock.blockFormat();
    const qreal previousBlockHeight
        = previousBlockFormat.lineHeight() * _previousBlock.layout()->lineCount()
        + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
    const QTextBlockFormat prePreviousBlockFormat = _prePreviousBlock.blockFormat();
    const qreal prePreviousBlockHeight
        = prePreviousBlockFormat.lineHeight() * _prePreviousBlock.layout()->lineCount()
        + prePreviousBlockFormat.topMargin() + prePreviousBlockFormat.bottomMargin();
    const qreal sizeToPageEnd
        = _pageHeight - _lastBlockHeight + previousBlockHeight + prePreviousBlockHeight;
    moveBlockToNextPage(_prePreviousBlock, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
    _block = _cursor.block();
    //
    // Запоминаем параметры блока время и места
    //
    blockItems[currentBlockInfo.number++]
        = BlockInfo{ prePreviousBlockHeight - prePreviousBlockFormat.topMargin(), 0,
                     TextBlockStyle::forBlock(_block) };
    //
    // Обозначаем последнюю высоту, как высоту предпредыдущего блока
    //
    _lastBlockHeight = prePreviousBlockHeight - prePreviousBlockFormat.topMargin();
    //
    // Текущий блок будет обработан, как очередной блок посередине страницы при следующем проходе
    //
}

void SimpleTextCorrector::Implementation::moveCurrentBlockWithPreviousToNextPage(
    const QTextBlock& _previousBlock, qreal _pageHeight, qreal _pageWidth, TextCursor& _cursor,
    QTextBlock& _block, qreal& _lastBlockHeight)
{
    --currentBlockInfo.number;

    const QTextBlockFormat previousBlockFormat = _previousBlock.blockFormat();
    const qreal previousBlockHeight
        = previousBlockFormat.lineHeight() * _previousBlock.layout()->lineCount()
        + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight + previousBlockHeight;
    moveBlockToNextPage(_previousBlock, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
    _block = _cursor.block();
    //
    // Запоминаем параметры предыдущего блока
    //
    blockItems[currentBlockInfo.number++]
        = BlockInfo{ previousBlockHeight - previousBlockFormat.topMargin(), 0,
                     TextBlockStyle::forBlock(_block) };
    //
    // Обозначаем последнюю высоту, как высоту предыдущего блока
    //
    _lastBlockHeight = previousBlockHeight - previousBlockFormat.topMargin();
    //
    // Текущий блок будет обработан, как очередной блок посередине страницы при следующем проходе
    //
}

void SimpleTextCorrector::Implementation::moveCurrentBlockToNextPage(
    const QTextBlockFormat& _blockFormat, qreal _blockHeight, qreal _pageHeight, qreal _pageWidth,
    TextCursor& _cursor, QTextBlock& _block, qreal& _lastBlockHeight)
{
    if (_blockHeight < _pageHeight) {
        const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight;
        moveBlockToNextPage(_block, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
        _block = _cursor.block();
    }

    //
    // Запоминаем параметры текущего блока
    //
    blockItems[currentBlockInfo.number++]
        = BlockInfo{ _blockHeight - _blockFormat.topMargin(), 0, TextBlockStyle::forBlock(_block) };

    if (_blockHeight < _pageHeight) {
        //
        // Обозначаем последнюю высоту, как высоту текущего блока
        //
        _lastBlockHeight = _blockHeight - _blockFormat.topMargin();
    } else {
        _lastBlockHeight = _blockHeight;
        while (_lastBlockHeight > _pageHeight) {
            _lastBlockHeight -= _pageHeight;
        }
    }
}

QTextBlock SimpleTextCorrector::Implementation::findPreviousBlock(const QTextBlock& _block)
{
    QTextBlock previousBlock = _block.previous();
    while (previousBlock.isValid() && !previousBlock.isVisible()) {
        --currentBlockInfo.number;
        previousBlock = previousBlock.previous();
    }
    return previousBlock;
}

QTextBlock SimpleTextCorrector::Implementation::findNextBlock(const QTextBlock& _block)
{
    QTextBlock nextBlock = _block.next();
    while (nextBlock.isValid() && !nextBlock.isVisible()
           && nextBlock.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)) {
        nextBlock = nextBlock.next();
    }
    return nextBlock;
}

void SimpleTextCorrector::Implementation::moveBlockToNextPage(const QTextBlock& _block,
                                                              qreal _spaceToPageEnd,
                                                              qreal _pageHeight, qreal _pageWidth,
                                                              TextCursor& _cursor)
{
    //
    // Смещаем курсор в начало блока
    //
    _cursor.setPosition(_block.position());
    //
    // Определим сколько пустых блоков нужно вставить
    //
    const QTextBlockFormat format = _block.blockFormat();
    const qreal blockHeight = format.lineHeight() + format.topMargin() + format.bottomMargin();
    const int insertBlockCount = _spaceToPageEnd / blockHeight;
    //
    // Определим формат блоков декорации
    //
    QTextBlockFormat decorationFormat = format;
    auto paragraphType = TextBlockStyle::forBlock(_block);
    switch (paragraphType) {
    case TextParagraphType::SceneHeading: {
        paragraphType = TextParagraphType::SceneHeadingShadow;
        break;
    }
    case TextParagraphType::BeatHeading: {
        paragraphType = TextParagraphType::BeatHeadingShadow;
        break;
    }
    case TextParagraphType::ActHeading:
    case TextParagraphType::ActFooter:
    case TextParagraphType::SequenceHeading:
    case TextParagraphType::SequenceFooter: {
        paragraphType = TextParagraphType::Action;
        break;
    }

    default: {
        break;
    }
    }
    decorationFormat.setProperty(TextBlockStyle::PropertyType, static_cast<int>(paragraphType));
    //
    decorationFormat.setProperty(TextBlockStyle::PropertyIsCorrection, true);
    decorationFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
    //
    // Вставляем блоки декорации
    //
    for (int blockIndex = 0; blockIndex < insertBlockCount; ++blockIndex) {
        //
        // Декорируем
        //
        insertBlock(_pageWidth, _cursor);
        _cursor.movePosition(TextCursor::PreviousBlock);
        _cursor.setBlockFormat(decorationFormat);
        //
        // Сохраним данные блока, чтобы перенести их к реальному владельцу
        //
        TextBlockData* blockData = nullptr;
        auto block = _cursor.block();
        if (block.userData() != nullptr) {
            blockData = new TextBlockData(static_cast<TextBlockData*>(block.userData()));
            block.setUserData(nullptr);
        }
        //
        // Запоминаем параметры текущего блока
        //
        blockItems[currentBlockInfo.number++]
            = BlockInfo{ blockHeight, _pageHeight - _spaceToPageEnd + blockIndex * blockHeight,
                         TextBlockStyle::forBlock(block) };
        //
        // Переведём курсор на блок после декорации
        //
        _cursor.movePosition(TextCursor::NextBlock);
        if (blockData != nullptr) {
            _cursor.block().setUserData(blockData);
        }
    }
}


// ****


SimpleTextCorrector::SimpleTextCorrector(QTextDocument* _document)
    : AbstractTextCorrector(_document)
    , d(new Implementation(this))
{
    Q_ASSERT_X(_document, Q_FUNC_INFO, "Document couldn't be a nullptr");
}

SimpleTextCorrector::~SimpleTextCorrector() = default;

void SimpleTextCorrector::setCorrectionOptions(const QStringList& _options)
{
    const auto needToCorrectPageBreaks = _options.contains("correct-page-breaks");
    if (d->needToCorrectPageBreaks == needToCorrectPageBreaks) {
        return;
    }

    d->needToCorrectPageBreaks = needToCorrectPageBreaks;

    clear();
    makeCorrections();
}

void SimpleTextCorrector::clearImpl()
{
    d->lastDocumentSize = QSizeF();
    d->currentBlockInfo.number = 0;
    d->blockItems.clear();
}

void SimpleTextCorrector::makeCorrections(int _position, int _charsChanged)
{
    Q_UNUSED(_charsChanged)

    //
    // Избегаем рекурсии, которая может возникать от того, что корректировка происходит
    // при изменении текста документа, но в то же время корректор сам делает изменения в документе
    //
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    //
    // Сначала корректируем видимость блоков
    //
    makeSoftCorrections(_position, _charsChanged);

    //
    // Затем при необходимости корректируем сам текст
    //

    if (d->needToCorrectPageBreaks) {
        d->correctPageBreaks(_position);
    }
}

void SimpleTextCorrector::makeSoftCorrections(int _position, int _charsChanged)
{
    d->updateBlocksVisibility(_position);
    d->updateBlocksHeight(_position, _charsChanged);
}

} // namespace BusinessLayer
