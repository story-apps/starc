#include "screenplay_text_corrector.h"

#include "screenplay_text_block_data.h"
#include "screenplay_text_cursor.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <ui/widgets/text_edit/page/page_text_edit.h>

#include <utils/tools/run_once.h>

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QTextBlock>
#include <QTextDocument>
#include <QRegularExpression>

#include <cmath>

using BusinessLayer::CharacterParser;
using BusinessLayer::ScreenplayBlockStyle;
using BusinessLayer::ScreenplayParagraphType;
using BusinessLayer::ScreenplayTemplateFacade;


namespace BusinessLayer
{

namespace {
    /**
     * @brief Список символов пунктуации, разделяющие предложения
     */
    const QRegularExpression kPunctuationCharacter("([.]|[!]|[?]|[:]|[;]|[…])");

    /**
     * @brief Автоматически добавляемые продолжения в диалогах
     */
    //: Continued
    static const char* kContinuedTerm = QT_TRANSLATE_NOOP("BusinessLogic::ScriptTextCorrector", "CONT'D");

    /**
     * @brief Автоматически добавляемые продолжения в диалогах
     */
    static const char* MORE = QT_TRANSLATE_NOOP("BusinessLogic::ScriptTextCorrector", "MORE");
    static const QString moreTerm() {
        return QString("(%1)").arg(QApplication::translate("BusinessLogic::ScriptTextCorrector", MORE));
    }

    /**
     * @brief Обновить компановку текста для блока
     */
    void updateBlockLayout(qreal _pageWidth, const QTextBlock& _block) {
        _block.layout()->setText(_block.text());
        _block.layout()->beginLayout();
        forever {
            QTextLine line = _block.layout()->createLine();
            if (!line.isValid()) {
                break;
            }

            line.setLineWidth(_pageWidth
                              - _block.blockFormat().leftMargin()
                              - _block.blockFormat().rightMargin());
        }
        _block.layout()->endLayout();
    }

    /**
     * @brief Вставить блок, обновив при этом лэйаут вставленного блока
     */
    void insertBlock(int _pageWidth, QTextCursor& _cursor) {
        _cursor.insertBlock();
        updateBlockLayout(_pageWidth, _cursor.block());
    }
}


class ScreenplayTextCorrector::Implementation
{
public:
    explicit Implementation(QTextDocument* _document);

    /**
     * @brief Скорректировать имена персонажей
     */
    void correctCharactersNames(int _position = -1, int _charsChanged = 0);

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
        const QTextBlock& _prePreviousBlock, const QTextBlock& _previousBlock, qreal _pageHeight,
        qreal _pageWidth, ScreenplayTextCursor& _cursor, QTextBlock& _block, qreal& _lastBlockHeight);

    /**
     * @brief Сместить текущий блок вместе с двумя предыдущими на следующую страницу
     */
    void moveCurrentBlockWithTwoPreviousToNextPage(const QTextBlock& _prePreviousBlock,
        const QTextBlock& _previousBlock, qreal _pageHeight, qreal _pageWidth,
        ScreenplayTextCursor& _cursor, QTextBlock& _block, qreal& _lastBlockHeight);

    /**
     * @brief Сместить текущий блок вместе с предыдущим на следующую страницу
     */
    void moveCurrentBlockWithPreviousToNextPage(const QTextBlock& _previousBlock,
        qreal _pageHeight, qreal _pageWidth, ScreenplayTextCursor& _cursor, QTextBlock& _block,
        qreal& _lastBlockHeight);

    /**
     * @brief Сместить текущий блок на следующую страницу
     */
    void moveCurrentBlockToNextPage(const QTextBlockFormat& _blockFormat, qreal _blockHeight,
        qreal _pageHeight, qreal _pageWidth, ScreenplayTextCursor& _cursor, QTextBlock& _block,
        qreal& _lastBlockHeight);

    /**
     * @brief Разорвать блок диалога
     */
    void breakDialogue(const QTextBlockFormat& _blockFormat, qreal _blockHeight,
        qreal _pageHeight, qreal _pageWidth, ScreenplayTextCursor& _cursor, QTextBlock& _block,
        qreal& _lastBlockHeight);

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
        qreal _pageWidth, ScreenplayTextCursor& _cursor);

    //
    // Данные
    //

    /**
     * @brief Документ который будем корректировать
     */
    QTextDocument* document = nullptr;

    /**
     * @brief Шаблон оформления сценария
     */
    QString templateId;

    /**
     * @brief Необходимо ли корректировать текст блоков имён персонажей
     */
    bool needToCorrectCharactersNames = true;

    /**
     * @brief Необходимо ли корректировать текст на разрывах страниц
     */
    bool needToCorrectPageBreaks = true;

    /**
     * @brief Запланированная корректировка
     */
    struct {
        bool isValid = false;
        int position = 0;
        int changed = 0;
        int end() const { return position + changed; }
    } plannedCorrection;

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
        explicit BlockInfo(qreal _height, qreal _top) :
            height(_height),
            top(_top)
        {}

        /**
         * @brief Валиден ли блок
         */
        bool isValid() const {
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
    };

    /**
     * @brief Модель параметров блоков
     */
    QVector<BlockInfo> blockItems;
};

ScreenplayTextCorrector::Implementation::Implementation(QTextDocument* _document)
    : document(_document)
{
}

