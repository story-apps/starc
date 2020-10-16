#include "screenplay_text_cursor.h"

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

} // namespace Ui
