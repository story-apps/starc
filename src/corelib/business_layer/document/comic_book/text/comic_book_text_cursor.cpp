#include "comic_book_text_cursor.h"

#include "comic_book_text_block_data.h"
#include "comic_book_text_document.h"

#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/base/base_text_edit.h>
#include <utils/shugar.h>

#include <QTextTable>
#include <QtGui/private/qtextdocument_p.h>


namespace BusinessLayer {

namespace {
ComicBookTextBlockData* cloneBlockData(const QTextBlock& _block)
{
    ComicBookTextBlockData* clonedBlockData = nullptr;
    if (_block.userData() != nullptr) {
        const auto blockData
            = static_cast<BusinessLayer::ComicBookTextBlockData*>(_block.userData());
        if (blockData != nullptr) {
            clonedBlockData = new ComicBookTextBlockData(blockData);
        }
    }
    return clonedBlockData;
}
} // namespace


ComicBookTextCursor::ComicBookTextCursor()
    : QTextCursor()
{
}

ComicBookTextCursor::ComicBookTextCursor(const QTextCursor& _other)
    : QTextCursor(_other)
{
}

ComicBookTextCursor::ComicBookTextCursor(QTextDocument* _document)
    : QTextCursor(_document)
{
}

ComicBookTextCursor::~ComicBookTextCursor() = default;

bool ComicBookTextCursor::isInEditBlock() const
{
#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
    return QTextDocumentPrivate::get(document())->isInEditBlock();
#else
    return document()->docHandle()->isInEditBlock();
#endif
}

bool ComicBookTextCursor::inTable() const
{
    return currentTable() != nullptr;
}

bool ComicBookTextCursor::inFirstColumn() const
{
    return currentTable() && currentTable()->cellAt(*this).column() == 0;
}

ComicBookTextCursor::Selection ComicBookTextCursor::selectionInterval() const
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

void ComicBookTextCursor::restartEditBlock()
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

void ComicBookTextCursor::removeCharacters(BaseTextEdit* _editor)
{
    removeCharacters(true, _editor);
}

//
// TODO: В нижеследующих методах сделать аккуратно через собственный курсор
//

void ComicBookTextCursor::removeCharacters(bool _backward, BaseTextEdit* _editor)
{
    Q_ASSERT(document() == _editor->document());

    ComicBookTextCursor cursor = _editor->textCursor();
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

            QTextCursor checkCursor = cursor;
            checkCursor.setPosition(topCursorPosition);
            //
            // ... переходим через корректирующие блоки блоки вперёд
            //
            if (!_backward && checkCursor.atBlockEnd()) {
                //
                // ... в разрыве, удаляем символы в оторванном абзаце
                //
                if (checkCursor.blockFormat().boolProperty(
                        ComicBookBlockStyle::PropertyIsBreakCorrectionStart)) {
                    checkCursor.movePosition(QTextCursor::NextBlock);
                    while (!checkCursor.atEnd()
                           && checkCursor.blockFormat().boolProperty(
                               ComicBookBlockStyle::PropertyIsCorrection)) {
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
                             ComicBookBlockStyle::PropertyIsCorrection)) {
                    checkCursor.movePosition(QTextCursor::NextBlock);
                    while (!checkCursor.atEnd()
                           && checkCursor.blockFormat().boolProperty(
                               ComicBookBlockStyle::PropertyIsCorrection)) {
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
                        ComicBookBlockStyle::PropertyIsBreakCorrectionStart)
                    || checkCursor.block().next().blockFormat().boolProperty(
                        ComicBookBlockStyle::PropertyIsBreakCorrectionEnd)) {
                    while (!checkCursor.atStart()
                           && checkCursor.blockFormat().boolProperty(
                               ComicBookBlockStyle::PropertyIsCorrection)) {
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
                             ComicBookBlockStyle::PropertyIsCorrection)) {
                    while (!checkCursor.atStart()
                           && checkCursor.blockFormat().boolProperty(
                               ComicBookBlockStyle::PropertyIsCorrection)) {
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
            if (ComicBookBlockStyle::forBlock(topBlock) == ComicBookParagraphType::FolderHeader
                && ComicBookBlockStyle::forBlock(bottomBlock)
                    == ComicBookParagraphType::FolderFooter
                && topBlock == document()->begin() && bottomBlock.next() == document()->end()) {
                //
                // Нельзя просто взять и удалить весь текст, потому что тогда останется блок
                // окончания папки, с которым ничего нельзя сделать, поэтому действуем
                // последовательно:
                // - делаем первый блок временем и местом
                // - помещаем туда символ, чтобы при удалении, Qt оставил в блоке и данные и формат
                // - удаляем весь остальной контент и заодно вставленный на предыдущем шаге символ
                //
                cursor.movePosition(QTextCursor::Start);
                auto comicBookDocument
                    = dynamic_cast<BusinessLayer::ComicBookTextDocument*>(document());
                Q_ASSERT(comicBookDocument);
                comicBookDocument->setParagraphType(ComicBookParagraphType::Page, cursor);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                cursor.insertText(" ");
                cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                cursor.deleteChar();
                cursor.deletePreviousChar();
                return;
            }
        }

        //
        // ... когда пользователь хочет удалить внутреннюю границу разделения
        //
        {
            ComicBookTextCursor checkCursor = cursor;
            checkCursor.setPosition(topCursorPosition);
            if (checkCursor.inTable() && checkCursor.inFirstColumn()) {
                checkCursor.setPosition(bottomCursorPosition);
                if (checkCursor.inTable() && !checkCursor.inFirstColumn()) {
                    //
                    // ... если нет выделения, значит нажат делит в конце последней ячейки левой
                    //     колонки, или бекспейс в начале первой ячейки правой колонки - удалим
                    //     таблицу
                    //
                    if (!cursor.hasSelection()) {
                        auto comicBookDocument
                            = dynamic_cast<BusinessLayer::ComicBookTextDocument*>(document());
                        comicBookDocument->mergeParagraph(cursor);
                        return;
                    }
                    //
                    // ... в остальных случаях ничего не делаем
                    //
                    else {
                        return;
                    }
                }
            }
        }

        //
        // ... запрещаем удалять пустую строку идущую после таблицы
        //
        if (topCursorPosition + 1 == bottomCursorPosition) {
            ComicBookTextCursor checkCursor = cursor;
            checkCursor.setPosition(bottomCursorPosition);
            if (checkCursor.atEnd()) {
                checkCursor.setPosition(topCursorPosition);
                if (ComicBookBlockStyle::forBlock(checkCursor.block())
                    == ComicBookParagraphType::PageSplitter) {
                    return;
                }
            }
        }
    }

    //
    // Получим стили блоков начала и конца выделения
    //
    // ... начала
    //
    const auto topBlock = document()->findBlock(topCursorPosition);
    const auto topParagraphType = ComicBookBlockStyle::forBlock(topBlock);
    const auto topStyle = TemplatesFacade::comicBookTemplate().paragraphStyle(topParagraphType);
    //
    // ... и конца
    //
    const auto bottomBlock = document()->findBlock(bottomCursorPosition);
    const auto bottomParagraphType = ComicBookBlockStyle::forBlock(bottomBlock);
    const auto bottomStyle
        = TemplatesFacade::comicBookTemplate().paragraphStyle(bottomParagraphType);

    //
    // Определим стиль результирующего блока и сохраним его данные
    //
    ComicBookBlockStyle targetStyle;
    ComicBookTextBlockData* targetBlockData = nullptr;
    bool isTopBlockShouldBeRemoved = false;
    //
    // Если пользователь хочет удалить пустую папку, расширим выделение, чтобы полностью её удалить
    //
    if (topParagraphType == ComicBookParagraphType::FolderHeader
        && bottomParagraphType == ComicBookParagraphType::FolderFooter
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
        const auto targetBlockType = ComicBookBlockStyle::forBlock(targetBlock);
        targetStyle = TemplatesFacade::comicBookTemplate().paragraphStyle(targetBlockType);
        targetBlockData = cloneBlockData(targetBlock);
    }
    //
    // Если в верхнем блоке нет текста, а в нижнем есть
    // Или верхний блок полностью удаляется, а нижний не полностью
    //
    else if ((topBlock.text().isEmpty() && !bottomBlock.text().isEmpty())
             || (topBlock.position() == topCursorPosition
                 && topBlock.position() + topBlock.text().length() < bottomCursorPosition
                 && bottomBlock.position() + bottomBlock.text().length() > bottomCursorPosition)) {
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
            cursor.block().setUserData(targetBlockData);
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
}

ComicBookTextCursor::FoldersToDelete ComicBookTextCursor::findFoldersToDelete(
    int _topCursorPosition, int _bottomCursorPosition, bool isTopBlockShouldBeRemoved)
{
    //
    // Начнём поиск с заданной позиции
    //
    QTextCursor cursor(document());
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
        const auto currentType = ComicBookBlockStyle::forBlock(cursor.block());

        //
        // Если найден блок открывающий папку, то нужно удалить закрывающий блок
        //
        if (currentType == ComicBookParagraphType::FolderHeader) {
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
        else if (currentType == ComicBookParagraphType::FolderFooter) {
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

void ComicBookTextCursor::removeGroupsPairs(
    int _cursorPosition, const ComicBookTextCursor::FoldersToDelete& _foldersToDelete,
    bool isTopBlockShouldBeRemoved)
{
    //
    // Удалим пары из последующего текста
    //
    if (_foldersToDelete.footers > 0) {
        QTextCursor cursor(document());
        cursor.setPosition(_cursorPosition);

        //
        // Если первый блок не будет удалён, то пропускаем его, т.к. он остаётся с текущим стилем
        //
        if (!isTopBlockShouldBeRemoved) {
            cursor.movePosition(QTextCursor::NextBlock);
        }

        //
        // ... открытые группы на пути поиска необходимого для удаления блока
        //
        int openedGroups = 0;
        int groupsToDeleteCount = _foldersToDelete.footers;
        do {
            const auto currentType = ComicBookBlockStyle::forBlock(cursor.block());
            if (currentType == ComicBookParagraphType::FolderFooter) {
                if (openedGroups == 0) {
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    if (cursor.hasSelection()) {
                        cursor.deleteChar();
                    }

                    //
                    // Уберём сам блок
                    //
                    ComicBookTextBlockData* blockData = nullptr;
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
            } else if (currentType == ComicBookParagraphType::FolderHeader) {
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
        QTextCursor cursor(document());
        cursor.setPosition(_cursorPosition);

        //
        // Если первый блок не будет удалён, то пропускаем его, т.к. он остаётся с текущим стилем
        //
        if (!isTopBlockShouldBeRemoved) {
            cursor.movePosition(QTextCursor::NextBlock);
        }

        //
        // ... открытые группы на пути поиска необходимого для удаления блока
        //
        int openedGroups = 0;
        int groupsToDeleteCount = _foldersToDelete.headers;
        do {
            const auto currentType = ComicBookBlockStyle::forBlock(cursor.block());
            if (currentType == ComicBookParagraphType::FolderHeader) {
                if (openedGroups == 0) {
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    if (cursor.hasSelection()) {
                        cursor.deleteChar();
                    }

                    //
                    // Удалим сам блок
                    //
                    auto blockData = cloneBlockData(cursor.block().next());
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
            } else if (currentType == ComicBookParagraphType::FolderFooter) {
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

} // namespace BusinessLayer