void ScreenplayTextCorrector::Implementation::correctCharactersNames(int _position, int _charsChanged)
{
    //
    // Определим границы работы алгоритма
    //
    int startPosition = _position;
    int endPosition = _position + _charsChanged;
    if (startPosition == -1) {
        startPosition = 0;
        endPosition = document->characterCount();
    }

    //
    // Начинаем работу с документом
    //
    ScreenplayTextCursor cursor(document);
    cursor.beginEditBlock();

    //
    // Расширим выделение
    //
    // ... от начала сцены
    //
    QVector<ScreenplayParagraphType> sceneBorders = { ScreenplayParagraphType::SceneHeading,
                                                      ScreenplayParagraphType::FolderHeader,
                                                      ScreenplayParagraphType::FolderFooter };
    QTextBlock block = document->findBlock(startPosition);
    while (block != document->begin()) {
        const auto blockType = ScreenplayBlockStyle::forBlock(block);
        if (sceneBorders.contains(blockType)) {
            break;
        }

        block = block.previous();
    }
    //
    // ... и до конца
    //
    {
        QTextBlock endBlock = document->findBlock(endPosition);
        while (endBlock.isValid()
               && endBlock != document->end()) {
            const ScreenplayParagraphType blockType = ScreenplayBlockStyle::forBlock(endBlock);
            if (sceneBorders.contains(blockType)) {
                break;
            }

            endBlock = endBlock.next();
        }
        endPosition = endBlock.previous().position();
    }

    //
    // Корректируем имена пресонажей в изменённой части документа
    //
    QString lastCharacterName;
    do {
        const auto blockType = ScreenplayBlockStyle::forBlock(block);
        //
        // Если дошли до новой сцены, очищаем последнее найдённое имя персонажа
        //
        if (sceneBorders.contains(blockType)) {
            lastCharacterName.clear();
        }
        //
        // Корректируем имя персонажа при необходимости
        //
        else if (blockType == ScreenplayParagraphType::Character) {
            const QString characterName = CharacterParser::name(block.text());
            const bool isStartPositionInBlock =
                    block.position() < startPosition
                    && block.position() + block.length() > startPosition;
            //
            // Если имя текущего персонажа не пусто и курсор не находится в редактируемом блоке
            //
            if (!characterName.isEmpty() && !isStartPositionInBlock) {
                //
                // Не второе подряд появление, удаляем из него вспомогательный текст, если есть
                //
                if (lastCharacterName.isEmpty()
                    || characterName != lastCharacterName) {
                    QTextBlockFormat characterFormat = block.blockFormat();
                    if (characterFormat.boolProperty(ScreenplayBlockStyle::PropertyIsCharacterContinued)) {
                        characterFormat.setProperty(ScreenplayBlockStyle::PropertyIsCharacterContinued, false);
                        cursor.setPosition(block.position());
                        cursor.setBlockFormat(characterFormat);
                    }
                }
                //
                // Если второе подряд и ещё не настроено, добавляем вспомогательный текст
                //
                else if (characterName == lastCharacterName){
                    const QString characterState = CharacterParser::extension(block.text());
                    QTextBlockFormat characterFormat = block.blockFormat();
                    if (characterState.isEmpty()
                        && !characterFormat.boolProperty(ScreenplayBlockStyle::PropertyIsCharacterContinued)) {
                        characterFormat.setProperty(ScreenplayBlockStyle::PropertyIsCharacterContinued, true);
                        cursor.setPosition(block.position());
                        cursor.setBlockFormat(characterFormat);
                    }
                }

                lastCharacterName = characterName;
            }
        }

        block = block.next();
    } while (block.isValid()
             && block.position() < endPosition);

    cursor.endEditBlock();
}

