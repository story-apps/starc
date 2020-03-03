#include "screenplay_text_cursor.h"


namespace Ui {

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

bool ScreenplayTextCursor::isBlockInTable() const
{
    return currentTable() != nullptr;
}

} // namespace Ui
