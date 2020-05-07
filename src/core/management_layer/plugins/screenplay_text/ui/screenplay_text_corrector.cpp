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

#include <cmath>

using BusinessLayer::CharacterParser;
using BusinessLayer::ScreenplayBlockStyle;
using BusinessLayer::ScreenplayParagraphType;
using BusinessLayer::ScreenplayTemplateFacade;


namespace Ui
{

namespace {
    /**
     * @brief Список символов пунктуации, разделяющие предложения
     */
    const QList<QChar> PUNCTUATION_CHARACTERS = { '.', '!', '?', ':', ';', QString("…").at(0)};

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
    void updateBlockLayout(int _pageWidth, const QTextBlock& _block) {
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
    Implementation(QTextDocument* _document, const QString& _templateName);

    /**
     * @brief Скорректировать имена персонажей
     */
    void correctCharactersNames(int _position = -1, int _charsRemoved = 0, int _charsAdded = 0);

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
    QString templateName;

    /**
     * @brief Необходимо ли корректировать текст блоков имён персонажей
     */
    bool needToCorrectCharactersNames = true;

    /**
     * @brief Необходимо ли корректировать текст на разрывах страниц
     */
    bool needToCorrectPageBreaks = true;

    /**
     * @brief Размер документа при последней проверке
     */
    QSizeF lastDocumentSize;

    /**
     * @brief Номер текущего блока при корректировке
     * @note Используем собственный счётчик номеров, т.к. во время
     *       коррекций номера блоков могут скакать в QTextBlock
     */
    int currentBlockNumber = 0;

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

ScreenplayTextCorrector::Implementation::Implementation(QTextDocument* _document, const QString& _templateName)
    : document(_document),
      templateName(_templateName)
{
}

void ScreenplayTextCorrector::Implementation::correctCharactersNames(int _position, int _charsRemoved, int _charsAdded)
{
    //
    // Определим границы работы алгоритма
    //
    int startPosition = _position;
    int endPosition = _position + (std::max(_charsRemoved, _charsAdded));
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
                // Если второе подряд, добавляем вспомогательный текст
                //
                else if (characterName == lastCharacterName){
                    const QString characterState = CharacterParser::extension(block.text());
                    //
                    // ... помечаем блок, что нужно отрисовывать вспомогательный текст
                    //
                    QTextBlockFormat characterFormat = block.blockFormat();
                    characterFormat.setProperty(ScreenplayBlockStyle::PropertyIsCharacterContinued,
                                                needToCorrectCharactersNames && characterState.isEmpty());
                    cursor.setPosition(block.position());
                    cursor.setBlockFormat(characterFormat);
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
    if (pageWidth < 0)
        return;
    //
    const qreal pageHeight = document->pageSize().height()
                             - rootFrameFormat.topMargin()
                             - rootFrameFormat.bottomMargin();
    if (pageHeight < 0)
        return;
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
    // Определим список блоков для принудительной ручной проверки
    // NOTE: Это необходимо для того, чтобы корректно обрабатывать изменение текста
    //       в предыдущих и следующих за переносом блоках
    //
    QSet<int> blocksToRecheck;
    if (_position != -1) {
        QTextBlock blockToRecheck = document->findBlock(_position);
        //
        // ... спускаемся на два блока вперёд
        //
        blockToRecheck = blockToRecheck.next();
        blockToRecheck = blockToRecheck.next();
        //
        // ... и возвращаемся на пять блоков назад
        // NOTE: максимальное количество блоков которое может быть перенесено на новую страницу
        //
        int recheckBlocksCount = 5;
        do {
            blocksToRecheck.insert(blockToRecheck.blockNumber());
            blockToRecheck = blockToRecheck.previous();
        } while (blockToRecheck.isValid()
                 && (recheckBlocksCount-- > 0
                     || blockToRecheck.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)
                     || blockToRecheck.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart)
                     || blockToRecheck.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionEnd)));
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
    currentBlockNumber = 0;
    while (block.isValid()) {
        //
        // Пропускаем невидимые блоки
        //
        if (!block.isVisible()) {
            block = block.next();
            ++currentBlockNumber;
            continue;
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
        qreal nextBlockHeight = 0;
        if (block.next().isValid()) {
            const qreal nextBlockLineHeight = block.next().blockFormat().lineHeight();
            const QTextBlockFormat nextBlockFormat = block.next().blockFormat();
            nextBlockHeight = nextBlockLineHeight + nextBlockFormat.topMargin();
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
                    || lastBlockHeight + blockHeight + nextBlockHeight > pageHeight);
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
                  && qFuzzyCompare(blockItems[currentBlockNumber].top, 0.0);
        if (blockItems[currentBlockNumber].isValid()
            && !isBlockEmptyDecorationOnTopOfThePage
            && qFuzzyCompare(blockItems[currentBlockNumber].height, blockHeight)
            && qFuzzyCompare(blockItems[currentBlockNumber].top, lastBlockHeight)
            && !blocksToRecheck.contains(currentBlockNumber)) {
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
            ++currentBlockNumber;
            continue;
        }


        //
        // Если позиция блока изменилась, то работаем по алгоритму корректировки текста
        //


        //
        // Работаем с декорациями
        //
        // ... если блок декорация, то удаляем его
        //
        if (blockFormat.boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
            cursor.setPosition(block.position());
            cursor.movePosition(ScreenplayTextCursor::EndOfBlock, ScreenplayTextCursor::KeepAnchor);
            cursor.movePosition(ScreenplayTextCursor::NextCharacter, ScreenplayTextCursor::KeepAnchor);
            cursor.deleteChar();
            //
            // ... и продолжим со следующего блока
            //
            block = cursor.block();
            continue;
        }
        //
        // ... если в текущем блоке начинается разрыв, пробуем его вернуть
        //
        else if (blockFormat.boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart)) {
            cursor.setPosition(block.position());
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
            cleanFormat.setProperty(PageTextEdit::PropertyDontShowCursor, QVariant());
            cleanFormat.setProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart, QVariant());
            cleanFormat.setProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionEnd, QVariant());
            cursor.setBlockFormat(cleanFormat);
            //
            // ... и проработаем текущий блок с начала
            //
            block = cursor.block();
            updateBlockLayout(static_cast<int>(pageWidth), block);
            continue;
        }