void ScreenplayTextCorrector::Implementation::correctPageBreaks(int _position)
{
    //
    // Определим высоту страницы
    //
    const QTextFrameFormat rootFrameFormat = document->rootFrame()->frameFormat();
    //
    const qreal pageWidth = document->pageSize().width()
                             - rootFrameFormat.leftMargin()
                             - rootFrameFormat.rightMargin();
    if (pageWidth < 0) {
        return;
    }
    qreal leftHalfWidth = 0.0;
    qreal rightHalfWidth = 0.0;
    {
        const auto currentTemplate = ScreenplayTemplateFacade::getTemplate(templateId);
        leftHalfWidth =  pageWidth
                         * currentTemplate.leftHalfOfPageWidthPercents() / 100.0
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
    const qreal pageHeight = document->pageSize().height()
                             - rootFrameFormat.topMargin()
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
        const int blocksCount = document->blockCount() * 1.1;
        if (blockItems.size() < blocksCount) {
            blockItems.resize(blocksCount * 2);
        }
    }

    //
    // Определим список блоков для принудительной ручной проверки для случая, когда пользователь
    // редактирует текст на границе страниц в разорванном блоке. Это необходимо для того,
    // чтобы корректно обрабатывать изменение текста в предыдущих и следующих за переносом блоках
    //
    if (_position != -1) {
        auto block = document->findBlock(_position);
        if (block.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart)
                || block.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionEnd)) {
            block = block.next().next();
            auto replies = 5;
            do {
                blockItems[block.blockNumber()] = {};
                block = block.previous();
            } while (block.isValid()
                     && (replies-- > 0
                         || block.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)
                         || block.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart)
                         || block.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionEnd)));
        }
    }

    //
    // Начинаем работу с документом
    //
    ScreenplayTextCursor cursor(document);
    cursor.beginEditBlock();

    //
    // Идём по каждому блоку документа с самого начала
    //
    QTextBlock block = document->begin();
    //
    // ... значение нижней позиции последнего блока относительно начала страницы
    //
    qreal lastBlockHeight = 0.0;
    currentBlockInfo = {};
    while (block.isValid()) {
        //
        // Запомним самый нижний блок, когда находимся в таблице
        //
        if (currentBlockInfo.inTable) {
            currentBlockInfo.tableBottom = std::max(currentBlockInfo.tableBottom, lastBlockHeight);
        }

        //
        // Если вошли в таблицу, или вышли из неё
        //
        if (ScreenplayBlockStyle::forBlock(block) == ScreenplayParagraphType::PageSplitter) {
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
            // Когда перешли во вторую колонку, восстановим высоту последнего блока и запомним смещение
            //
            cursor.setPosition(block.position());
            if (!cursor.inFirstColumn()) {
                currentBlockInfo.inFirstColumn = false;
                lastBlockHeight = currentBlockInfo.tableTop;
            }
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
        const qreal blockHeight =
                qFuzzyCompare(lastBlockHeight, 0.0)
                ? blockLineHeight * blockLineCount + blockFormat.bottomMargin()
                : blockLineHeight * blockLineCount + blockFormat.topMargin() + blockFormat.bottomMargin();
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
                = blockFormat.boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)
                  && block.text().isEmpty()
                  && qFuzzyCompare(blockItems[currentBlockInfo.number].top, 0.0);
        if (blockItems[currentBlockInfo.number].isValid()
            && !isBlockEmptyDecorationOnTopOfThePage
            && qFuzzyCompare(blockItems[currentBlockInfo.number].height, blockHeight)
            && qFuzzyCompare(blockItems[currentBlockInfo.number].top, lastBlockHeight)) {
            //
            // Если не изменилась
            //
            // ... в данном случае блок не может быть на разрыве
            //
            Q_ASSERT_X(atPageBreak == false, Q_FUNC_INFO, "Normally cached blocks can't be placed on page breaks");
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
        // Если в блоке сменилось расположение или высота, проверяем также ближайшие блоки,
        // чтобы корректно обработать ситуацию, когда в блоке удалили текст и теперь он может
        // быть помещён на предыдущей странице
        //
        if (blockItems[currentBlockInfo.number].isValid()) {
            const auto maxDecorationBlocks = 2;
            const int topIndex = std::max(0, currentBlockInfo.number - maxDecorationBlocks);
            if (topIndex == 0) {
                block = document->begin();
                lastBlockHeight = 0;
            } else {
                block = block.previous().previous();
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
        if (blockFormat.boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
            blockItems[currentBlockInfo.number] = {};
            cursor.setPosition(block.position());
            if (cursor.block().next() != cursor.document()->end()) {
                cursor.movePosition(ScreenplayTextCursor::EndOfBlock, ScreenplayTextCursor::KeepAnchor);
                cursor.movePosition(ScreenplayTextCursor::NextCharacter, ScreenplayTextCursor::KeepAnchor);
                cursor.deleteChar();
            } else {
                cursor.movePosition(ScreenplayTextCursor::PreviousCharacter);
                cursor.movePosition(ScreenplayTextCursor::NextBlock, ScreenplayTextCursor::KeepAnchor);
                cursor.movePosition(ScreenplayTextCursor::EndOfBlock, ScreenplayTextCursor::KeepAnchor);
                cursor.deleteChar();
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
        else if (blockFormat.boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart)
                 || blockFormat.boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionEnd)) {
            cursor.setPosition(block.position());

            //
            // Если в конце разрыва, вернёмся к началу
            //
            if (blockFormat.boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionEnd)) {
                do {
                    blockItems[currentBlockInfo.number] = {};
                    cursor.movePosition(ScreenplayTextCursor::PreviousBlock);
                    --currentBlockInfo.number;
                    //
                    // ... восстанавливаем последнюю высоту от предыдущего элемента
                    //
                    lastBlockHeight = blockItems[currentBlockInfo.number].top;
                } while (!cursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart));
            }

            //
            // Начинаем склеивать разрыв
            //
            cursor.movePosition(ScreenplayTextCursor::EndOfBlock);
            do {
                cursor.movePosition(ScreenplayTextCursor::NextBlock, ScreenplayTextCursor::KeepAnchor);
            } while (cursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection));
            //
            // ... если дошли до конца разрыва, то сшиваем его
            //
            if (cursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionEnd)) {
                cursor.insertText(" ");
            }
            //
            // ... а если после начала разрыва идёт другой блок, то просто убираем декорации
            //
            else {
                cursor.movePosition(ScreenplayTextCursor::PreviousCharacter, ScreenplayTextCursor::KeepAnchor);
                if (cursor.hasSelection()) {
                    cursor.deleteChar();
                }
            }
            //
            // ... очищаем значения обрывов
            //
            QTextBlockFormat cleanFormat = blockFormat;
            cleanFormat.clearProperty(PageTextEdit::PropertyDontShowCursor);
            cleanFormat.clearProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart);
            cleanFormat.clearProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionEnd);
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
        if (needToCorrectPageBreaks
            && (atPageEnd || atPageBreak)) {
            switch (ScreenplayBlockStyle::forBlock(block)) {
                //
                // Если это время и место или начало папки
                //
                case ScreenplayParagraphType::SceneHeading:
                case ScreenplayParagraphType::FolderHeader: {
                    //
                    // Переносим на следующую страницу
                    //
                    moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);

                    break;
                }

                //
                // Конец папки распологаем либо только в конце страницы, либо целиком переносим на следующую страницу
                //
                case ScreenplayParagraphType::FolderFooter: {
                    //
                    // Если в конце страницы, оставляем как есть
                    //
                    if (atPageEnd) {
                        //
                        // Запоминаем параметры текущего блока
                        //
                        blockItems[currentBlockInfo.number++] = BlockInfo{blockHeight, lastBlockHeight};
                        //
                        // и идём дальше
                        //
                        lastBlockHeight = 0;
                    }
                    //
                    // В противном случае, просто переносим блок на следующую страницу
                    //
                    else {
                        moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                    }

                    break;
                }

                //
                // Если это участники сцены
                //
                case ScreenplayParagraphType::SceneCharacters: {
                    //
                    // Определим предыдущий блок
                    //
                    QTextBlock previousBlock = findPreviousBlock(block);
                    //
                    // Если перед ним идёт время и место, переносим его тоже
                    //
                    if (previousBlock.isValid()
                        && ScreenplayBlockStyle::forBlock(previousBlock) == ScreenplayParagraphType::SceneHeading) {
                        moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                    }
                    //
                    // В противном случае, просто переносим блок на следующую страницу
                    //
                    else {
                        moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                    }

                    break;
                }

                //
                // Если это имя персонажа
                //
                case ScreenplayParagraphType::Character: {
                    //
                    // Определим предыдущий блок
                    //
                    QTextBlock previousBlock = findPreviousBlock(block);
                    //
                    // Если перед ним идёт время и место, переносим его тоже
                    //
                    if (previousBlock.isValid()
                        && ScreenplayBlockStyle::forBlock(previousBlock) == ScreenplayParagraphType::SceneHeading) {
                        moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                    }
                    //
                    // Если перед ним идут участники сцены, то проверим ещё на один блок назад
                    //
                    else if (previousBlock.isValid()
                             && ScreenplayBlockStyle::forBlock(previousBlock) == ScreenplayParagraphType::SceneCharacters) {
                        //
                        // Проверяем предыдущий блок
                        //
                        QTextBlock prePreviousBlock = findPreviousBlock(previousBlock);
                        //
                        // Если перед участниками идёт время и место, переносим его тоже
                        //
                        if (prePreviousBlock.isValid()
                            && ScreenplayBlockStyle::forBlock(prePreviousBlock) == ScreenplayParagraphType::SceneHeading) {
                            moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                        }
                        //
                        // В противном случае просто переносим вместе с участниками
                        //
                        else {
                            moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                        }
                    }
                    //
                    // В противном случае, просто переносим блок на следующую страницу
                    //
                    else {
                        moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                    }

                    break;
                }


                //
                // Если это ремарка
                //
                case ScreenplayParagraphType::Parenthetical: {
                    //
                    // Определим предыдущий блок
                    //
                    QTextBlock previousBlock = findPreviousBlock(block);
                    //
                    // Если перед ним идёт реплика или лирика, то вставляем блок ДАЛЬШЕ
                    // и переносим на следующую страницу вставляя ПЕРСОНАЖ (ПРОД.) перед ремаркой
                    //
                    if (previousBlock.isValid()
                        && (ScreenplayBlockStyle::forBlock(previousBlock) == ScreenplayParagraphType::Dialogue
                            || ScreenplayBlockStyle::forBlock(previousBlock) == ScreenplayParagraphType::Lyrics)) {
                        breakDialogue(blockFormat, blockHeight, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                    }
                    //
                    // В противном случае, если перед ремаркой идёт персонаж
                    //
                    else if (previousBlock.isValid()
                             && ScreenplayBlockStyle::forBlock(previousBlock) == ScreenplayParagraphType::Character) {
                        //
                        // Проверяем предыдущий блок
                        //
                        QTextBlock prePreviousBlock = findPreviousBlock(previousBlock);
                        //
                        // Если перед персонажем идёт время и место, переносим его тоже
                        //
                        if (prePreviousBlock.isValid()
                            && ScreenplayBlockStyle::forBlock(prePreviousBlock) == ScreenplayParagraphType::SceneHeading) {
                            moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                        }
                        //
                        // В противном случае, если перед персонажем идут участники сцены
                        //
                        else if (prePreviousBlock.isValid()
                                 && ScreenplayBlockStyle::forBlock(prePreviousBlock) == ScreenplayParagraphType::SceneCharacters) {
                            //
                            // Проверяем предыдущий блок
                            //
                            QTextBlock prePrePreviousBlock = findPreviousBlock(prePreviousBlock);
                            //
                            // Если перед участниками идёт время и место, переносим его тоже
                            //
                            if (prePreviousBlock.isValid()
                                && ScreenplayBlockStyle::forBlock(prePreviousBlock) == ScreenplayParagraphType::SceneHeading) {
                                moveCurrentBlockWithThreePreviousToNextPage(prePrePreviousBlock, prePreviousBlock, previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                            }
                            //
                            // В противном случае просто переносим вместе с участниками
                            //
                            else {
                                moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                            }
                        }
                        //
                        // В противном случае просто переносим вместе с участниками
                        //
                        else {
                            moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                        }
                    }
                    //
                    // В противном случае, просто переносим блок на следующую страницу
                    //
                    else {
                        moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                    }

                    break;
                }


                //
                // Если это реплика или лирика и попадает на разрыв
                // - если можно, то оставляем текст так, чтобы он занимал не менее 2 строк,
                //	 добавляем ДАЛЬШЕ и на следующей странице имя персонажа с (ПРОД) и остальной текст
                // - в противном случае
                // -- если перед диалогом идёт имя персонажа, то переносим их вместе на след.
                // -- если перед диалогом идёт ремарка
                // --- если перед ремаркой идёт имя персонажа, то переносим их всех вместе
                // --- если перед ремаркой идёт диалог, то разрываем по ремарке, пишем вместо неё
                //	   ДАЛЬШЕ, а на следующей странице имя персонажа с (ПРОД), ремарку и сам диалог
                //
                case ScreenplayParagraphType::Dialogue:
                case ScreenplayParagraphType::Lyrics: {
                    //
                    // Проверяем следующий блок
                    //
                    QTextBlock nextBlock = findNextBlock(block);
                    //
                    // Если реплика в конце страницы и после ней идёт не ремарка, то оставляем как есть
                    //
                    if (atPageEnd
                        && (!nextBlock.isValid()
                            || (nextBlock.isValid()
                                && ScreenplayBlockStyle::forBlock(nextBlock) != ScreenplayParagraphType::Parenthetical))) {
                        //
                        // Запоминаем параметры текущего блока
                        //
                        blockItems[currentBlockInfo.number++] = BlockInfo{blockHeight, lastBlockHeight};
                        //
                        // и идём дальше
                        //
                        lastBlockHeight = 0;
                    }
                    //
                    // В противном случае разрываем
                    //
                    else {
                        //
                        // Если влезает 2 или более строк
                        //
                        const int minPlacedLines = 2;
                        const int linesCanBePlaced = (pageHeight - lastBlockHeight - blockFormat.topMargin()) / blockLineHeight;
                        int lineToBreak = linesCanBePlaced - 1; // Резервируем одну строку, под блок (ДАЛЬШЕ)
                        //
                        // ... пробуем разорвать на максимально низкой строке
                        //
                        bool isBreaked = false;
                        while (lineToBreak >= minPlacedLines
                               && !isBreaked) {
                            const QTextLine line = block.layout()->lineAt(lineToBreak - 1); // -1 т.к. нужен индекс, а не порядковый номер
                            const QString lineText = block.text().mid(line.textStart(), line.textLength());
                            const int punctuationIndex = lineText.lastIndexOf(kPunctuationCharacter);
                            //
                            // ... нашлось место, где можно разорвать
                            //
                            if (punctuationIndex != -1) {
                                //
                                // ... разрываем
                                //
                                // +1, т.к. символ пунктуации нужно оставить в текущем блоке
                                cursor.setPosition(block.position() + line.textStart() + punctuationIndex + 1);
                                insertBlock(currentBlockWidth(), cursor);
                                //
                                // ... запоминаем параметры блока оставшегося в конце страницы
                                //
                                const qreal breakStartBlockHeight =
                                        lineToBreak * blockLineHeight + blockFormat.topMargin() + blockFormat.bottomMargin();
                                blockItems[currentBlockInfo.number++] = BlockInfo{breakStartBlockHeight, lastBlockHeight};
                                lastBlockHeight += breakStartBlockHeight;
                                //
                                // ... если после разрыва остался пробел, уберём его
                                //
                                if (cursor.block().text().startsWith(" ")) {
                                    cursor.deleteChar();
                                }
                                //
                                // ... помечаем блоки, как разорванные
                                //
                                QTextBlockFormat breakStartFormat = blockFormat;
                                breakStartFormat.setProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart, true);
                                cursor.movePosition(ScreenplayTextCursor::PreviousBlock);
                                cursor.setBlockFormat(breakStartFormat);
                                //
                                QTextBlockFormat breakEndFormat = blockFormat;
                                breakEndFormat.setProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionEnd, true);
                                cursor.movePosition(ScreenplayTextCursor::NextBlock);
                                cursor.setBlockFormat(breakEndFormat);
                                //
                                // ... обновим лэйаут оторванного блока
                                //
                                block = cursor.block();
                                updateBlockLayout(currentBlockWidth(), block);
                                const qreal breakEndBlockHeight =
                                        block.layout()->lineCount() * blockLineHeight + blockFormat.topMargin()
                                        + blockFormat.bottomMargin();
                                //
                                // ... и декорируем разрыв диалога по правилам
                                //
                                breakDialogue(blockFormat, breakEndBlockHeight, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                                //
                                // ... сохраняем оторванный конец блока и корректируем последнюю высоту
                                //
                                block = block.next();
                                blockItems[currentBlockInfo.number++] = BlockInfo{breakEndBlockHeight, lastBlockHeight};
                                lastBlockHeight += breakEndBlockHeight;
                                //
                                // ... помечаем, что разорвать удалось
                                //
                                isBreaked = true;
                                break;
                            }

                            //
                            // На текущей строке разорвать не удалось, перейдём к предыдущей
                            //
                            --lineToBreak;
                        }

                        //
                        // Разорвать не удалось, переносим целиком
                        //
                        if (!isBreaked) {
                            //
                            // Определим предыдущий блок
                            //
                            QTextBlock previousBlock = findPreviousBlock(block);
                            //
                            // Если перед ним идёт персонаж, переносим его тоже
                            //
                            if (previousBlock.isValid()
                                && ScreenplayBlockStyle::forBlock(previousBlock) == ScreenplayParagraphType::Character) {
                                //
                                // Проверяем предыдущий блок
                                //
                                QTextBlock prePreviousBlock = findPreviousBlock(previousBlock);
                                //
                                // Если перед именем идёт время и место, переносим его тоже
                                //
                                if (prePreviousBlock.isValid()
                                    && ScreenplayBlockStyle::forBlock(prePreviousBlock) == ScreenplayParagraphType::SceneHeading) {
                                    moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                                }
                                //
                                // Если перед именем идут участники сцены
                                //
                                else if (prePreviousBlock.isValid()
                                         && ScreenplayBlockStyle::forBlock(prePreviousBlock) == ScreenplayParagraphType::SceneCharacters) {
                                    //
                                    // Проверяем предыдущий блок
                                    //
                                    QTextBlock prePrePreviousBlock = findPreviousBlock(prePreviousBlock);
                                    //
                                    // Если перед участниками идёт время и место, переносим его тоже
                                    //
                                    if (prePreviousBlock.isValid()
                                        && ScreenplayBlockStyle::forBlock(prePreviousBlock) == ScreenplayParagraphType::SceneHeading) {
                                        moveCurrentBlockWithThreePreviousToNextPage(prePrePreviousBlock, prePreviousBlock, previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                                    }
                                    //
                                    // В противном случае просто переносим вместе с участниками
                                    //
                                    else {
                                        moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                                    }
                                }
                                //
                                // В противном случае просто переносим вместе с персонажем
                                //
                                else {
                                    moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                                }
                            }
                            //
                            // Если перед ним идёт ремарка, то проверим ещё на один блок назад
                            //
                            else if (previousBlock.isValid()
                                     && ScreenplayBlockStyle::forBlock(previousBlock) == ScreenplayParagraphType::Parenthetical) {
                                //
                                // Проверяем предыдущий блок
                                //
                                QTextBlock prePreviousBlock = findPreviousBlock(previousBlock);
                                //
                                // Если перед ремаркой идёт персонаж, переносим его тоже
                                //
                                if (prePreviousBlock.isValid()
                                    && ScreenplayBlockStyle::forBlock(prePreviousBlock) == ScreenplayParagraphType::Character) {
                                    moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                                }
                                //
                                // В противном случае разрываем на ремарке
                                //
                                else {
                                    const QTextBlockFormat previousBlockFormat = previousBlock.blockFormat();
                                    const qreal previousBlockHeight =
                                            previousBlockFormat.lineHeight() * previousBlock.layout()->lineCount()
                                            + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
                                    lastBlockHeight -= previousBlockHeight;
                                    --currentBlockInfo.number;
                                    breakDialogue(previousBlockFormat, previousBlockHeight, pageHeight, currentBlockWidth(), cursor, previousBlock, lastBlockHeight);

                                    block = previousBlock;
                                }
                            }
                            //
                            // В противном случае, просто переносим блок на следующую страницу
                            //
                            else {
                                moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                            }
                        }
                    }

                    break;
                }

                //
                // Если это описание действия или любой другой блок, для которого нет собственных правил
                //
                default: {
                    //
                    // Если в конце страницы, оставляем как есть
                    //
                    if (atPageEnd) {
                        //
                        // Запоминаем параметры текущего блока
                        //
                        blockItems[currentBlockInfo.number++] = BlockInfo{blockHeight, lastBlockHeight};
                        //
                        // и идём дальше
                        //
                        lastBlockHeight = 0;
                    }
                    //
                    // Если на разрыве между страниц
                    //
                    else {
                        //
                        // Если влезает 2 или более строк
                        //
                        const int minPlacedLines = 2;
                        const int linesCanBePlaced = (pageHeight - lastBlockHeight - blockFormat.topMargin()) / blockLineHeight;
                        int lineToBreak = linesCanBePlaced;
                        //
                        // ... пробуем разорвать на максимально низкой строке
                        //
                        bool isBreaked = false;
                        while (lineToBreak >= minPlacedLines
                               && !isBreaked) {
                            const QTextLine line = block.layout()->lineAt(lineToBreak - 1); // -1 т.к. нужен индекс, а не порядковый номер
                            const QString lineText = block.text().mid(line.textStart(), line.textLength());
                            const int punctuationIndex = lineText.lastIndexOf(kPunctuationCharacter);
                            //
                            // ... нашлось место, где можно разорвать
                            //
                            if (punctuationIndex != -1) {
                                //
                                // ... разрываем
                                //
                                // +1, т.к. символ пунктуации нужно оставить в текущем блоке
                                cursor.setPosition(block.position() + line.textStart() + punctuationIndex + 1);
                                insertBlock(currentBlockWidth(), cursor);
                                //
                                // ... запоминаем параметры блока оставшегося в конце страницы
                                //
                                const qreal breakStartBlockHeight =
                                        lineToBreak * blockLineHeight + blockFormat.topMargin() + blockFormat.bottomMargin();
                                blockItems[currentBlockInfo.number++] = BlockInfo{breakStartBlockHeight, lastBlockHeight};
                                //
                                // ... если после разрыва остался пробел, уберём его
                                //
                                if (cursor.block().text().startsWith(" ")) {
                                    cursor.deleteChar();
                                }
                                //
                                // ... помечаем блоки, как разорванные
                                //
                                QTextBlockFormat breakStartFormat = blockFormat;
                                breakStartFormat.setProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart, true);
                                cursor.movePosition(ScreenplayTextCursor::PreviousBlock);
                                cursor.setBlockFormat(breakStartFormat);
                                //
                                QTextBlockFormat breakEndFormat = blockFormat;
                                breakEndFormat.setProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionEnd, true);
                                cursor.movePosition(ScreenplayTextCursor::NextBlock);
                                cursor.setBlockFormat(breakEndFormat);
                                //
                                // ... переносим оторванный конец на следующую страницу,
                                //     если на текущую влезает ещё хотя бы одна строка текста
                                //
                                block = cursor.block();
                                const qreal sizeToPageEnd = pageHeight - lastBlockHeight - breakStartBlockHeight;
                                if (sizeToPageEnd >= blockFormat.topMargin() + blockLineHeight) {
                                    moveBlockToNextPage(block, sizeToPageEnd, pageHeight, currentBlockWidth(), cursor);
                                    block = cursor.block();
                                }
                                updateBlockLayout(currentBlockWidth(), block);
                                const qreal breakEndBlockHeight =
                                        block.layout()->lineCount() * blockLineHeight + blockFormat.bottomMargin();
                                //
                                // ... запоминаем параметры блока перенесённого на следующую страницу
                                //
                                blockItems[currentBlockInfo.number++] =
                                        BlockInfo{breakEndBlockHeight, 0};
                                //
                                // ... обозначаем последнюю высоту
                                //
                                lastBlockHeight = breakEndBlockHeight;
                                //
                                // ... помечаем, что разорвать удалось
                                //
                                isBreaked = true;
                                break;
                            }

                            //
                            // На текущей строке разорвать не удалось, перейдём к предыдущей
                            //
                            --lineToBreak;
                        }

                        //
                        // Разорвать не удалось, переносим целиком
                        //
                        if (!isBreaked) {
                            //
                            // Определим предыдущий блок
                            //
                            QTextBlock previousBlock = findPreviousBlock(block);
                            //
                            // Если перед ним идёт время и место, переносим его тоже
                            //
                            if (previousBlock.isValid()
                                && ScreenplayBlockStyle::forBlock(previousBlock) == ScreenplayParagraphType::SceneHeading) {
                                moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                            }
                            //
                            // Если перед ним идут участники сцены, то проверим ещё на один блок назад
                            //
                            else if (previousBlock.isValid()
                                     && ScreenplayBlockStyle::forBlock(previousBlock) == ScreenplayParagraphType::SceneCharacters) {
                                //
                                // Проверяем предыдущий блок
                                //
                                QTextBlock prePreviousBlock = findPreviousBlock(previousBlock);
                                //
                                // Если перед участниками идёт время и место, переносим его тоже
                                //
                                if (prePreviousBlock.isValid()
                                    && ScreenplayBlockStyle::forBlock(prePreviousBlock) == ScreenplayParagraphType::SceneHeading) {
                                    moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                                }
                                //
                                // В противном случае просто переносим вместе с участниками
                                //
                                else {
                                    moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                                }
                            }
                            //
                            // В противном случае, просто переносим блок на следующую страницу
                            //
                            else {
                                moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, currentBlockWidth(), cursor, block, lastBlockHeight);
                            }
                        }
                    }

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
            blockItems[currentBlockInfo.number++] = BlockInfo{blockHeight, lastBlockHeight};
            //
            // и идём дальше
            //
            lastBlockHeight += blockHeight;
        }

        block = block.next();
    }

    cursor.endEditBlock();
}

