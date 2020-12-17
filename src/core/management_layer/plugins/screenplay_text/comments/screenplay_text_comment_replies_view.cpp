#include "screenplay_text_comment_replies_view.h"

#include "screenplay_text_comment_view.h"
#include "screenplay_text_comments_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>

#include <ui/design_system/design_system.h>

#include <ui/widgets/chat/chat_message.h>
#include <ui/widgets/chat/chat_messages_view.h>
#include <ui/widgets/chat/user.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/text_field/text_field.h>

#include <QBoxLayout>
#include <QDateTime>
#include <QScrollArea>

using BusinessLayer::ScreenplayTextCommentsModel;


namespace Ui
{

class ScreenplayTextCommentRepliesView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    ScreenplayTextCommentView* headerView = nullptr;
    ChatMessagesView* repliesView = nullptr;
    QScrollArea* repliesViewContainer = nullptr;
    Shadow* repliesViewTopShadow = nullptr;
    TextField* replyTextField = nullptr;
};

ScreenplayTextCommentRepliesView::Implementation::Implementation(QWidget* _parent)
    : headerView(new ScreenplayTextCommentView(_parent)),
      repliesView(new ChatMessagesView),
      repliesViewContainer(new QScrollArea(_parent)),
      repliesViewTopShadow(new Shadow(Qt::TopEdge, repliesViewContainer)),
      replyTextField(new TextField(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    repliesViewContainer->setPalette(palette);
    repliesViewContainer->setFrameShape(QFrame::NoFrame);
    repliesViewContainer->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    repliesViewContainer->setVerticalScrollBar(new ScrollBar);
    repliesViewContainer->setWidget(repliesView);
    repliesViewContainer->setWidgetResizable(true);

    replyTextField->setEnterMakesNewLine(true);
    replyTextField->setTrailingIcon(u8"\U000f048A");
    replyTextField->setUnderlineDecorationVisible(false);
    replyTextField->setTitleVisible(false);
    replyTextField->setDefaultMarginsEnabled(false);
}


// ****


ScreenplayTextCommentRepliesView::ScreenplayTextCommentRepliesView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    setFocusProxy(d->replyTextField);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->headerView);
    layout->addWidget(d->repliesViewContainer, 1);
    layout->addWidget(d->replyTextField);

    connect(d->headerView, &ScreenplayTextCommentView::clicked, this, &ScreenplayTextCommentRepliesView::closePressed);


    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayTextCommentRepliesView::~ScreenplayTextCommentRepliesView()
{

}

void ScreenplayTextCommentRepliesView::setCommentIndex(const QModelIndex& _index)
{
    d->headerView->setCommentIndex(_index);

    const auto comments
            = _index.data(ScreenplayTextCommentsModel::ReviewMarkCommentsRole)
                    .value<QVector<BusinessLayer::ScreenplayTextModelTextItem::ReviewComment>>();
    QVector<ChatMessage> replies;
    for (auto comment : comments) {
        if (comment == comments.first()
            || comment.text.isEmpty()) {
            continue;
        }

        replies.append({ QDateTime::fromString(comment.date, Qt::ISODate), comment.text, User(comment.author) });
    }
    d->repliesView->setMessages(replies);
}

void ScreenplayTextCommentRepliesView::updateTranslations()
{

}

void ScreenplayTextCommentRepliesView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());

    d->headerView->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->headerView->setTextColor(Ui::DesignSystem::color().onPrimary());

    d->repliesView->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->repliesView->setTextColor(Ui::DesignSystem::color().onPrimary());

    d->replyTextField->setBackgroundColor(Ui::DesignSystem::color().onPrimary());
    d->replyTextField->setTextColor(Ui::DesignSystem::color().onPrimary());
}

} // namespace Ui
