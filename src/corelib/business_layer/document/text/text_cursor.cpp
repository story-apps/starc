#include "text_cursor.h"

#include "text_block_data.h"
#include "text_document.h"

#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/base/base_text_edit.h>
#include <utils/shugar.h>

#include <QTextTable>
#include <QtGui/private/qtextdocument_p.h>


namespace BusinessLayer {

namespace {
TextBlockData* cloneBlockData(const QTextBlock& _block)
{
    TextBlockData* clonedBlockData = nullptr;
    if (_block.userData() != nullptr) {
        const auto blockData = static_cast<BusinessLayer::TextBlockData*>(_block.userData());
        if (blockData != nullptr) {
            clonedBlockData = new TextBlockData(blockData);
        }
    }
    return clonedBlockData;
}

} // namespace


TextCursor::TextCursor()
    : QTextCursor()
{
}

TextCursor::TextCursor(const QTextCursor& _other)
    : QTextCursor(_other)
{
}

TextCursor::TextCursor(QTextDocument* _document)
    : QTextCursor(_document)
{
}

TextCursor::~TextCursor() = default;

bool TextCursor::isInEditBlock() const
{
#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
    return QTextDocumentPrivate::get(document())->isInEditBlock();
#else
    return document()->docHandle()->isInEditBlock();
#endif
}

bool TextCursor::inTable() const
{
    return currentTable() != nullptr;
}

bool TextCursor::isTableEmpty(bool _skipCurrentBlockEmptynessCheck) const
{
    if (!inTable()) {
        return false;
    }

    auto cursor = *this;

    //
    // Идём до начала таблицы
    //
    while (cursor.inTable()) {
        cursor.movePosition(TextCursor::PreviousBlock);
    }
    cursor.movePosition(TextCursor::NextBlock);
    //
    // Идём до конца таблицы
    //
    while (cursor.inTable()) {
        const bool skipBlock = _skipCurrentBlockEmptynessCheck && cursor.block() == this->block();
        if (!skipBlock && !cursor.block().text().isEmpty()) {
            return false;
        }
        cursor.movePosition(TextCursor::NextBlock);
    }

    return true;
}

bool TextCursor::inFirstColumn() const
{
    return currentTable() && currentTable()->cellAt(*this).column() == 0;
}

TextCursor::Selection TextCursor::selectionInterval() const
{
    if (!hasSelection()) {
        return { position(), position() };
    }

    if (selectionStart() > selectionEnd()) {
        return { selectionEnd(), selectionStart() };
    } else {
        return { selectionStart(), selectionEnd() };
    }
}

void TextCursor::restartEditBlock()
{
    endEditBlock();

    int editsCount = 0;
    while (isInEditBlock()) {
        ++editsCount;
        endEditBlock();
    }

    joinPreviousEditBlock();

    while (editsCount != 0) {
        beginEditBlock();
        --editsCount;
    }
}

void TextCursor::removeCharacters(BaseTextEdit* _editor)
{
    removeCharacters(true, _editor);
}

void TextCursor::removeCharacters(bool _backward, BaseTextEdit* _editor)
{
    Q_ASSERT(document() == _editor->document());

    TextCursor cursor = _editor->textCursor();
    if (!cursor.hasSelection()) {
        //
        // Если в начале документа нажат backspace
        //
        if (cursor.atStart() && _backward == true) {
            return;
        }

        //
        // Если в конце документа нажат delete
        //
        if (cursor.atEnd() && _backward == false) {
            return;
        }
    }

    //
    // Определим границы выделения
    //
    int topCursorPosition = 0;
    int bottomCursorPosition = 0;
    {
        if (cursor.hasSelection()) {
            topCursorPosition = cursor.selectionInterval().from;
            bottomCursorPosition = cursor.selectionInterval().to;
        } else {
            topCursorPosition = cursor.position() - (_backward ? 1 : 0);
            bottomCursorPosition = topCursorPosition + 1;

            TextCursor checkCursor = cursor;
            checkCursor.setPosition(topCursorPosition);
            //
            // ... переходим через корректирующие блоки блоки вперёд
            //
            if (!_backward && checkCursor.atBlockEnd()) {
                //
                // ... в разрыве, удаляем символы в оторванном абзаце
                //
                if (checkCursor.blockFormat().boolProperty(
                        TextBlockStyle::PropertyIsBreakCorrectionStart)) {
                    checkCursor.movePosition(QTextCursor::NextBlock);
                    while (!checkCursor.atEnd()
                           && checkCursor.blockFormat().boolProperty(
                               TextBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::EndOfBlock);
                        checkCursor.movePosition(QTextCursor::NextCharacter);
                    }

                    topCursorPosition = checkCursor.position();
                    bottomCursorPosition = topCursorPosition + 1;
                }
                //
                // ... в смещении блоков, удаляем все декорации
                //
                else if (checkCursor.block().next().blockFormat().boolProperty(
                             TextBlockStyle::PropertyIsCorrection)) {
                    checkCursor.movePosition(QTextCursor::NextBlock);
                    while (!checkCursor.atEnd()
                           && checkCursor.blockFormat().boolProperty(
                               TextBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::EndOfBlock);
                        checkCursor.movePosition(QTextCursor::NextCharacter);
                    }

                    bottomCursorPosition = checkCursor.position();
                }
            }
            //
            // ... назад
            //
            else if (_backward && checkCursor.atBlockEnd()) {
                //
                // ... в разрыве, удаляем символы в оторванном абзаце
                //
                if (checkCursor.blockFormat().boolProperty(
                        TextBlockStyle::PropertyIsBreakCorrectionStart)
                    || checkCursor.block().next().blockFormat().boolProperty(
                        TextBlockStyle::PropertyIsBreakCorrectionEnd)) {
                    while (!checkCursor.atStart()
                           && checkCursor.blockFormat().boolProperty(
                               TextBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::PreviousBlock);
                    }
                    checkCursor.movePosition(QTextCursor::EndOfBlock);
                    checkCursor.movePosition(QTextCursor::PreviousCharacter);

                    topCursorPosition = checkCursor.position();
                    bottomCursorPosition = topCursorPosition + 1;
                }
                //
                // ... в смещении блоков, удаляем все декорации
                //
                else if (checkCursor.blockFormat().boolProperty(
                             TextBlockStyle::PropertyIsCorrection)) {
                    while (!checkCursor.atStart()
                           && checkCursor.blockFormat().boolProperty(
                               TextBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::PreviousBlock);
                    }
                    checkCursor.movePosition(QTextCursor::EndOfBlock);

                    topCursorPosition = checkCursor.position();
                }
            }
        }
    }

    //
    // Обрабатываем особые случаи
    //
    {
        //
        // ... когда в документе выделен весь текст, который находится внутри папки/папок
        //
        {
            auto topBlock = document()->findBlock(topCursorPosition);
            auto bottomBlock = document()->findBlock(bottomCursorPosition);
            if (((TextBlockStyle::forBlock(topBlock) == TextParagraphType::ActHeading
                  && TextBlockStyle::forBlock(bottomBlock) == TextParagraphType::ActFooter)
                 || (TextBlockStyle::forBlock(topBlock) == TextParagraphType::SequenceHeading
                     && TextBlockStyle::forBlock(bottomBlock) == TextParagraphType::SequenceFooter)
                 || (TextBlockStyle::forBlock(topBlock) == TextParagraphType::PartHeading
                     && TextBlockStyle::forBlock(bottomBlock) == TextParagraphType::PartFooter)
                 || (TextBlockStyle::forBlock(topBlock) == TextParagraphType::ChapterHeading
                     && TextBlockStyle::forBlock(bottomBlock) == TextParagraphType::ChapterFooter))
                && topBlock == document()->begin() && bottomBlock == document()->lastBlock()) {
                //
                // Нельзя просто взять и удалить весь текст, потому что тогда останется блок
                // окончания папки, с которым ничего нельзя сделать, поэтому действуем
                // последовательно:
                // - делаем первый блок временем и местом
                // - помещаем туда символ, чтобы при удалении, Qt оставил в блоке и данные и формат
                // - удаляем весь остальной контент и заодно вставленный на предыдущем шаге символ
                //
                cursor.movePosition(QTextCursor::Start);
                auto textDocument = static_cast<BusinessLayer::TextDocument*>(document());
                Q_ASSERT(textDocument);
                textDocument->setParagraphType(textTemplate().defaultParagraphType(), cursor);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                cursor.insertText(" ");
                cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                cursor.deleteChar();
                cursor.deletePreviousChar();
                return;
            }
        }

        //
        // ... нажатие делит и бекспейс на границах таблицы
        //
        if (!cursor.hasSelection()) {
            //
            // ... когда пользователь нажал делит в блоке идущем перед таблицей
            //
            {
                auto checkCursor = cursor;
                checkCursor.setPosition(topCursorPosition);
                if (!checkCursor.inTable()) {
                    const bool isTopBlockEmpty = checkCursor.block().text().isEmpty();
                    const auto topBlock = checkCursor.block();
                    checkCursor.setPosition(bottomCursorPosition);
                    if (topBlock.next() == checkCursor.block()
                        && TextBlockStyle::forBlock(checkCursor)
                            == TextParagraphType::PageSplitter) {
                        //
                        // ... если блок пуст, то удалим блок, пододвинув таблицу наверх
                        //
                        if (isTopBlockEmpty) {
                            cursor.setPosition(topCursorPosition);
                            cursor.movePosition(TextCursor::NextCharacter, TextCursor::KeepAnchor);
                            cursor.deleteChar();
                            cursor.movePosition(TextCursor::NextCharacter);
                            _editor->setTextCursor(cursor);
                        }
                        //
                        // ... в противном случае, игнорируем изменение
                        //

                        return;
                    }
                }
            }

            //
            // ... когда пользователь нажал бэкспейс в блоке идущем после таблицы
            //
            {
                auto checkCursor = cursor;
                checkCursor.setPosition(topCursorPosition);
                if (TextBlockStyle::forBlock(checkCursor) == TextParagraphType::PageSplitter) {
                    checkCursor.setPosition(bottomCursorPosition);
                    if (!checkCursor.inTable()) {
                        //
                        // ... если блок не пуст, то игнорируем нажатие
                        //
                        if (!checkCursor.block().text().isEmpty()) {
                            return;
                        }
                        //
                        // ... в противном случае, работаем по общему алгоритму
                        //
                    }
                }
            }
        }

        //
        // ... когда пользователь хочет удалить контент внутри одной таблицы
        //
        {
            auto checkCursor = cursor;
            checkCursor.setPosition(topCursorPosition);
            const auto topCursorInTable = checkCursor.inTable();
            const auto topCursorInFirstColumn = checkCursor.inFirstColumn();
            bool isTableEmpty = false;
            if (topCursorInTable) {
                isTableEmpty = checkCursor.isTableEmpty();
            }
            checkCursor.setPosition(bottomCursorPosition);
            const auto bottomCursorInTable = checkCursor.inTable();
            const auto bottomCursorInFirstColumn = checkCursor.inFirstColumn();
            if (bottomCursorInTable) {
                isTableEmpty = checkCursor.isTableEmpty();
            }
            if (topCursorInTable || bottomCursorInTable) {
                //
                // ... удаление полностью внутри таблицы
                //
                if (topCursorInTable && topCursorInFirstColumn && bottomCursorInTable
                    && !bottomCursorInFirstColumn) {
                    //
                    // ... если нет выделения, значит нажат делит в конце последней ячейки левой
                    //     колонки, или бекспейс в начале первой ячейки правой колонки
                    //
                    if (!cursor.hasSelection()) {
                        //
                        // ... удалим таблицу, если она пустая, или разрешено в шаблоне
                        //
                        if (isTableEmpty || textTemplate().canMergeParagraph()) {
                            auto textDocument
                                = static_cast<BusinessLayer::TextDocument*>(document());
                            textDocument->mergeParagraph(cursor);
                        }
                    }
                    //
                    // ... если есть выделение, удаляем таблицу к херам, т.к. выделена может быть
                    //     только вся таблица целиком
                    //
                    else {
                        //
                        // ... выделяем таблицу вместе с границами
                        //
                        const auto selectionInterval = cursor.selectionInterval();
                        cursor.setPosition(selectionInterval.from);
                        while (TextBlockStyle::forBlock(cursor)
                               != TextParagraphType::PageSplitter) {
                            cursor.movePosition(TextCursor::PreviousBlock);
                        }
                        do {
                            cursor.movePosition(TextCursor::NextBlock, TextCursor::KeepAnchor);
                        } while (TextBlockStyle::forBlock(cursor) != TextParagraphType::PageSplitter
                                 && cursor.position() < selectionInterval.to);
                        //
                        // ... удаляем таблицу
                        //
                        cursor.removeSelectedText();
                        cursor.deletePreviousChar();
                        //
                        // ... если в документе ничего не осталось, то применим дефолтный формат
                        //
                        if (cursor.document()->isEmpty()) {
                            auto textDocument
                                = qobject_cast<BusinessLayer::TextDocument*>(cursor.document());
                            textDocument->setParagraphType(textTemplate().defaultParagraphType(),
                                                           cursor);
                        }
                        //
                        // ... а если осталось, то вернём курсор на символ назад, если он попал в
                        // таблицу
                        //
                        else if (TextBlockStyle::forBlock(cursor)
                                 == TextParagraphType::PageSplitter) {
                            cursor.movePosition(TextCursor::PreviousBlock);
                            cursor.movePosition(TextCursor::EndOfBlock);
                            _editor->setTextCursor(cursor);
                        }
                    }
                    return;
                }
                //
                // ... при попытке удаления через границу таблицы
                //
                else if ((!topCursorInTable && bottomCursorInTable)
                         || (topCursorInTable && !bottomCursorInTable)) {
                    //
                    // ... если таблица пуста, то удаляем
                    //
                    if (isTableEmpty) {
                        cursor.setPosition(topCursorInTable ? topCursorPosition
                                                            : bottomCursorPosition);
                        //
                        // Идём до конца таблицы
                        //
                        while (!cursor.atEnd()
                               && TextBlockStyle::forBlock(cursor)
                                   != TextParagraphType::PageSplitter) {
                            cursor.movePosition(TextCursor::EndOfBlock);
                            cursor.movePosition(TextCursor::NextBlock);
                        }

                        //
                        // Выделяем таблицу
                        //
                        cursor.movePosition(TextCursor::PreviousBlock, TextCursor::KeepAnchor);
                        //
                        // ... и заходим в конец предыдущего блока
                        //
                        cursor.movePosition(TextCursor::PreviousCharacter, TextCursor::KeepAnchor);

                        //
                        // Удаляем таблицу
                        //
                        cursor.removeSelectedText();

                        //
                        // Если попали в конец предыдущей таблицы, то зайдём в неё
                        //
                        if (TextBlockStyle::forBlock(cursor) == TextParagraphType::PageSplitter) {
                            cursor.movePosition(TextCursor::PreviousCharacter);
                            _editor->setTextCursor(cursor);
                        }
                    }
                    //
                    // ... если не пуста, то оставляем как есть
                    //
                    else {
                        //
                        // ... лишь сдвинув курсор по направлению удаления
                        //     добавляем смещение, чтобы покинуть блок разделителя страницы
                        //
                        int delta = 1;
                        do {
                            cursor.setPosition(_backward ? topCursorPosition - delta
                                                         : bottomCursorPosition + delta);
                            ++delta;
                        } while (TextBlockStyle::forBlock(cursor)
                                 == TextParagraphType::PageSplitter);
                        _editor->setTextCursor(cursor);
                    }

                    return;
                }
                //
                // ... во всех остальных случаях работаем по общей схеме
                //
            }
        }

        //
        // ... когда пользователь хочет удалить несколько таблиц
        //
        if (cursor.hasSelection()) {
            auto checkCursor = cursor;
            checkCursor.setPosition(topCursorPosition);
            const auto topCursorInPageSplitter
                = TextBlockStyle::forBlock(checkCursor) == TextParagraphType::PageSplitter;
            checkCursor.setPosition(bottomCursorPosition);
            const auto bottomCursorInPageSplitter
                = TextBlockStyle::forBlock(checkCursor) == TextParagraphType::PageSplitter;
            if (topCursorInPageSplitter && bottomCursorInPageSplitter) {
                //
                // ... выделяем таблицу вместе с границами
                //
                const auto selectionInterval = cursor.selectionInterval();
                cursor.setPosition(selectionInterval.from);
                while (TextBlockStyle::forBlock(cursor) != TextParagraphType::PageSplitter) {
                    cursor.movePosition(TextCursor::PreviousBlock);
                }
                do {
                    cursor.movePosition(TextCursor::NextBlock, TextCursor::KeepAnchor);
                } while (TextBlockStyle::forBlock(cursor) != TextParagraphType::PageSplitter
                         && cursor.position() < selectionInterval.to);
                //
                // ... удаляем таблицу
                //
                cursor.removeSelectedText();
                cursor.deletePreviousChar();
                //
                // ... если в документе ничего не осталось, то применим дефолтный формат
                //
                if (cursor.document()->isEmpty()) {
                    auto textDocument
                        = qobject_cast<BusinessLayer::TextDocument*>(cursor.document());
                    textDocument->setParagraphType(textTemplate().defaultParagraphType(), cursor);
                }
                //
                // ... а если осталось, то вернём курсор на символ назад, если он попал в
                // таблицу
                //
                else if (TextBlockStyle::forBlock(cursor) == TextParagraphType::PageSplitter) {
                    cursor.movePosition(TextCursor::PreviousBlock);
                    cursor.movePosition(TextCursor::EndOfBlock);
                    _editor->setTextCursor(cursor);
                }
                return;
            }
        }
    }

    //
    // Получим стили блоков начала и конца выделения
    //
    // ... начала
    //
    const auto topBlock = document()->findBlock(topCursorPosition);
    const auto topParagraphType = TextBlockStyle::forBlock(topBlock);
    const auto topStyle = textTemplate().paragraphStyle(topParagraphType);
    //
    // ... и конца
    //
    const auto bottomBlock = document()->findBlock(bottomCursorPosition);
    const auto bottomParagraphType = TextBlockStyle::forBlock(bottomBlock);
    const auto bottomStyle = textTemplate().paragraphStyle(bottomParagraphType);

    //
    // Определим стиль результирующего блока и сохраним его данные
    //
    TextBlockStyle targetStyle;
    TextBlockData* targetBlockData = nullptr;
    bool isTopBlockShouldBeRemoved = false;
    //
    // Если пользователь хочет удалить пустую папку, расширим выделение, чтобы полностью её удалить
    //
    if (((topParagraphType == TextParagraphType::ActHeading
          && bottomParagraphType == TextParagraphType::ActFooter)
         || (topParagraphType == TextParagraphType::SequenceHeading
             && bottomParagraphType == TextParagraphType::SequenceFooter)
         || (topParagraphType == TextParagraphType::PartHeading
             && bottomParagraphType == TextParagraphType::PartFooter)
         || (topParagraphType == TextParagraphType::ChapterHeading
             && bottomParagraphType == TextParagraphType::ChapterFooter))
        && topBlock.next() == bottomBlock) {
        if (bottomBlock.next() == document()->end()) {
            cursor.setPosition(topBlock.position());
            cursor.movePosition(QTextCursor::PreviousBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
            topCursorPosition = cursor.position();
        } else {
            cursor.setPosition(bottomBlock.position());
            cursor.movePosition(QTextCursor::NextBlock);
            bottomCursorPosition = cursor.position();
            isTopBlockShouldBeRemoved = true;
        }
        const QTextBlock targetBlock = cursor.block();
        const auto targetBlockType = TextBlockStyle::forBlock(targetBlock);
        targetStyle = textTemplate().paragraphStyle(targetBlockType);
        targetBlockData = cloneBlockData(targetBlock);
    }
    //
    // Если оба блока пусты и ни один из них не разрыв и не декорация
    //
    else if (topBlock.text().isEmpty() && bottomBlock.text().isEmpty()
             && topParagraphType != TextParagraphType::PageSplitter
             && bottomParagraphType != TextParagraphType::PageSplitter
             && !topBlock.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
             && !bottomBlock.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)) {
        //
        // ... результирующим будет более "старший"
        //
        if (topParagraphType < bottomParagraphType) {
            targetStyle = topStyle;
            targetBlockData = cloneBlockData(topBlock);
        } else {
            targetStyle = bottomStyle;
            targetBlockData = cloneBlockData(bottomBlock);
            isTopBlockShouldBeRemoved = true;
        }
    }
    //
    // Если в верхнем блоке нет текста, а в нижнем есть
    // Или верхний блок полностью удаляется и при этом не является разрывом блока,
    //     а нижний не является декорацией
    //
    else if ((topBlock.text().isEmpty() && !bottomBlock.text().isEmpty())
             || (topBlock.position() == topCursorPosition
                 && topBlock.position() + topBlock.text().length() < bottomCursorPosition
                 && topParagraphType != TextParagraphType::PageSplitter
                 && !bottomBlock.blockFormat().boolProperty(
                     TextBlockStyle::PropertyIsCorrection))) {
        //
        // ... то результирующим стилем будет стиль нижнего блока
        //
        targetStyle = bottomStyle;
        targetBlockData = cloneBlockData(bottomBlock);
        isTopBlockShouldBeRemoved = true;
    }
    //
    // Во всех остальных случаях, результирующим стилем будет стиль верхнего
    //
    else {
        targetStyle = topStyle;
        targetBlockData = cloneBlockData(topBlock);
    }

    //    //
    //    // Если при удалении целевым был назначен стиль разделения, то сбросим его до стандартного
    //    //
    //    if (targetStyle.type() == TextParagraphType::PageSplitter) {
    //        targetStyle = textTemplate().paragraphStyle(textTemplate().defaultParagraphType());
    //        delete targetBlockData;
    //        targetBlockData = nullptr;
    //    }

    //
    // Собственно удаление
    //
    cursor.beginEditBlock();
    {
        FoldersToDelete foldersToDelete;

        //
        // Подсчитать количество группирующих элементов входящих в выделение
        //
        const bool needToDeleteGroups = topBlock != bottomBlock;
        if (needToDeleteGroups) {
            foldersToDelete = findFoldersToDelete(topCursorPosition, bottomCursorPosition,
                                                  isTopBlockShouldBeRemoved);
        }
        //
        // Определим происходит ли изменение внутри одного блока
        //
        const bool inblockChange = topBlock == bottomBlock;

        //
        // Удалить текст
        //
        cursor.setPosition(topCursorPosition);
        cursor.setPosition(bottomCursorPosition, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        //
        // Положим корректные данные в блок
        //
        if (!inblockChange) {
            cursor.setBlockFormat(targetStyle.blockFormat(cursor.inTable()));
            cursor.setBlockCharFormat(targetStyle.charFormat());
            cursor.block().setUserData(targetBlockData);
        }
        //
        // Если изменение происходит в одном блоке, то клон данных блока нам не нужен
        //
        else {
            delete targetBlockData;
        }

        //
        // Удалить вторые половинки группирующих элементов
        //
        if (needToDeleteGroups) {
            removeGroupsPairs(cursor.position(), foldersToDelete, isTopBlockShouldBeRemoved);
        }
    }

    //
    // Завершим операцию удаления
    //
    cursor.endEditBlock();

    //
    // Если после удаления курсор встал в невидимый блок, двигем его красиво
    //
    cursor = _editor->textCursor();
    if (!cursor.block().isVisible()) {
        //
        // Для удаления назад ничего не делаем, кьют сам всё позиционирует нормально
        //
        if (_backward) {
        }
        //
        // Для удаления вперёд двигаем курсор к следующему блоку
        //
        else {
            while (!cursor.atEnd() && !cursor.block().isVisible()) {
                cursor.movePosition(QTextCursor::EndOfBlock);
                cursor.movePosition(QTextCursor::NextBlock);
            }
        }
    }

    //
    // Если после удаления курсор встал в блок, в котором запрещено позиционирование курсора,
    // корректируем его позицию
    //
    if (cursor.blockFormat().boolProperty(PageTextEdit::PropertyDontShowCursor)) {
        //
        // Т.к. это актуально только для таблиц, корректируем лишь на один абзац, чтобы не городить
        // сложную логику по охвату всех видимых и невидимых кейсов
        //
        cursor.movePosition(cursor.atStart() ? TextCursor::NextCharacter
                                             : TextCursor::PreviousCharacter);
    }

    _editor->setTextCursor(cursor);
}

TextCursor::FoldersToDelete TextCursor::findFoldersToDelete(int _topCursorPosition,
                                                            int _bottomCursorPosition,
                                                            bool isTopBlockShouldBeRemoved)
{
    //
    // Начнём поиск с заданной позиции
    //
    TextCursor cursor(document());
    cursor.setPosition(_topCursorPosition);

    //
    // Если первый блок не будет удалён, то пропускаем его, т.к. он остаётся с текущим стилем
    //
    if (!isTopBlockShouldBeRemoved) {
        cursor.movePosition(QTextCursor::NextBlock);
    }

    FoldersToDelete foldersToDelete;
    while (cursor.position() <= _bottomCursorPosition) {
        //
        // Если последний блок не будет удалён, то пропускаем его, т.к. он останется с текущим
        // стилем
        //
        if (cursor.block().position() <= _bottomCursorPosition
            && cursor.block().position() + cursor.block().text().length() >= _bottomCursorPosition
            && isTopBlockShouldBeRemoved) {
            break;
        }

        //
        // Определим тип блока
        //
        const auto currentType = TextBlockStyle::forBlock(cursor.block());

        //
        // Если найден блок открывающий папку, то нужно удалить закрывающий блок
        //
        if (currentType == TextParagraphType::ActHeading
            || currentType == TextParagraphType::SequenceHeading
            || currentType == TextParagraphType::PartHeading
            || currentType == TextParagraphType::ChapterHeading) {
            //
            // ... если все группы закрыты, нужно удалить последующую закрытую
            //
            if (foldersToDelete.headers == 0) {
                ++foldersToDelete.footers;
            }
            //
            // ... в противном случае закрываем открытую группу
            //
            else {
                --foldersToDelete.headers;
            }
        }
        //
        // Если найден блок закрывающий папку
        //
        else if (currentType == TextParagraphType::ActFooter
                 || currentType == TextParagraphType::SequenceFooter
                 || currentType == TextParagraphType::PartFooter
                 || currentType == TextParagraphType::ChapterFooter) {
            //
            // ... если все группы закрыты, нужно удалить предыдущую открытую
            //
            if (foldersToDelete.footers == 0) {
                ++foldersToDelete.headers;
            }
            //
            // ... в противном случае закрываем открытую группу
            //
            else {
                --foldersToDelete.footers;
            }
        }

        //
        // Если дошли до конца, прерываем выполнение
        //
        if (cursor.atEnd()) {
            break;
        }

        //
        // Перейдём к следующему блоку или концу блока
        //
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::NextBlock);
    }
    return foldersToDelete;
}

void TextCursor::removeGroupsPairs(int _cursorPosition,
                                   const TextCursor::FoldersToDelete& _foldersToDelete,
                                   bool isTopBlockShouldBeRemoved)
{
    //
    // Удалим пары из последующего текста
    //
    if (_foldersToDelete.footers > 0) {
        TextCursor cursor(document());
        cursor.setPosition(_cursorPosition);

        //
        // Если первый блок не будет удалён, то пропускаем его, т.к. он остаётся с текущим стилем
        //
        if (const auto cursorParagraphType = TextBlockStyle::forBlock(cursor);
            !isTopBlockShouldBeRemoved
            && (cursorParagraphType == TextParagraphType::ActFooter
                || cursorParagraphType == TextParagraphType::SequenceFooter
                || cursorParagraphType == TextParagraphType::PartFooter
                || cursorParagraphType == TextParagraphType::ChapterFooter)) {
            cursor.movePosition(QTextCursor::NextBlock);
        }

        //
        // ... открытые группы на пути поиска необходимого для удаления блока
        //
        int openedGroups = 0;
        int groupsToDeleteCount = _foldersToDelete.footers;
        do {
            const auto currentType = TextBlockStyle::forBlock(cursor.block());
            if (currentType == TextParagraphType::ActFooter
                || currentType == TextParagraphType::SequenceFooter
                || currentType == TextParagraphType::PartFooter
                || currentType == TextParagraphType::ChapterFooter) {
                if (openedGroups == 0) {
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    if (cursor.hasSelection()) {
                        cursor.deleteChar();
                    }

                    //
                    // Уберём сам блок
                    //
                    TextBlockData* blockData = nullptr;
                    if (cursor.atEnd()) {
                        blockData = cloneBlockData(cursor.block().previous());
                        cursor.deletePreviousChar();
                    } else {
                        blockData = cloneBlockData(cursor.block().next());
                        cursor.deleteChar();
                    }
                    cursor.block().setUserData(blockData);

                    --groupsToDeleteCount;

                    continue;
                } else {
                    --openedGroups;
                }
            } else if (currentType == TextParagraphType::ActHeading
                       || currentType == TextParagraphType::SequenceHeading
                       || currentType == TextParagraphType::PartHeading
                       || currentType == TextParagraphType::ChapterHeading) {
                //
                // ... встретилась новая группа, которую не нужно удалять
                //
                ++openedGroups;
            }

            if (cursor.atEnd()) {
                Q_ASSERT(false);
                break;
            }

            cursor.movePosition(QTextCursor::NextBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
        } while (groupsToDeleteCount > 0);
    }

    //
    // Удалим пары из предшествующего текста
    //
    if (_foldersToDelete.headers > 0) {
        TextCursor cursor(document());
        cursor.setPosition(_cursorPosition);

        //
        // Если первый блок не будет удалён, то пропускаем его, т.к. он остаётся с текущим стилем
        //
        if (const auto cursorParagraphType = TextBlockStyle::forBlock(cursor);
            !isTopBlockShouldBeRemoved
            && (cursorParagraphType == TextParagraphType::ActHeading
                || cursorParagraphType == TextParagraphType::SequenceHeading
                || cursorParagraphType == TextParagraphType::PartHeading
                || cursorParagraphType == TextParagraphType::ChapterHeading)) {
            cursor.movePosition(QTextCursor::PreviousBlock);
        }

        //
        // ... открытые группы на пути поиска необходимого для удаления блока
        //
        int openedGroups = 0;
        int groupsToDeleteCount = _foldersToDelete.headers;
        do {
            const auto currentType = TextBlockStyle::forBlock(cursor.block());
            if (currentType == TextParagraphType::ActHeading
                || currentType == TextParagraphType::SequenceHeading
                || currentType == TextParagraphType::PartHeading
                || currentType == TextParagraphType::ChapterHeading) {
                if (openedGroups == 0) {
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    if (cursor.hasSelection()) {
                        cursor.deleteChar();
                    }

                    //
                    // Удалим сам блок
                    //
                    TextBlockData* blockData = nullptr;
                    if (cursor.atEnd()) {
                        blockData = cloneBlockData(cursor.block().previous());
                        cursor.deletePreviousChar();
                    } else {
                        blockData = cloneBlockData(cursor.block().next());
                        cursor.deleteChar();
                    }
                    cursor.block().setUserData(blockData);

                    --groupsToDeleteCount;

                    continue;
                } else {
                    --openedGroups;
                }
            } else if (currentType == TextParagraphType::ActFooter
                       || currentType == TextParagraphType::SequenceFooter
                       || currentType == TextParagraphType::PartFooter
                       || currentType == TextParagraphType::ChapterFooter) {
                //
                // ... встретилась новая группа, которую не нужно удалять
                //
                ++openedGroups;
            }

            if (cursor.atStart()) {
                break;
            }

            cursor.movePosition(QTextCursor::PreviousBlock);
        } while (groupsToDeleteCount > 0);
    }
}

const TextTemplate& TextCursor::textTemplate() const
{
    if (auto textDocument = qobject_cast<TextDocument*>(document()); textDocument != nullptr) {
        return TemplatesFacade::textTemplate(textDocument->model());
    }

    return TemplatesFacade::simpleTextTemplate();
}

} // namespace BusinessLayer
