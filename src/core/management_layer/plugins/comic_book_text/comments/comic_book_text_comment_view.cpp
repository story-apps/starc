#include "comic_book_text_comment_view.h"

#include "comic_book_text_comment_delegate.h"

#include <ui/design_system/design_system.h>

#include <QModelIndex>
#include <QPainter>


namespace Ui {

class ComicBookTextCommentView::Implementation
{
public:
    QStyleOptionViewItem option(const Widget* _widget) const;

    ComicBookTextCommentDelegate delegate;
    QModelIndex commentIndex;
};

QStyleOptionViewItem ComicBookTextCommentView::Implementation::option(const Widget* _widget) const
{
    QStyleOptionViewItem option;
    option.widget = _widget;
    option.rect = _widget->rect();
    option.palette.setColor(QPalette::Base, _widget->backgroundColor());
    option.palette.setColor(QPalette::Text, _widget->textColor());
    return option;
}


// ****


ComicBookTextCommentView::ComicBookTextCommentView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    auto sizePolicy = this->sizePolicy();
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
    d->delegate.setSingleCommentMode(true);
}

ComicBookTextCommentView::~ComicBookTextCommentView() = default;

void ComicBookTextCommentView::setCommentIndex(const QModelIndex& _index)
{
    if (d->commentIndex == _index) {
        return;
    }

    d->commentIndex = _index;
    updateGeometry();
    update();
}

int ComicBookTextCommentView::heightForWidth(int _width) const
{
    auto option = d->option(this);
    option.rect.setWidth(_width);
    return d->delegate.sizeHint(option, d->commentIndex).height();
}

void ComicBookTextCommentView::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    QPainter painter(this);
    d->delegate.paint(&painter, d->option(this), d->commentIndex);
}

void ComicBookTextCommentView::mouseReleaseEvent(QMouseEvent* _event)
{
    Widget::mouseReleaseEvent(_event);

    emit clicked();
}
} // namespace Ui
