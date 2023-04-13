#include "pre_handler.h"

#include "../stageplay_text_edit.h"

#include <business_layer/templates/stageplay_template.h>
#include <business_layer/templates/templates_facade.h>

#include <QKeyEvent>
#include <QTextBlock>

using Ui::StageplayTextEdit;


namespace KeyProcessingLayer {

PreHandler::PreHandler(Ui::StageplayTextEdit* _editor)
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
    const auto& topStyle = editor()->stageplayTemplate().paragraphStyle(
        BusinessLayer::TextBlockStyle::forBlock(topCursor.block()));

    //
    // Получим стиль последнего блока в выделении
    //
    QTextCursor bottomCursor(editor()->document());
    bottomCursor.setPosition(qMax(cursor.selectionStart(), cursor.selectionEnd()));
    const auto& bottomStyle = editor()->stageplayTemplate().paragraphStyle(
        BusinessLayer::TextBlockStyle::forBlock(bottomCursor.block()));

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
