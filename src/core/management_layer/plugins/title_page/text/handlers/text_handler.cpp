#include "text_handler.h"

#include "../title_page_edit.h"

#include <business_layer/templates/simple_text_template.h>

using Ui::TitlePageEdit;


namespace KeyProcessingLayer {

TextHandler::TextHandler(TitlePageEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void TextHandler::handleEnter(QKeyEvent*)
{
    //
    // Вставляем ещё один блок текста
    //
    editor()->addParagraph(BusinessLayer::TextParagraphType::Text);
}

void TextHandler::handleTab(QKeyEvent*)
{
    //
    // Ничего не делаем
    //
}

} // namespace KeyProcessingLayer
