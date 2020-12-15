#include "messages_view.h"

#include "message.h"
#include "messages_view_content.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>


class MessagesView::Implementation
{
public:
    Implementation();

    MessagesViewContent* content = nullptr;
};

MessagesView::Implementation::Implementation()
    : content(new MessagesViewContent)
{
    content->setBackgroundColor(Ui::DesignSystem::color().background());
    content->setTextColor(Ui::DesignSystem::color().onBackground());
}


// ****


MessagesView::MessagesView(QWidget* _parent)
    : QScrollArea(_parent),
      d(new Implementation)
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    setPalette(palette);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBar(new ScrollBar);

    setWidget(d->content);
    setWidgetResizable(true);
}

MessagesView::~MessagesView() = default;

void MessagesView::setCurrectUser(const User& _user)
{
    d->content->setCurrectUser(_user);
}

void MessagesView::setMessages(const QVector<Message>& _messages)
{
    d->content->setMessages(_messages);
}
