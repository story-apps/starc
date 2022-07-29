#include "comment_replies_view.h"

#include "comment_view.h"
#include "comments_model.h"

#include <business_layer/model/text/text_model_text_item.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/chat/chat_message.h>
#include <ui/widgets/chat/chat_messages_view.h>
#include <ui/widgets/chat/user.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
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

namespace {
constexpr int kInvalidIndex = -1;
}

class CommentRepliesView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QModelIndex commentIndex;
    int editedReplyIndex = kInvalidIndex;

    CommentView* headerView = nullptr;
    ChatMessagesView* repliesView = nullptr;
    QScrollArea* repliesViewContainer = nullptr;
    ScrollBar* repliesViewScrollBar = nullptr;
    Subtitle2Label* editingTitle = nullptr;
    Body2Label* editingMessage = nullptr;
    IconButton* cancelEditing = nullptr;
    TextField* replyTextField = nullptr;
};

CommentRepliesView::Implementation::Implementation(QWidget* _parent)
    : headerView(new CommentView(_parent))
    , repliesView(new ChatMessagesView)
    , repliesViewContainer(new QScrollArea(_parent))
    , repliesViewScrollBar(new ScrollBar(repliesViewContainer))
    , editingTitle(new Subtitle2Label(_parent))
    , editingMessage(new Body2Label(_parent))
    , cancelEditing(new IconButton(_parent))
    , replyTextField(new TextField(_parent))
{
    new Shadow(Qt::TopEdge, repliesViewContainer);
    new Shadow(Qt::BottomEdge, repliesViewContainer);

    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    repliesViewContainer->setPalette(palette);
    repliesViewContainer->setFrameShape(QFrame::NoFrame);
    repliesViewContainer->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    repliesViewContainer->setVerticalScrollBar(repliesViewScrollBar);
    repliesViewContainer->setWidget(repliesView);
    repliesViewContainer->setWidgetResizable(true);

    editingTitle->hide();
    editingMessage->hide();
    cancelEditing->setIcon(u8"\U000F0156");
    cancelEditing->hide();

    UiHelper::initSpellingFor(replyTextField);
    replyTextField->setEnterMakesNewLine(true);
    replyTextField->setTrailingIcon(u8"\U000f048A");
    replyTextField->setUnderlineDecorationVisible(false);
    replyTextField->setLabelVisible(false);
    replyTextField->setDefaultMarginsEnabled(false);
}


// ****


CommentRepliesView::CommentRepliesView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusProxy(d->replyTextField);

    d->replyTextField->installEventFilter(this);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->headerView);
    layout->addWidget(d->repliesViewContainer, 1);
    {
        auto editingLayout = new QGridLayout;
        editingLayout->setContentsMargins({});
        editingLayout->setSpacing(0);
        editingLayout->addWidget(d->editingTitle, 0, 0);
        editingLayout->addWidget(d->editingMessage, 1, 0);
        editingLayout->addWidget(d->cancelEditing, 0, 1, 2, 1, Qt::AlignHCenter | Qt::AlignTop);
        editingLayout->setColumnStretch(0, 1);
        layout->addLayout(editingLayout);
    }
    layout->addWidget(d->replyTextField);

    connect(d->repliesView, &ChatMessagesView::messageContextMenuRequested, this,
            &CommentRepliesView::replyContextMenuRequested);
    connect(d->headerView, &CommentView::clicked, this, &CommentRepliesView::closePressed);
    connect(d->cancelEditing, &IconButton::clicked, this,
            [this] { changeMessage(kInvalidIndex, {}); });
    connect(d->replyTextField, &TextField::trailingIconPressed, this,
            &CommentRepliesView::postReply);
}

CommentRepliesView::~CommentRepliesView() = default;

void CommentRepliesView::setReadOnly(bool _readOnly)
{
    d->replyTextField->setReadOnly(_readOnly);
}

void CommentRepliesView::setCurrentUser(const QString& _name, const QString& _email)
{
    d->repliesView->setCurrentUser(User(_name, _email));
}

void CommentRepliesView::changeMessage(int _replyIndex, const QString& _text)
{
    d->editedReplyIndex = _replyIndex;

    if (_text.isEmpty()) {
        d->editingTitle->hide();
        d->editingMessage->hide();
        d->cancelEditing->hide();
        d->replyTextField->setTrailingIcon(u8"\U000F048A");
        d->replyTextField->clear();
        return;
    }

    d->editingTitle->show();
    d->editingMessage->setText(_text);
    d->editingMessage->show();
    d->cancelEditing->show();
    d->replyTextField->setTrailingIcon(u8"\U000F0450");
    d->replyTextField->setText(_text);
    d->replyTextField->setFocus();
}

QModelIndex CommentRepliesView::commentIndex() const
{
    return d->commentIndex;
}

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
    const auto comments = _index.data(CommentsModel::ReviewMarkRepliesRole)
                              .value<QVector<BusinessLayer::TextModelTextItem::ReviewComment>>();
    QVector<ChatMessage> replies;
    for (const auto& comment : comments) {
        if (comment == comments.first()) {
            continue;
        }

        replies.append({ QDateTime::fromString(comment.date, Qt::ISODate), comment.text,
                         User(comment.author, comment.authorEmail) });
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
            if (d->editedReplyIndex != kInvalidIndex) {
                changeMessage(kInvalidIndex, {});
            } else {
                emit closePressed();
            }
            return true;
        } else if (keyEvent->modifiers().testFlag(Qt::ControlModifier)
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
        if (d->editedReplyIndex != kInvalidIndex) {
            changeMessage(kInvalidIndex, {});
        } else {
            emit closePressed();
        }
    }

    Widget::keyPressEvent(_event);
}

void CommentRepliesView::updateTranslations()
{
    d->headerView->setToolTip(tr("Back to comments list"));
    d->editingTitle->setText(tr("Edit reply"));
    d->replyTextField->setTrailingIconToolTip(QString("%1 (%2)").arg(
        tr("Add reply"), QKeySequence("Ctrl+Enter").toString(QKeySequence::NativeText)));
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

    d->editingTitle->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->editingTitle->setTextColor(Ui::DesignSystem::color().secondary());
    d->editingTitle->setContentsMargins(Ui::DesignSystem::layout().px16(),
                                        Ui::DesignSystem::layout().px12(), 0,
                                        Ui::DesignSystem::layout().px4());
    d->editingMessage->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->editingMessage->setContentsMargins(Ui::DesignSystem::layout().px16(), 0, 0,
                                          Ui::DesignSystem::layout().px12());
    d->editingMessage->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->cancelEditing->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->cancelEditing->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->cancelEditing->setContentsMargins(0, 0, Ui::DesignSystem::layout().px2(), 0);

    d->replyTextField->setBackgroundColor(Ui::DesignSystem::color().onPrimary());
    d->replyTextField->setTextColor(Ui::DesignSystem::color().onPrimary());
}

void CommentRepliesView::postReply()
{
    if (d->replyTextField->text().isEmpty()) {
        return;
    }

    if (d->editedReplyIndex == kInvalidIndex) {
        emit addReplyPressed(d->replyTextField->text());
        d->replyTextField->clear();
    } else {
        emit editReplyPressed(d->editedReplyIndex, d->replyTextField->text());
        changeMessage(kInvalidIndex, {});
    }
    setCommentIndex(d->commentIndex);
}

} // namespace Ui