        //
        // Работаем с переносами
        //
        if (needToCorrectPageBreaks
            && (atPageEnd || atPageBreak)) {
            switch (ScreenplayBlockStyle::forBlock(block)) {
                //
                // Если это время и место
                //
                case ScreenplayParagraphType::SceneHeading: {
                    //
                    // Переносим на следующую страницу
                    //
                    moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);

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
                        moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
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
                        moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
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
                            moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                        }
                        //
                        // В противном случае просто переносим вместе с участниками
                        //
                        else {
                            moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                        }
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
                        breakDialogue(blockFormat, blockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);
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
                            moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
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
                                moveCurrentBlockWithThreePreviousToNextPage(prePrePreviousBlock, prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                            }
                            //
                            // В противном случае просто переносим вместе с участниками
                            //
                            else {
                                moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                            }
                        }
                        //
                        // В противном случае просто переносим вместе с участниками
                        //
                        else {
                            moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                        }
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
                        blockItems[currentBlockNumber++] = BlockInfo{blockHeight, lastBlockHeight};
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
                            for (const auto& punctuation : PUNCTUATION_CHARACTERS) {
                                const int punctuationIndex = lineText.lastIndexOf(punctuation);
                                //
                                // ... нашлось место, где можно разорвать
                                //
                                if (punctuationIndex != -1) {
                                    //
                                    // ... разрываем
                                    //
                                    // +1, т.к. символ пунктуации нужно оставить в текущем блоке
                                    cursor.setPosition(block.position() + line.textStart() + punctuationIndex + 1);
                                    insertBlock(pageWidth, cursor);
                                    //
                                    // ... запоминаем параметры блока оставшегося в конце страницы
                                    //
                                    const qreal breakStartBlockHeight =
                                            lineToBreak * blockLineHeight + blockFormat.topMargin() + blockFormat.bottomMargin();
                                    blockItems[currentBlockNumber++] = BlockInfo{breakStartBlockHeight, lastBlockHeight};
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
                                    updateBlockLayout(pageWidth, block);
                                    const qreal breakEndBlockHeight =
                                            block.layout()->lineCount() * blockLineHeight + blockFormat.topMargin()
                                            + blockFormat.bottomMargin();
                                    //
                                    // ... и декорируем разрыв диалога по правилам
                                    //
                                    breakDialogue(blockFormat, breakEndBlockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                    //
                                    // ... сохраняем оторванный конец блока и корректируем последнюю высоту
                                    //
                                    block = block.next();
                                    blockItems[currentBlockNumber++] = BlockInfo{breakEndBlockHeight, lastBlockHeight};
                                    lastBlockHeight += breakEndBlockHeight;
                                    //
                                    // ... помечаем, что разорвать удалось
                                    //
                                    isBreaked = true;
                                    break;
                                }
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
                                    moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
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
                                        moveCurrentBlockWithThreePreviousToNextPage(prePrePreviousBlock, prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                    }
                                    //
                                    // В противном случае просто переносим вместе с участниками
                                    //
                                    else {
                                        moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                    }
                                }
                                //
                                // В противном случае просто переносим вместе с персонажем
                                //
                                else {
                                    moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
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
                                    moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
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
                                    --currentBlockNumber;
                                    breakDialogue(previousBlockFormat, previousBlockHeight, pageHeight, pageWidth, cursor, previousBlock, lastBlockHeight);

                                    block = previousBlock;
                                }
                            }
                            //
                            // В противном случае, просто переносим блок на следующую страницу
                            //
                            else {
                                moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);
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
                        blockItems[currentBlockNumber++] = BlockInfo{blockHeight, lastBlockHeight};
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
                            for (const auto& punctuation : PUNCTUATION_CHARACTERS) {
                                const int punctuationIndex = lineText.lastIndexOf(punctuation);
                                //
                                // ... нашлось место, где можно разорвать
                                //
                                if (punctuationIndex != -1) {
                                    //
                                    // ... разрываем
                                    //
                                    // +1, т.к. символ пунктуации нужно оставить в текущем блоке
                                    cursor.setPosition(block.position() + line.textStart() + punctuationIndex + 1);
                                    insertBlock(pageWidth, cursor);
                                    //
                                    // ... запоминаем параметры блока оставшегося в конце страницы
                                    //
                                    const qreal breakStartBlockHeight =
                                            lineToBreak * blockLineHeight + blockFormat.topMargin() + blockFormat.bottomMargin();
                                    blockItems[currentBlockNumber++] = BlockInfo{breakStartBlockHeight, lastBlockHeight};
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
                                        moveBlockToNextPage(block, sizeToPageEnd, pageHeight, pageWidth, cursor);
                                        block = cursor.block();
                                    }
                                    updateBlockLayout(pageWidth, block);
                                    const qreal breakEndBlockHeight =
                                            block.layout()->lineCount() * blockLineHeight + blockFormat.bottomMargin();
                                    //
                                    // ... запоминаем параметры блока перенесённого на следующую страницу
                                    //
                                    blockItems[currentBlockNumber++] =
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
                                moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
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
                                    moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                }
                                //
                                // В противном случае просто переносим вместе с участниками
                                //
                                else {
                                    moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                }
                            }
                            //
                            // В противном случае, просто переносим блок на следующую страницу
                            //
                            else {
                                moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);
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
            blockItems[currentBlockNumber++] = BlockInfo{blockHeight, lastBlockHeight};
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
    --currentBlockNumber;
    --currentBlockNumber;
    --currentBlockNumber;

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
    blockItems[currentBlockNumber++] =
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
    --currentBlockNumber;
    --currentBlockNumber;

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
    blockItems[currentBlockNumber++] =
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
    --currentBlockNumber;

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
    blockItems[currentBlockNumber++] =
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
    blockItems[currentBlockNumber++] = BlockInfo{_blockHeight - _blockFormat.topMargin(), 0};
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
    // Оформить его, как ремарку
    //
    const auto parentheticalStyle = ScreenplayTemplateFacade::getTemplate(templateName)
                                    .blockStyle(ScreenplayParagraphType::Parenthetical);
    QTextBlockFormat parentheticalFormat = parentheticalStyle.blockFormat();
    parentheticalFormat.setProperty(ScreenplayBlockStyle::PropertyIsCorrection, true);
    parentheticalFormat.setProperty(ScreenplayBlockStyle::PropertyIsCorrectionContinued, true);
    parentheticalFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
    _cursor.setBlockFormat(parentheticalFormat);
    //
    // Вставить текст ДАЛЬШЕ
    //
    _cursor.insertText(moreTerm());
    _block = _cursor.block();
    updateBlockLayout(_pageWidth, _block);
    const qreal moreBlockHeight =
            _block.layout()->lineCount() * parentheticalFormat.lineHeight()
            + parentheticalFormat.topMargin()
            + parentheticalFormat.bottomMargin();
    blockItems[currentBlockNumber++] = BlockInfo{moreBlockHeight, _lastBlockHeight};
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
           && ScreenplayBlockStyle::forBlock(characterBlock) != ScreenplayParagraphType::Character) {
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
        QTextBlockFormat characterFormat = characterBlock.blockFormat();
        characterFormat.setProperty(ScreenplayBlockStyle::PropertyIsCorrection, true);
        characterFormat.setProperty(ScreenplayBlockStyle::PropertyIsCorrectionCharacter, true);
        characterFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
        _cursor.setBlockFormat(characterFormat);
        //
        // И вставляем текст с именем персонажа
        //
        const QString characterName = CharacterParser::name(characterBlock.text());
        _cursor.insertText(characterName + continuedTerm());
        _block = _cursor.block();
        //
        updateBlockLayout(_pageWidth, _block);
        const qreal continuedBlockHeight =
                _block.layout()->lineCount() * characterFormat.lineHeight()
                + characterFormat.bottomMargin();
        blockItems[currentBlockNumber++] = BlockInfo{continuedBlockHeight, 0};
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
        blockItems[currentBlockNumber++] = BlockInfo{_blockHeight - _blockFormat.topMargin(), 0};
        _lastBlockHeight = _blockHeight - _blockFormat.topMargin();
    }
}

