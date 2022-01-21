#include "text_cursor.h"

#include <QtGui/private/qtextdocument_p.h>


namespace BusinessLayer {

TextCursor::TextCursor()
    : QTextCursor()
{
}

TextCursor::TextCursor(const QTextCursor& _other)
    : QTextCursor(_other)
{
}

TextCursor::TextCursor(QTextDocument* _document)
    : QTextCursor(_document)
{
}

TextCursor::~TextCursor() = default;

bool TextCursor::isInEditBlock() const
{
    return QTextDocumentPrivate::get(document())->isInEditBlock();
}

TextCursor::Selection TextCursor::selectionInterval() const
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

void TextCursor::restartEditBlock()
{
    endEditBlock();

    int editsCount = 0;
    while (isInEditBlock()) {
        ++editsCount;
        endEditBlock();
    }

    joinPreviousEditBlock();

    while (editsCount != 0) {
        beginEditBlock();
        --editsCount;
    }
}

} // namespace BusinessLayer
