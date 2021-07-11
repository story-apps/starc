#include "text_handler.h"

#include "../screenplay_title_page_edit.h"

using Ui::ScreenplayTitlePageEdit;


namespace KeyProcessingLayer {

TextHandler::TextHandler(ScreenplayTitlePageEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void TextHandler::handleEnter(QKeyEvent*)
{
    //
    // Вставляем ещё один блок текста
    //
    editor()->addParagraph(editor()->currentParagraphType());
}

void TextHandler::handleTab(QKeyEvent*)
{
    //
    // Ничего не делаем
    //
}

} // namespace KeyProcessingLayer
