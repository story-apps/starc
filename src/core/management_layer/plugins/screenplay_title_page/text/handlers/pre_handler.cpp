#include "pre_handler.h"

#include "../screenplay_title_page_edit.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>

#include <QKeyEvent>
#include <QTextBlock>

using Ui::ScreenplayTitlePageEdit;


namespace KeyProcessingLayer
{

PreHandler::PreHandler(Ui::ScreenplayTitlePageEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void PreHandler::handleDelete(QKeyEvent* _event)
{
    if (_event == nullptr) {
		StandardKeyHandler::handleDelete();
	}
}

void PreHandler::handleOther(QKeyEvent* _event)
{
	//
	// Получим необходимые значения
	//
	// ... курсор в текущем положении
	QTextCursor cursor = editor()->textCursor();

	//
	// Получим стиль первого блока в выделении
	//
	QTextCursor topCursor(editor()->document());
	topCursor.setPosition(qMin(cursor.selectionStart(), cursor.selectionEnd()));
    const auto topStyle = BusinessLayer::TemplatesFacade::screenplayTemplate().blockStyle(
                              BusinessLayer::ScreenplayBlockStyle::forBlock(topCursor.block()));

	//
	// Получим стиль последнего блока в выделении
	//
	QTextCursor bottomCursor(editor()->document());
	bottomCursor.setPosition(qMax(cursor.selectionStart(), cursor.selectionEnd()));
    const auto bottomStyle = BusinessLayer::TemplatesFacade::screenplayTemplate().blockStyle(
                                 BusinessLayer::ScreenplayBlockStyle::forBlock(bottomCursor.block()));

	//
	// Не все стили можно редактировать
	//
	if (topStyle.isCanModify()
		&& bottomStyle.isCanModify()) {
		//
		// Если имеется выделение, то удалим все выделенные символы
		//
		if (cursor.hasSelection()
			&& !_event->text().isEmpty()) {
			handleDelete();
		}
	}
}

} // namespace KeyProcessingLayer