void ScreenplayTextCorrector::Implementation::moveCurrentBlockWithThreePreviousToNextPage(const QTextBlock& _prePrePreviousBlock,
    const QTextBlock& _prePreviousBlock, const QTextBlock& _previousBlock, qreal _pageHeight,
    qreal _pageWidth, ScreenplayTextCursor& _cursor, QTextBlock& _block, qreal& _lastBlockHeight)
{
    --currentBlockInfo.number;
    --currentBlockInfo.number;
    --currentBlockInfo.number;

    const QTextBlockFormat previousBlockFormat = _previousBlock.blockFormat();
    const qreal previousBlockHeight =
            previousBlockFormat.lineHeight() * _previousBlock.layout()->lineCount()
            + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
    const QTextBlockFormat prePreviousBlockFormat = _prePreviousBlock.blockFormat();
    const qreal prePreviousBlockHeight =
            prePreviousBlockFormat.lineHeight() * _prePreviousBlock.layout()->lineCount()
            + prePreviousBlockFormat.topMargin() + prePreviousBlockFormat.bottomMargin();
    const QTextBlockFormat prePrePreviousBlockFormat = _prePrePreviousBlock.blockFormat();
    const qreal prePrePreviousBlockHeight =
            prePrePreviousBlockFormat.lineHeight() * _prePrePreviousBlock.layout()->lineCount()
            + prePrePreviousBlockFormat.topMargin() + prePrePreviousBlockFormat.bottomMargin();
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight + previousBlockHeight + prePreviousBlockHeight + prePrePreviousBlockHeight;
    moveBlockToNextPage(_prePrePreviousBlock, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
    _block = _cursor.block();
    //
    // Запоминаем параметры блока время и места
    //
    blockItems[currentBlockInfo.number++] =
            BlockInfo{prePreviousBlockHeight - prePreviousBlockFormat.topMargin(), 0};
    //
    // Обозначаем последнюю высоту, как высоту предпредпредыдущего блока
    //
    _lastBlockHeight = prePrePreviousBlockHeight - prePrePreviousBlockFormat.topMargin();
    //
    // Текущий блок будет обработан, как очередной блок посередине страницы при следующем проходе
    //
}

void ScreenplayTextCorrector::Implementation::moveCurrentBlockWithTwoPreviousToNextPage(const QTextBlock& _prePreviousBlock,
    const QTextBlock& _previousBlock, qreal _pageHeight, qreal _pageWidth, ScreenplayTextCursor& _cursor,
    QTextBlock& _block, qreal& _lastBlockHeight)
{
    --currentBlockInfo.number;
    --currentBlockInfo.number;

    const QTextBlockFormat previousBlockFormat = _previousBlock.blockFormat();
    const qreal previousBlockHeight =
            previousBlockFormat.lineHeight() * _previousBlock.layout()->lineCount()
            + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
    const QTextBlockFormat prePreviousBlockFormat = _prePreviousBlock.blockFormat();
    const qreal prePreviousBlockHeight =
            prePreviousBlockFormat.lineHeight() * _prePreviousBlock.layout()->lineCount()
            + prePreviousBlockFormat.topMargin() + prePreviousBlockFormat.bottomMargin();
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight + previousBlockHeight + prePreviousBlockHeight;
    moveBlockToNextPage(_prePreviousBlock, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
    _block = _cursor.block();
    //
    // Запоминаем параметры блока время и места
    //
    blockItems[currentBlockInfo.number++] =
            BlockInfo{prePreviousBlockHeight - prePreviousBlockFormat.topMargin(), 0};
    //
    // Обозначаем последнюю высоту, как высоту предпредыдущего блока
    //
    _lastBlockHeight = prePreviousBlockHeight - prePreviousBlockFormat.topMargin();
    //
    // Текущий блок будет обработан, как очередной блок посередине страницы при следующем проходе
    //
}

void ScreenplayTextCorrector::Implementation::moveCurrentBlockWithPreviousToNextPage(const QTextBlock& _previousBlock,
    qreal _pageHeight, qreal _pageWidth, ScreenplayTextCursor& _cursor, QTextBlock& _block, qreal& _lastBlockHeight)
{
    --currentBlockInfo.number;

    const QTextBlockFormat previousBlockFormat = _previousBlock.blockFormat();
    const qreal previousBlockHeight =
            previousBlockFormat.lineHeight() * _previousBlock.layout()->lineCount()
            + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight + previousBlockHeight;
    moveBlockToNextPage(_previousBlock, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
    _block = _cursor.block();
    //
    // Запоминаем параметры предыдущего блока
    //
    blockItems[currentBlockInfo.number++] =
            BlockInfo{previousBlockHeight - previousBlockFormat.topMargin(), 0};
    //
    // Обозначаем последнюю высоту, как высоту предыдущего блока
    //
    _lastBlockHeight = previousBlockHeight - previousBlockFormat.topMargin();
    //
    // Текущий блок будет обработан, как очередной блок посередине страницы при следующем проходе
    //
}

void ScreenplayTextCorrector::Implementation::moveCurrentBlockToNextPage(const QTextBlockFormat& _blockFormat,
    qreal _blockHeight, qreal _pageHeight, qreal _pageWidth, ScreenplayTextCursor& _cursor, QTextBlock& _block,
    qreal& _lastBlockHeight)
{
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight;
    moveBlockToNextPage(_block, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
    _block = _cursor.block();
    //
    // Запоминаем параметры текущего блока
    //
    blockItems[currentBlockInfo.number++] = BlockInfo{_blockHeight - _blockFormat.topMargin(), 0};
    //
    // Обозначаем последнюю высоту, как высоту текущего блока
    //
    _lastBlockHeight = _blockHeight - _blockFormat.topMargin();
}

void ScreenplayTextCorrector::Implementation::breakDialogue(const QTextBlockFormat& _blockFormat, qreal _blockHeight,
    qreal _pageHeight, qreal _pageWidth, ScreenplayTextCursor& _cursor, QTextBlock& _block,
    qreal& _lastBlockHeight)
{
    //
    // Вставить блок
    //
    _cursor.setPosition(_block.position());
    insertBlock(_pageWidth, _cursor);
    updateBlockLayout(_pageWidth, _cursor.block());
    _cursor.movePosition(ScreenplayTextCursor::PreviousBlock);
    //
    // Оформить его, как персонажа, но без отступа сверху
    //
    const auto moreKeywordStyle = ScreenplayTemplateFacade::getTemplate(templateId)
                                  .blockStyle(ScreenplayParagraphType::Character);
    QTextBlockFormat moreKeywordFormat = moreKeywordStyle.blockFormat(_cursor.inTable());
    moreKeywordFormat.setTopMargin(0);
    moreKeywordFormat.setProperty(ScreenplayBlockStyle::PropertyIsCorrection, true);
    moreKeywordFormat.setProperty(ScreenplayBlockStyle::PropertyIsCorrectionContinued, true);
    moreKeywordFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
    _cursor.setBlockFormat(moreKeywordFormat);
    _cursor.setBlockCharFormat(moreKeywordStyle.charFormat());
    //
    // Вставить текст ДАЛЬШЕ
    //
    _cursor.insertText(moreTerm());
    _block = _cursor.block();
    updateBlockLayout(_pageWidth, _block);
    const qreal moreBlockHeight =
            _block.layout()->lineCount() * moreKeywordFormat.lineHeight()
            + moreKeywordFormat.topMargin()
            + moreKeywordFormat.bottomMargin();
    blockItems[currentBlockInfo.number++] = BlockInfo{moreBlockHeight, _lastBlockHeight};
    //
    // Перенести текущий блок на следующую страницу, если на текущей влезает
    // ещё хотя бы одна строка текста
    //
    _cursor.movePosition(ScreenplayTextCursor::NextBlock);
    _block = _cursor.block();
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight - moreBlockHeight;
    if (sizeToPageEnd >= _blockFormat.topMargin() + _blockFormat.lineHeight()) {
        moveBlockToNextPage(_block, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
        _block = _cursor.block();
    }
    //
    // Вставить перед текущим блоком декорацию ПЕРСОНАЖ (ПРОД.)
    // и для этого ищём предыдущий блок с именем персонажа
    //
    QTextBlock characterBlock = _block.previous();
    while (characterBlock.isValid()
           && (characterBlock.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)
               || ScreenplayBlockStyle::forBlock(characterBlock) != ScreenplayParagraphType::Character)) {
        characterBlock = characterBlock.previous();
    }
    //
    // Если нашли блок с именем персонажа
    //
    if (characterBlock.isValid()) {
        //
        // Вставляем блок
        //
        _cursor.setPosition(_block.position());
        insertBlock(_pageWidth, _cursor);
        _cursor.movePosition(ScreenplayTextCursor::PreviousBlock);
        //
        // Оформляем его, как имя персонажа
        //
        QTextBlockFormat characterBlockFormat = characterBlock.blockFormat();
        characterBlockFormat.setProperty(ScreenplayBlockStyle::PropertyIsCorrection, true);
        characterBlockFormat.setProperty(ScreenplayBlockStyle::PropertyIsCorrectionCharacter, true);
        characterBlockFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
        _cursor.setBlockFormat(characterBlockFormat);
        _cursor.setBlockCharFormat(characterBlock.charFormat());
        //
        // И вставляем текст с именем персонажа
        //
        const QString characterName = CharacterParser::name(characterBlock.text());
        _cursor.insertText(characterName + continuedTerm());
        _block = _cursor.block();
        //
        updateBlockLayout(_pageWidth, _block);
        const qreal continuedBlockHeight =
                _block.layout()->lineCount() * characterBlockFormat.lineHeight()
                + characterBlockFormat.bottomMargin();
        blockItems[currentBlockInfo.number++] = BlockInfo{continuedBlockHeight, 0};
        //
        // Обозначаем последнюю высоту, как высоту предыдущего блока
        //
        _lastBlockHeight = continuedBlockHeight;
        //
        // Текущий блок будет обработан, как очередной блок посередине страницы при следующем проходе
        //
    }
    //
    // А если не нашли то оставляем блок прямо сверху страницы
    //
    else {
        blockItems[currentBlockInfo.number++] = BlockInfo{_blockHeight - _blockFormat.topMargin(), 0};
        _lastBlockHeight = _blockHeight - _blockFormat.topMargin();
    }
}

QTextBlock ScreenplayTextCorrector::Implementation::findPreviousBlock(const QTextBlock& _block)
{
    QTextBlock previousBlock = _block.previous();
    while (previousBlock.isValid() && !previousBlock.isVisible()) {
        --currentBlockInfo.number;
        previousBlock = previousBlock.previous();
    }
    return previousBlock;
}

QTextBlock ScreenplayTextCorrector::Implementation::findNextBlock(const QTextBlock& _block)
{
    QTextBlock nextBlock = _block.next();
    while (nextBlock.isValid()
           && !nextBlock.isVisible()
           && nextBlock.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
        nextBlock = nextBlock.next();
    }
    return nextBlock;
}

void ScreenplayTextCorrector::Implementation::moveBlockToNextPage(const QTextBlock& _block, qreal _spaceToPageEnd,
    qreal _pageHeight, qreal _pageWidth, ScreenplayTextCursor& _cursor)
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
    const auto paragraphType = ScreenplayBlockStyle::forBlock(_block);
    if (paragraphType == ScreenplayParagraphType::SceneHeading) {
        decorationFormat.setProperty(ScreenplayBlockStyle::PropertyType,
                                     static_cast<int>(ScreenplayParagraphType::SceneHeadingShadow));
    }
    if (paragraphType == ScreenplayParagraphType::FolderHeader
        || paragraphType == ScreenplayParagraphType::FolderHeader) {
        decorationFormat.setProperty(ScreenplayBlockStyle::PropertyType,
                                     static_cast<int>(ScreenplayParagraphType::Action));
        decorationFormat.setBackground(Qt::NoBrush);
    }
    decorationFormat.setProperty(ScreenplayBlockStyle::PropertyIsCorrection, true);
    decorationFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
    //
    // Вставляем блоки декорации
    //
    for (int blockIndex = 0; blockIndex < insertBlockCount; ++blockIndex) {
        //
        // Декорируем
        //
        insertBlock(_pageWidth, _cursor);
        _cursor.movePosition(ScreenplayTextCursor::PreviousBlock);
        _cursor.setBlockFormat(decorationFormat);
        //
        // Сохраним данные блока, чтобы перенести их к реальному владельцу
        //
        ScreenplayTextBlockData* blockData = nullptr;
        auto block = _cursor.block();
        if (block.userData() != nullptr) {
            blockData = new ScreenplayTextBlockData(static_cast<ScreenplayTextBlockData*>(block.userData()));
            block.setUserData(nullptr);
        }
        //
        // Запоминаем параметры текущего блока
        //
        blockItems[currentBlockInfo.number++] = BlockInfo{blockHeight, _pageHeight - _spaceToPageEnd + blockIndex * blockHeight};
        //
        // Переведём курсор на блок после декорации
        //
        _cursor.movePosition(ScreenplayTextCursor::NextBlock);
        if (blockData != nullptr) {
            _cursor.block().setUserData(blockData);
        }
    }
}


// ****


QString ScreenplayTextCorrector::continuedTerm()
{
    return QString(" (%1)").arg(QApplication::translate("BusinessLogic::ScriptTextCorrector", kContinuedTerm));
}

ScreenplayTextCorrector::ScreenplayTextCorrector(QTextDocument* _document) :
    QObject(_document),
    d(new Implementation(_document))
{
    Q_ASSERT_X(d->document, Q_FUNC_INFO, "Document couldn't be a nullptr");
}

ScreenplayTextCorrector::~ScreenplayTextCorrector() = default;

void ScreenplayTextCorrector::setTemplateId(const QString& _templateId)
{
    if (d->templateId == _templateId) {
        return;
    }

    d->templateId = _templateId;
    correct();
}

void ScreenplayTextCorrector::setNeedToCorrectCharactersNames(bool _need)
{
    if (d->needToCorrectCharactersNames == _need) {
        return;
    }

    d->needToCorrectCharactersNames = _need;
    correct();
}

void ScreenplayTextCorrector::setNeedToCorrectPageBreaks(bool _need)
{
    if (d->needToCorrectPageBreaks == _need) {
        return;
    }

    d->needToCorrectPageBreaks = _need;
    clear();
    correct();
}

void ScreenplayTextCorrector::clear()
{
    d->lastDocumentSize = QSizeF();
    d->currentBlockInfo.number = 0;
    d->blockItems.clear();
}

void ScreenplayTextCorrector::correct(int _position, int _charsChanged)
{
    //
    // Избегаем рекурсии, которая может возникать от того, что корректировка происходит
    // при изменении текста документа, но в то же время корректор сам делает изменения в документе
    //
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    if (d->needToCorrectCharactersNames) {
        d->correctCharactersNames(_position, _charsChanged);
    }

    if (d->needToCorrectPageBreaks) {
        d->correctPageBreaks(_position);
    }
}

void ScreenplayTextCorrector::planCorrection(int _position, int _charsRemoved, int _charsAdded)
{
    //
    // Если корректировка ещё не была запланирована, то просто заполняем информацию
    // об изменённой части текстового документ
    //
    if (!d->plannedCorrection.isValid) {
        d->plannedCorrection = { true, _position, std::max(_charsRemoved, _charsAdded) };
    }
    //
    // А если уже была запланирована, то расширим выделение
    //
    else if (d->plannedCorrection.position > _position) {
        const auto newPosition = _position;
        const auto newChanged = std::max(_charsRemoved, _charsAdded);
        if (newPosition < d->plannedCorrection.position) {
            d->plannedCorrection.changed += d->plannedCorrection.position - newPosition;
            d->plannedCorrection.position = newPosition;
        }
        const auto newEnd = newPosition + newChanged;
        if (newEnd > d->plannedCorrection.end()) {
            d->plannedCorrection.changed = newEnd - d->plannedCorrection.position;
        }
    }
}

void ScreenplayTextCorrector::makePlannedCorrection()
{
    if (!d->plannedCorrection.isValid) {
        return;
    }

    correct(d->plannedCorrection.position, d->plannedCorrection.changed);
    d->plannedCorrection = {};
}

} // namespace Ui