QTextBlock ScreenplayTextCorrector::Implementation::findPreviousBlock(const QTextBlock& _block)
{
    QTextBlock previousBlock = _block.previous();
    while (previousBlock.isValid() && !previousBlock.isVisible()) {
        --currentBlockNumber;
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
    if (ScreenplayBlockStyle::forBlock(_block) == ScreenplayParagraphType::SceneHeading) {
        decorationFormat.setProperty(ScreenplayBlockStyle::PropertyType,
                                     static_cast<int>(ScreenplayParagraphType::SceneHeadingShadow));
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
        blockItems[currentBlockNumber++] = BlockInfo{blockHeight, _pageHeight - _spaceToPageEnd + blockIndex * blockHeight};
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

ScreenplayTextCorrector::ScreenplayTextCorrector(QTextDocument* _document, const QString& _templateName) :
    QObject(_document),
    d(new Implementation(_document, _templateName))
{
    Q_ASSERT_X(d->document, Q_FUNC_INFO, "Document couldn't be a nullptr");
}

ScreenplayTextCorrector::~ScreenplayTextCorrector() = default;

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
    d->currentBlockNumber = 0;
    d->blockItems.clear();
}

void ScreenplayTextCorrector::correct(int _position, int _charRemoved, int _charAdded)
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
        d->correctCharactersNames(_position, _charRemoved, _charAdded);
    }

    if (d->needToCorrectPageBreaks) {
        d->correctPageBreaks(_position);
    }
}

} // namespace Ui
