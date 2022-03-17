#include "comment_replies_view.h"

#include "comment_view.h"
#include "comments_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/chat/chat_message.h>
#include <ui/widgets/chat/chat_messages_view.h>
#include <ui/widgets/chat/user.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QBoxLayout>
#include <QDateTime>
#include <QKeyEvent>
#include <QScrollArea>
#include <QTimer>

using BusinessLayer::CommentsModel;


namespace Ui {

class CommentRepliesView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    QModelIndex commentIndex;

    CommentView* headerView = nullptr;
    ChatMessagesView* repliesView = nullptr;
    QScrollArea* repliesViewContainer = nullptr;
    ScrollBar* repliesViewScrollBar = nullptr;
    Shadow* repliesViewTopShadow = nullptr;
    TextField* replyTextField = nullptr;
};

CommentRepliesView::Implementation::Implementation(QWidget* _parent)
    : headerView(new CommentView(_parent))
    , repliesView(new ChatMessagesView)
    , repliesViewContainer(new QScrollArea(_parent))
    , repliesViewScrollBar(new ScrollBar(repliesViewContainer))
    , repliesViewTopShadow(new Shadow(Qt::TopEdge, repliesViewContainer))
    , replyTextField(new TextField(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    repliesViewContainer->setPalette(palette);
    repliesViewContainer->setFrameShape(QFrame::NoFrame);
    repliesViewContainer->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    repliesViewContainer->setVerticalScrollBar(repliesViewScrollBar);
    repliesViewContainer->setWidget(repliesView);
    repliesViewContainer->setWidgetResizable(true);

    UiHelper::initSpellingFor(replyTextField);
    replyTextField->setEnterMakesNewLine(true);
    replyTextField->setTrailingIcon(u8"\U000f048A");
    replyTextField->setUnderlineDecorationVisible(false);
    replyTextField->setTitleVisible(false);
    replyTextField->setDefaultMarginsEnabled(false);
}


// ****


CommentRepliesView::CommentRepliesView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusProxy(d->replyTextField);

    d->replyTextField->installEventFilter(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->headerView);
    layout->addWidget(d->repliesViewContainer, 1);
    layout->addWidget(d->replyTextField);

    connect(d->headerView, &CommentView::clicked, this, &CommentRepliesView::closePressed);
    connect(d->replyTextField, &TextField::trailingIconPressed, this,
            &CommentRepliesView::postReply);


    updateTranslations();
    designSystemChangeEvent(nullptr);
}

QModelIndex CommentRepliesView::commentIndex() const
{
    return d->commentIndex;
}

CommentRepliesView::~CommentRepliesView() = default;

void CommentRepliesView::setCommentIndex(const QModelIndex& _index)
{
    //
    // Если сменился индекс, отобразим вверху текущий комментарий
    //
    const bool isIndexChanged = d->commentIndex != _index;
    if (isIndexChanged) {
        d->commentIndex = _index;
        d->headerView->setCommentIndex(d->commentIndex);
    }

    //
    // Собираем ответы на комментарий и помещаем их во вьюху
    //
    const auto comments
        = _index.data(CommentsModel::ReviewMarkRepliesRole)
              .value<QVector<BusinessLayer::ScreenplayTextModelTextItem::ReviewComment>>();
    QVector<ChatMessage> replies;
    for (auto comment : comments) {
        if (comment == comments.first()) {
            continue;
        }

        replies.append({ QDateTime::fromString(comment.date, Qt::ISODate), comment.text,
                         User(comment.author) });
    }
    d->repliesView->setMessages(replies);

    //
    // Если это установка нового индекса, то предрасчитаем размер скролбара,
    // чтобы проскролить его вниз до момента первой отрисовки экрана
    //
    if (isIndexChanged) {
        const auto repliesHeight = d->repliesView->heightForWidth(width());
        d->repliesViewContainer->verticalScrollBar()->setMaximum(repliesHeight);
    }

    //
    // Отложенно скролим вьюху, чтобы пересчиталась геометрия окна чата
    //
    QTimer::singleShot(0, this, [this] {
        d->repliesViewContainer->verticalScrollBar()->setValue(
            d->repliesViewContainer->verticalScrollBar()->maximum());
    });
}

bool CommentRepliesView::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == d->replyTextField && _event->type() == QEvent::KeyPress) {
        const auto keyEvent = static_cast<QKeyEvent*>(_event);
        if (keyEvent->key() == Qt::Key_Escape) {
            emit closePressed();
        } else if (!keyEvent->modifiers().testFlag(Qt::ShiftModifier)
                   && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {
            postReply();
            return true;
        }
    }

    return Widget::eventFilter(_watched, _event);
}

void CommentRepliesView::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Escape) {
        emit closePressed();
    }

    Widget::keyPressEvent(_event);
}

void CommentRepliesView::updateTranslations()
{
    d->headerView->setToolTip(tr("Back to comments list"));
    d->replyTextField->setTrailingIconToolTip(tr("Add comment"));
}

void CommentRepliesView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());
    setTextColor(Ui::DesignSystem::color().onPrimary());

    d->headerView->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->headerView->setTextColor(Ui::DesignSystem::color().onPrimary());

    d->repliesView->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->repliesView->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->repliesViewScrollBar->setBackgroundColor(
        ColorHelper::transparent(textColor(), Ui::DesignSystem::elevationEndOpacity()));
    d->repliesViewScrollBar->setHandleColor(
        ColorHelper::transparent(textColor(), Ui::DesignSystem::focusBackgroundOpacity()));

    d->replyTextField->setBackgroundColor(Ui::DesignSystem::color().onPrimary());
    d->replyTextField->setTextColor(Ui::DesignSystem::color().onPrimary());
}

void CommentRepliesView::postReply()
{
    if (d->replyTextField->text().isEmpty()) {
        return;
    }

    emit addReplyPressed(d->replyTextField->text());
    d->replyTextField->clear();
    setCommentIndex(d->commentIndex);
}

} // namespace Ui
