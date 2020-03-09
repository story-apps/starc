#include "folder_header_handler.h"

#include <business_layer/templates/screenplay_template.h>

#include <ui/screenplay_text_edit.h>

#include <QCoreApplication>
#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::ScreenplayBlockStyle;
using BusinessLayer::ScreenplayParagraphType;
using Ui::ScreenplayTextEdit;


namespace KeyProcessingLayer
{

FolderHeaderHandler::FolderHeaderHandler(ScreenplayTextEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void FolderHeaderHandler::handleEnter(QKeyEvent*)
{
	//
	// Получим необходимые значения
	//
	// ... курсор в текущем положении
	QTextCursor cursor = editor()->textCursor();
	// ... блок текста в котором находится курсор
	QTextBlock currentBlock = cursor.block();
	// ... текст до курсора
	QString cursorBackwardText = currentBlock.text().left(cursor.positionInBlock());
	// ... текст после курсора
	QString cursorForwardText = currentBlock.text().mid(cursor.positionInBlock());


	//
	// Обработка
	//
	if (editor()->isCompleterVisible()) {
		//! Если открыт подстановщик

		//
		// Ни чего не делаем
		//
	} else {
		//! Подстановщик закрыт

		if (cursor.hasSelection()) {
			//! Есть выделение

			//
			// Ни чего не делаем
			//
		} else {
			//! Нет выделения

			if (cursorBackwardText.isEmpty()
				&& cursorForwardText.isEmpty()) {
				//! Текст пуст

				//
				// Ни чего не делаем
				//
                editor()->setCurrentParagraphType(changeForEnter(ScreenplayParagraphType::FolderHeader));
			} else {
				//! Текст не пуст

				if (cursorBackwardText.isEmpty()) {
					//! В начале блока

					//
					// Вставить блок время и место перед папкой
					//
                    cursor.beginEditBlock();
					cursor.insertBlock();
					cursor.movePosition(QTextCursor::PreviousCharacter);
					cursor.setBlockFormat(QTextBlockFormat());
					editor()->setTextCursor(cursor);
                    editor()->setCurrentParagraphType(ScreenplayParagraphType::SceneHeading);
                    editor()->moveCursor(QTextCursor::NextCharacter);
                    cursor.endEditBlock();
				} else if (cursorForwardText.isEmpty()) {
					//! В конце блока

					//
					// Вставить блок время и место
					//
                    editor()->addParagraph(jumpForEnter(ScreenplayParagraphType::FolderHeader));
				} else {
					//! Внутри блока

					//
					// Вставить блок время и место
					//
                    editor()->addParagraph(ScreenplayParagraphType::SceneHeading);
				}
			}
		}
	}
}

void FolderHeaderHandler::handleTab(QKeyEvent*)
{
	//
	// Получим необходимые значения
	//
	// ... курсор в текущем положении
	QTextCursor cursor = editor()->textCursor();
	// ... блок текста в котором находится курсор
	QTextBlock currentBlock = cursor.block();
	// ... текст до курсора
	QString cursorBackwardText = currentBlock.text().left(cursor.positionInBlock());
	// ... текст после курсора
	QString cursorForwardText = currentBlock.text().mid(cursor.positionInBlock());


	//
	// Обработка
	//
	if (editor()->isCompleterVisible()) {
		//! Если открыт подстановщик

		//
		// Ни чего не делаем
		//
	} else {
		//! Подстановщик закрыт

		if (cursor.hasSelection()) {
			//! Есть выделение

			//
			// Ни чего не делаем
			//
		} else {
			//! Нет выделения

			if (cursorBackwardText.isEmpty()
				&& cursorForwardText.isEmpty()) {
				//! Текст пуст

				//
				// Ни чего не делаем
				//
                editor()->setCurrentParagraphType(changeForTab(ScreenplayParagraphType::FolderHeader));
			} else {
				//! Текст не пуст

				if (cursorBackwardText.isEmpty()) {
					//! В начале блока

					//
					// Ни чего не делаем
					//
				} else if (cursorForwardText.isEmpty()) {
					//! В конце блока

					//
					// Как ENTER
					//
                    editor()->addParagraph(jumpForTab(ScreenplayParagraphType::FolderHeader));
				} else {
					//! Внутри блока

					//
					// Ни чего не делаем
					//
				}
			}
		}
	}
}

void FolderHeaderHandler::handleOther(QKeyEvent* _event)
{
	//
	// Если не было введено текста, прерываем операцию
	// _event->key() == -1 // событие посланное редактором текста, его необходимо обработать
	//
	if (_event == 0
		|| (_event->key() != -1 && _event->text().isEmpty())) {
		return;
	}

    updateFooter();
}

void FolderHeaderHandler::handleInput(QInputMethodEvent* _event)
{
    //
    // Если не было введено текста, прерываем операцию
    //
    if (_event->commitString().isEmpty()
        || _event->preeditString().isEmpty()) {
        return;
    }

    updateFooter();
}

void FolderHeaderHandler::updateFooter()
{
    QTextCursor cursor = editor()->textCursor();

    //
    // Если редактируется заголовок группы
    //
    if (editor()->currentParagraphType() == ScreenplayParagraphType::FolderHeader) {
        //
        // ... открытые группы на пути поиска необходимого для обновления блока
        //
        int openedGroups = 0;
        bool isFooterUpdated = false;
        while (!isFooterUpdated && !cursor.atEnd()) {
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.movePosition(QTextCursor::NextBlock);

            const ScreenplayParagraphType currentType = ScreenplayBlockStyle::forBlock(cursor.block());
            if (currentType == ScreenplayParagraphType::FolderFooter) {
                if (openedGroups == 0) {
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    cursor.insertText(
                        QString("%1 %2")
                                .arg(QCoreApplication::translate("KeyProcessingLayer::FolderFooterHandler", "END OF"),
                                     editor()->textCursor().block().text()));
                    isFooterUpdated = true;
                } else {
                    --openedGroups;
                }
            } else if (currentType == ScreenplayParagraphType::FolderHeader) {
                // ... встретилась новая группа
                ++openedGroups;
            }
        }
    }
}

} // namespace KeyProcessingLayer
