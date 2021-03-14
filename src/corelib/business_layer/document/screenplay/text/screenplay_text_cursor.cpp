#include "screenplay_text_cursor.h"

#include "screenplay_text_block_data.h"
#include "screenplay_text_document.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <ui/widgets/text_edit/base/base_text_edit.h>

#include <utils/shugar.h>

#include <QTextTable>


namespace BusinessLayer {

namespace {
ScreenplayTextBlockData* cloneBlockData(const QTextBlock& _block) {
    ScreenplayTextBlockData* clonedBlockData = nullptr;
    if (_block.userData() != nullptr) {
        const auto blockData = static_cast<BusinessLayer::ScreenplayTextBlockData*>(_block.userData());
        if (blockData != nullptr) {
            clonedBlockData = new ScreenplayTextBlockData(blockData);
        }
    }
    return clonedBlockData;
}
} // namespace


ScreenplayTextCursor::ScreenplayTextCursor()
    : QTextCursor()
{
}

ScreenplayTextCursor::ScreenplayTextCursor(const QTextCursor& _other)
    : QTextCursor(_other)
{
}

ScreenplayTextCursor::ScreenplayTextCursor(QTextDocument* _document)
    : QTextCursor(_document)
{
}

ScreenplayTextCursor::~ScreenplayTextCursor()
{
}

bool ScreenplayTextCursor::inTable() const
{
    return currentTable() != nullptr;
}

bool ScreenplayTextCursor::inFirstColumn() const
{
    return currentTable() && currentTable()->cellAt(*this).column() == 0;
}

ScreenplayTextCursor::Selection ScreenplayTextCursor::selectionInterval() const
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

void ScreenplayTextCursor::removeCharacters(BaseTextEdit* _editor)
{
    removeCharacters(true, _editor);
}

//
// TODO: В нижеследующих методах сделать аккуратно через собственный курсор
//

void ScreenplayTextCursor::removeCharacters(bool _backward, BaseTextEdit* _editor)
{
    Q_ASSERT(document() == _editor->document());

    ScreenplayTextCursor cursor = _editor->textCursor();
    if (!cursor.hasSelection()) {
        //
        // Если в начале документа нажат backspace
        //
        if (cursor.atStart()
            && _backward == true) {
            return;
        }

        //
        // Если в конце документа нажат delete
        //
        if (cursor.atEnd()
            && _backward == false) {
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
            if (!_backward
                && checkCursor.atBlockEnd()) {
                //
                // ... в разрыве, удаляем символы в оторванном абзаце
                //
                if (checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart)) {
                    checkCursor.movePosition(QTextCursor::NextBlock);
                    while (!checkCursor.atEnd()
                           && checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::EndOfBlock);
                        checkCursor.movePosition(QTextCursor::NextCharacter);
                    }

                    topCursorPosition = checkCursor.position();
                    bottomCursorPosition = topCursorPosition + 1;
                }
                //
                // ... в смещении блоков, удаляем все декорации
                //
                else if (checkCursor.block().next().blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
                    checkCursor.movePosition(QTextCursor::NextBlock);
                    while (!checkCursor.atEnd()
                           && checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::EndOfBlock);
                        checkCursor.movePosition(QTextCursor::NextCharacter);
                    }

                    bottomCursorPosition = checkCursor.position();
                }
            }
            //
            // ... назад
            //
            else if (_backward
                     && checkCursor.atBlockEnd()) {
                //
                // ... в разрыве, удаляем символы в оторванном абзаце
                //
                if (checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart)
                    || checkCursor.block().next().blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionEnd)) {
                    while (!checkCursor.atStart()
                           && checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
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
                else if (checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
                    while (!checkCursor.atStart()
                           && checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
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
        auto topBlock = document()->findBlock(topCursorPosition);
        auto bottomBlock = document()->findBlock(bottomCursorPosition);
        if (ScreenplayBlockStyle::forBlock(topBlock) == ScreenplayParagraphType::FolderHeader
            && ScreenplayBlockStyle::forBlock(bottomBlock) == ScreenplayParagraphType::FolderFooter
            && topBlock == document()->begin()
            && bottomBlock.next() == document()->end()) {
            //
            // Нельзя просто взять и удалить весь текст, потому что тогда останется блок
            // окончания папки, с которым ничего нельзя сделать, поэтому действуем последовательно:
            // - делаем первый блок временем и местом
            // - помещаем туда символ, чтобы при удалении, Qt оставил в блоке и данные и формат
            // - удаляем весь остальной контент и заодно вставленный на предыдущем шаге символ
            //
            cursor.movePosition(QTextCursor::Start);
            auto screenplayDocument = dynamic_cast<BusinessLayer::ScreenplayTextDocument*>(document());
            Q_ASSERT(screenplayDocument);
            screenplayDocument->setParagraphType(ScreenplayParagraphType::SceneHeading, cursor);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            cursor.insertText(" ");
            cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            cursor.deleteChar();
            cursor.deletePreviousChar();
            return;
        }

        //
        // ... когда пользователь хочет удалить внутреннюю границу разделения
        //
        ScreenplayTextCursor checkCursor = cursor;
        checkCursor.setPosition(topCursorPosition);
        if (checkCursor.inTable() && checkCursor.inFirstColumn()) {
            checkCursor.setPosition(bottomCursorPosition);
            if (checkCursor.inTable() && !checkCursor.inFirstColumn()) {
                //
                // ... если нет выделения, значит нажат делит в конце последней ячейки левой
                //     колонки, или бекспейс в начале первой ячейки правой колонки - удалим таблицу
                //
                if (!cursor.hasSelection()) {
                    auto screenplayDocument = dynamic_cast<BusinessLayer::ScreenplayTextDocument*>(document());
                    screenplayDocument->mergeParagraph(cursor);
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
    // Получим стили блоков начала и конца выделения
    //
    // ... начала
    //
    const auto topBlock = document()->findBlock(topCursorPosition);
    const auto topParagraphType = ScreenplayBlockStyle::forBlock(topBlock);
    const auto topStyle = ScreenplayTemplateFacade::getTemplate().blockStyle(topParagraphType);
    //
    // ... и конца
    //
    const auto bottomBlock = document()->findBlock(bottomCursorPosition);
    const auto bottomParagraphType = ScreenplayBlockStyle::forBlock(bottomBlock);
    const auto bottomStyle = ScreenplayTemplateFacade::getTemplate().blockStyle(bottomParagraphType);

    //
    // Определим стиль результирующего блока и сохраним его данные
    //
    ScreenplayBlockStyle targetStyle;
    ScreenplayTextBlockData* targetBlockData = nullptr;
    bool isTopBlockShouldBeRemoved = false;
    //
    // Если пользователь хочет удалить пустую папку, расширим выделение, чтобы полностью её удалить
    //
    if (topParagraphType == ScreenplayParagraphType::FolderHeader
        && bottomParagraphType == ScreenplayParagraphType::FolderFooter
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
        const auto targetBlockType = ScreenplayBlockStyle::forBlock(targetBlock);
        targetStyle = ScreenplayTemplateFacade::getTemplate().blockStyle(targetBlockType);
        targetBlockData = cloneBlockData(targetBlock);
    }
    //
    // Если в верхнем блоке нет текста, а в нижнем есть
    // Или верхний блок полностью удаляется, а нижний не полностью
    //
    else if ((topBlock.text().isEmpty()
              && !bottomBlock.text().isEmpty())
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
            cursor.setBlockFormat(targetStyle.blockFormat());
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

ScreenplayTextCursor::FoldersToDelete ScreenplayTextCursor::findFoldersToDelete(
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
        // Если последний блок не будет удалён, то пропускаем его, т.к. он останется с текущим стилем
        //
        if (cursor.block().position() <= _bottomCursorPosition
            && cursor.block().position() + cursor.block().text().length() >= _bottomCursorPosition
            && isTopBlockShouldBeRemoved) {
            break;
        }

        //
        // Определим тип блока
        //
        const auto currentType = ScreenplayBlockStyle::forBlock(cursor.block());

        //
        // Если найден блок открывающий папку, то нужно удалить закрывающий блок
        //
        if (currentType == ScreenplayParagraphType::FolderHeader) {
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
        else if (currentType == ScreenplayParagraphType::FolderFooter) {
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

void ScreenplayTextCursor::removeGroupsPairs(int _cursorPosition,
    const ScreenplayTextCursor::FoldersToDelete& _foldersToDelete, bool isTopBlockShouldBeRemoved)
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
            const auto currentType = ScreenplayBlockStyle::forBlock(cursor.block());
            if (currentType == ScreenplayParagraphType::FolderFooter) {
                if (openedGroups == 0) {
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    if (cursor.hasSelection()) {
                        cursor.deleteChar();
                    }

                    //
                    // Уберём сам блок
                    //
                    ScreenplayTextBlockData* blockData = nullptr;
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
            } else if (currentType == ScreenplayParagraphType::FolderHeader) {
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
            const auto currentType = ScreenplayBlockStyle::forBlock(cursor.block());
            if (currentType == ScreenplayParagraphType::FolderHeader) {
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
            } else if (currentType == ScreenplayParagraphType::FolderFooter) {
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

} // namespace Ui
