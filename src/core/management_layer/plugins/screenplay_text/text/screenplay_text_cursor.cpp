#include "screenplay_text_cursor.h"

#include "screenplay_text_block_data.h"
#include "screenplay_text_edit.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <QTextTable>


namespace BusinessLayer {

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

void ScreenplayTextCursor::removeCharacters(Ui::ScreenplayTextEdit* _editor)
{
    removeCharacters(true, _editor);
}

//
// TODO: В нижеследующих методах сделать аккуратно через собственный курсор
//

void ScreenplayTextCursor::removeCharacters(bool _backward, Ui::ScreenplayTextEdit* _editor)
{
    Q_ASSERT(document() == _editor->document());

    auto cursor = _editor->textCursor();
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
            topCursorPosition = qMin(cursor.selectionStart(), cursor.selectionEnd());
            bottomCursorPosition = qMax(cursor.selectionStart(), cursor.selectionEnd());
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
    // Получим стили блоков начала и конца выделения
    //
    // ... начала
    //
    auto topParagraphType = ScreenplayParagraphType::Undefined;
    QTextBlock topBlock;
    {
        QTextCursor topCursor(document());
        topCursor.setPosition(topCursorPosition);
        topParagraphType = ScreenplayBlockStyle::forBlock(topCursor.block());
        topBlock = topCursor.block();
    }
    //
    // ... и конца
    //
    auto bottomParagraphType = ScreenplayParagraphType::Undefined;
    QTextBlock bottomBlock;
    {
        QTextCursor bottomCursor(document());
        bottomCursor.setPosition(bottomCursorPosition);
        bottomParagraphType = ScreenplayBlockStyle::forBlock(bottomCursor.block());
        bottomBlock = bottomCursor.block();
    }
    const auto topStyle = ScreenplayTemplateFacade::getTemplate().blockStyle(topParagraphType);
    const auto bottomStyle = ScreenplayTemplateFacade::getTemplate().blockStyle(bottomParagraphType);

    //
    // Определим стиль результирующего блока
    //
    auto targetStyle = _backward ? topStyle : bottomStyle;
    {
        if (topBlock == bottomBlock) {
            targetStyle = topStyle;
        } else {
            if (topStyle.isEmbeddable() && !bottomStyle.isEmbeddable() && !bottomBlock.text().isEmpty()) {
                targetStyle = bottomStyle;
            } else if (!topBlock.text().isEmpty()) {
                targetStyle = topStyle;
            } else if (!bottomBlock.text().isEmpty()) {
                targetStyle = bottomStyle;
            }
        }
    }

    //
    // Собственно удаление
    //
    FoldersToDelete groupsToDeleteCounts;
    cursor.beginEditBlock();
    {
        //
        // Подсчитать количество группирующих элементов входящих в выделение
        //
        const bool needToDeleteGroups
                = topBlock != bottomBlock
                  && (topStyle.isEmbeddable() || bottomStyle.isEmbeddable());
        if (needToDeleteGroups) {
            groupsToDeleteCounts = findGroupCountsToDelete(topCursorPosition, bottomCursorPosition);
        }

        //
        // В результирующем блоке должны остаться данные от верхнего блока (нижний блок считаем удалённым)
        //
        BusinessLayer::ScreenplayTextBlockData* blockData = nullptr;
        if (topBlock.userData() != nullptr) {
            auto topBlockData = static_cast<BusinessLayer::ScreenplayTextBlockData*>(topBlock.userData());
            if (topBlockData != nullptr) {
                blockData = new BusinessLayer::ScreenplayTextBlockData(topBlockData);
            }
        }


        //
        // Удалить текст
        //
        cursor.setPosition(topCursorPosition);
        cursor.setPosition(bottomCursorPosition, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        //
        // Положим корректные данные в блок
        //
        cursor.block().setUserData(blockData);

        //
        // Удалить вторые половинки группирующих элементов
        //
        if (needToDeleteGroups) {
            removeGroupsPairs(cursor.position(), groupsToDeleteCounts);
        }
    }

    //
    // Если и верхний и нижний блоки являются началом и концом папки,
    //
    if (topParagraphType == ScreenplayParagraphType::FolderHeader
        && bottomParagraphType == ScreenplayParagraphType::FolderFooter) {
        //
        // ... то ничего не делаем, весь мусорный текст был удалён выше
        //
    }
    //
    // Применим финальный стиль
    //
    else {
        //
        // Если стиль последнего абзаца сменился при удалении, корректируем его
        //
        if (_editor->currentParagraphType() != targetStyle.type()) {
            _editor->setCurrentParagraphType(targetStyle.type());
        }
        //
        // В противном случае, при удалении, формат текста остаётся от предыдущего блока,
        // поэтому перезапишем его корректным форматом
        //
        else {
            cursor.setBlockCharFormat(targetStyle.charFormat());
        }
    }

    //
    // Завершим операцию удаления
    //
    cursor.endEditBlock();
}

ScreenplayTextCursor::FoldersToDelete ScreenplayTextCursor::findGroupCountsToDelete(int _topCursorPosition, int _bottomCursorPosition)
{
    FoldersToDelete groupCountsToDelete;

    //
    // Начнём поиск с заданной позиции
    //
    QTextCursor searchGroupsCursor(document());
    searchGroupsCursor.setPosition(_topCursorPosition);

    //
    // Пропускаем поиск в первом блоке области удаления, т.к. он всегда остаётся с текущим стилем
    //
    searchGroupsCursor.movePosition(QTextCursor::NextBlock);

    while (searchGroupsCursor.position() <= _bottomCursorPosition) {
        //
        // Определим тип блока
        //
        ScreenplayParagraphType currentType =
                ScreenplayBlockStyle::forBlock(searchGroupsCursor.block());

        //
        // Если найден блок открывающий папку, то нужно удалить закрывающий блок
        //
        if (currentType == ScreenplayParagraphType::FolderHeader) {
            ++groupCountsToDelete.footers;
        }
        //
        // Если найден блок закрывающий папку
        // ... если все группы закрыты, нужно удалить предыдущую открытую
        // ... в противном случае закрываем открытую группу
        //
        else if (currentType == ScreenplayParagraphType::FolderFooter) {
            if (groupCountsToDelete.footers == 0) {
                ++groupCountsToDelete.headers;
            }
            else {
                --groupCountsToDelete.footers;
            }
        }

        //
        // Если дошли до конца, прерываем выполнение
        //
        if (searchGroupsCursor.atEnd()) {
            break;
        }

        //
        // Перейдём к следующему блоку или концу блока
        //
        searchGroupsCursor.movePosition(QTextCursor::EndOfBlock);
        searchGroupsCursor.movePosition(QTextCursor::NextBlock);
    }

    return groupCountsToDelete;
}

void ScreenplayTextCursor::removeGroupsPairs(int _cursorPosition, const ScreenplayTextCursor::FoldersToDelete& _groupCountsToDelete)
{
    //
    // Удалим пары из последующего текста
    //
    // ... папки
    //
    if (_groupCountsToDelete.footers > 0) {
        QTextCursor cursor(document());
        cursor.setPosition(_cursorPosition);

        // ... открытые группы на пути поиска необходимого для удаления блока
        int openedGroups = 0;
        int groupsToDeleteCount = _groupCountsToDelete.footers;
        do {
            const auto currentType = ScreenplayBlockStyle::forBlock(cursor.block());
            if (currentType == ScreenplayParagraphType::FolderFooter) {
                if (openedGroups == 0) {
                    cursor.select(QTextCursor::BlockUnderCursor);
                    cursor.deleteChar();

                    //
                    // Если жто был самый последний блок
                    //
                    if (cursor.atEnd()) {
                        cursor.deletePreviousChar();
                    }

                    --groupsToDeleteCount;

                    continue;
                } else {
                    --openedGroups;
                }
            } else if (currentType == ScreenplayParagraphType::FolderHeader) {
                // ... встретилась новая группа, которую не нужно удалять
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
    // ... папки
    //
    if (_groupCountsToDelete.headers > 0) {
        QTextCursor cursor(document());
        cursor.setPosition(_cursorPosition);

        // ... открытые группы на пути поиска необходимого для удаления блока
        int openedGroups = 0;
        int groupsToDeleteCount = _groupCountsToDelete.headers;
        do {
            const auto currentType = ScreenplayBlockStyle::forBlock(cursor.block());
            if (currentType == ScreenplayParagraphType::FolderHeader) {
                if (openedGroups == 0) {
                    cursor.select(QTextCursor::BlockUnderCursor);
                    cursor.deleteChar();

                    //
                    // Если это был самый первый блок
                    //
                    if (cursor.atStart()) {
                        cursor.deleteChar();
                    }

                    --groupsToDeleteCount;

                    continue;
                } else {
                    --openedGroups;
                }
            } else if (currentType == ScreenplayParagraphType::FolderFooter) {
                // ... встретилась новая группа, которую не нужно удалять
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
