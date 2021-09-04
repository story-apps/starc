#include "pre_handler.h"

#include "../comic_book_text_edit.h"

#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/templates_facade.h>

#include <QKeyEvent>
#include <QTextBlock>

using Ui::ComicBookTextEdit;


namespace KeyProcessingLayer {

PreHandler::PreHandler(Ui::ComicBookTextEdit* _editor)
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
    const auto topStyle = BusinessLayer::TemplatesFacade::comicBookTemplate().paragraphStyle(
        BusinessLayer::ComicBookBlockStyle::forBlock(topCursor.block()));

    //
    // Получим стиль последнего блока в выделении
    //
    QTextCursor bottomCursor(editor()->document());
    bottomCursor.setPosition(qMax(cursor.selectionStart(), cursor.selectionEnd()));
    const auto bottomStyle = BusinessLayer::TemplatesFacade::comicBookTemplate().paragraphStyle(
        BusinessLayer::ComicBookBlockStyle::forBlock(bottomCursor.block()));

    //
    // Не все стили можно редактировать
    //
    if (topStyle.isCanModify() && bottomStyle.isCanModify()) {
        //
        // Если имеется выделение, то удалим все выделенные символы
        //
        if (cursor.hasSelection() && !_event->text().isEmpty()) {
            handleDelete();
        }
    }
}

} // namespace KeyProcessingLayer
