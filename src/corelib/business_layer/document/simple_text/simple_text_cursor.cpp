#include "simple_text_cursor.h"

#include <QtGui/private/qtextdocument_p.h>


namespace BusinessLayer {

SimpleTextCursor::SimpleTextCursor()
    : QTextCursor()
{
}

SimpleTextCursor::SimpleTextCursor(const QTextCursor& _other)
    : QTextCursor(_other)
{
}

SimpleTextCursor::SimpleTextCursor(QTextDocument* _document)
    : QTextCursor(_document)
{
}

SimpleTextCursor::~SimpleTextCursor() = default;

bool SimpleTextCursor::isInEditBlock() const
{
#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
    return QTextDocumentPrivate::get(document())->isInEditBlock();
#else
    return document()->docHandle()->isInEditBlock();
#endif
}

SimpleTextCursor::Selection SimpleTextCursor::selectionInterval() const
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

void SimpleTextCursor::restartEditBlock()
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
