#include "account_view.h"

#include "session_widget.h"

#include <domain/session_info.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <ui/widgets/image/image_card.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/tools/debouncer.h>

#include <QAction>
#include <QGridLayout>
#include <QScrollArea>
#include <QVariantAnimation>


namespace Ui {

class AccountView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Проскролить представление до заданного виджета
     */
    void scrollToTitle(AbstractLabel* title);


    QScrollArea* content = nullptr;
    QVariantAnimation scrollAnimation;

    QVariantAnimation colorAnimation;
    AbstractLabel* colorableTitle = nullptr;

    Card* accountInfo = nullptr;
    QGridLayout* accountInfoLayout = nullptr;
    int accountInfoLastRow = 0;
    H6Label* accountTitle = nullptr;
    TextField* name = nullptr;
    Debouncer changeNameDebouncer{ 500 };
    TextField* description = nullptr;
    Debouncer changeDescriptionDebouncer{ 500 };
    ImageCard* avatar = nullptr;

    Card* subscriptionInfo = nullptr;
    QGridLayout* subscriptionInfoLayout = nullptr;
    int subscriptionInfoLastRow = 0;
    H6Label* subscriptionTitle = nullptr;

    H5Label* sessionsTitle = nullptr;
    QVector<SessionWidget*> sessions;
};

AccountView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , accountInfo(new Card(_parent))
    , accountInfoLayout(new QGridLayout)
    , accountTitle(new H6Label(accountInfo))
    , name(new TextField(accountInfo))
    , description(new TextField(accountInfo))
    , avatar(new ImageCard(accountInfo))
    , subscriptionInfo(new Card(_parent))
    , subscriptionInfoLayout(new QGridLayout)
    , subscriptionTitle(new H6Label(subscriptionInfo))
    , sessionsTitle(new H5Label(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);
    scrollAnimation.setEasingCurve(QEasingCurve::OutQuad);
    scrollAnimation.setDuration(180);
    colorAnimation.setEasingCurve(QEasingCurve::InBack);
    colorAnimation.setDuration(1400);

    name->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    description->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    avatar->setDecorationIcon(u8"\U000F0004");

    accountInfoLayout->setContentsMargins({});
    accountInfoLayout->setSpacing(0);
    int row = 0;
    accountInfoLayout->addWidget(accountTitle, row++, 0);
    accountInfoLayout->addWidget(name, row++, 0);
    accountInfoLayout->addWidget(description, row++, 0);
    accountInfoLastRow = row;
    accountInfoLayout->setRowMinimumHeight(accountInfoLastRow,
                                           1); // добавляем пустую строку, вместо отступа снизу
    accountInfo->setLayoutReimpl(accountInfoLayout);

    subscriptionInfoLayout->setContentsMargins({});
    subscriptionInfoLayout->setSpacing(0);
    row = 0;
    subscriptionInfoLayout->addWidget(subscriptionTitle, row++, 0);
    subscriptionInfoLastRow = row;
    subscriptionInfoLayout->setRowMinimumHeight(subscriptionInfoLastRow,
                                                1); // добавляем пустую строку, вместо отступа снизу
    subscriptionInfo->setLayoutReimpl(subscriptionInfoLayout);

    auto contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    auto layout = new QGridLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    row = 0;
    layout->addWidget(accountInfo, row, 0, 1, 2);
    layout->addWidget(avatar, row++, 2, 3, 1, Qt::AlignTop);
    layout->addWidget(subscriptionInfo, row++, 0, 1, 2);
    layout->addWidget(sessionsTitle, row++, 0, 1, 2);
    ++row; // оставляем строку для сессий
    layout->setRowStretch(row, 1);
    contentWidget->setLayout(layout);
}

void AccountView::Implementation::scrollToTitle(AbstractLabel* title)
{
    const QRect microFocus = title->inputMethodQuery(Qt::ImCursorRectangle).toRect();
    const QRect defaultMicroFocus
        = title->QWidget::inputMethodQuery(Qt::ImCursorRectangle).toRect();
    QRect focusRect = (microFocus != defaultMicroFocus)
        ? QRect(title->mapTo(content->widget(), microFocus.topLeft()), microFocus.size())
        : QRect(title->mapTo(content->widget(), QPoint(0, 0)), title->size());
    const QRect visibleRect(-content->widget()->pos(), content->viewport()->size());

    focusRect.adjust(-50, -50, 50, 50);

    scrollAnimation.setStartValue(content->verticalScrollBar()->value());
    scrollAnimation.setEndValue(focusRect.top());
    scrollAnimation.start();

    colorAnimation.stop();
    if (colorableTitle != nullptr) {
        colorableTitle->setTextColor(colorAnimation.endValue().value<QColor>());
    }
    colorableTitle = title;
    colorAnimation.setEndValue(colorableTitle->textColor());
    colorableTitle->setTextColor(colorAnimation.startValue().value<QColor>());
    colorAnimation.start();
}


// ****


AccountView::AccountView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);


    connect(&d->scrollAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                d->content->verticalScrollBar()->setValue(_value.toInt());
            });
    connect(&d->colorAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                if (d->colorableTitle != nullptr) {
                    d->colorableTitle->setTextColor(_value.value<QColor>());
                }
            });
    //
    // Аккаунт
    //
    connect(d->name, &TextField::textChanged, &d->changeNameDebouncer, &Debouncer::orderWork);
    connect(&d->changeNameDebouncer, &Debouncer::gotWork, this, [this] {
        if (d->name->text().isEmpty()) {
            d->name->setError(tr("Username can't be empty, please fill it"));
            return;
        }

        d->name->setError({});
        emit nameChanged(d->name->text());
    });
    //
    connect(d->description, &TextField::textChanged, &d->changeDescriptionDebouncer,
            &Debouncer::orderWork);
    connect(&d->changeDescriptionDebouncer, &Debouncer::gotWork, this,
            [this] { emit descriptionChanged(d->description->text()); });
    //
    connect(d->avatar, &ImageCard::imageChanged, this, &AccountView::avatarChanged);

    //
    // Подписка
    //

    //
    // Сессии
    //
}

AccountView::~AccountView() = default;

void AccountView::showAccount()
{
    d->scrollToTitle(d->accountTitle);
}

void AccountView::showSubscription()
{
    d->scrollToTitle(d->subscriptionTitle);
}

void AccountView::showSessions()
{
    d->scrollToTitle(d->sessionsTitle);
}

void AccountView::setEmail(const QString& _email)
{
    d->accountTitle->setText(_email);
}

void AccountView::setName(const QString& _name)
{
    if (d->name->text() == _name) {
        return;
    }

    QSignalBlocker blocker(d->name);
    d->name->setText(_name);
}

void AccountView::setDescription(const QString& _description)
{
    if (d->description->text() == _description) {
        return;
    }

    QSignalBlocker blocker(d->description);
    d->description->setText(_description);
}

void AccountView::setAvatar(const QPixmap& _avatar)
{
    QSignalBlocker blocker(d->avatar);
    d->avatar->setImage(_avatar);
}

void AccountView::setSubscriptionInfo(Domain::SubscriptionType _subscriptionType,
                                      const QDateTime& _subscriptionEnds)
{
}

void AccountView::setSessions(const QVector<Domain::SessionInfo>& _sessions)
{
    while (!d->sessions.isEmpty()) {
        d->sessions.takeLast()->deleteLater();
    }

    auto layout = qobject_cast<QGridLayout*>(d->content->widget()->layout());
    int row = layout->rowCount() - 2;
    int column = 0;
    for (const auto& sessionInfo : _sessions) {
        auto sessionWidget = new SessionWidget(d->content->widget());
        sessionWidget->setSessionInfo(sessionInfo);
        if (_sessions.size() == 1) {
            sessionWidget->hideterminateButton();
        }

        connect(sessionWidget, &SessionWidget::terminateOthersRequested, this,
                [this, sessionInfo, _sessions] {
                    for (const auto& session : _sessions) {
                        if (session.sessionKey == sessionInfo.sessionKey) {
                            continue;
                        }

                        emit terminateSessionRequested(session.sessionKey);
                    }
                });
        connect(sessionWidget, &SessionWidget::terminateRequested, this,
                [this, sessionInfo] { emit terminateSessionRequested(sessionInfo.sessionKey); });

        layout->addWidget(sessionWidget, row, column++);
        if (column > 1) {
            ++row;
            column = 0;
        }

        d->sessions.append(sessionWidget);
    }
}

void AccountView::updateTranslations()
{
    d->name->setLabel(tr("Your name"));
    d->description->setLabel(tr("Your bio"));
    d->avatar->setSupportingText(tr("Add avatar +"), tr("Change avatar..."),
                                 tr("Do you want to delete your avatar?"));
    d->avatar->setImageCroppingText(tr("Select an area for the avatar"));
    d->subscriptionTitle->setText(tr("Subscription type"));
    d->sessionsTitle->setText(tr("Active sessions"));
}

void AccountView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());

    d->colorAnimation.setStartValue(Ui::DesignSystem::color().secondary());

    for (auto card : {
             d->accountInfo,
             d->subscriptionInfo,
         }) {
        card->setBackgroundColor(DesignSystem::color().background());
    }

    auto titleColor = DesignSystem::color().onBackground();
    titleColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    auto titleMargins = Ui::DesignSystem::label().margins().toMargins();
    titleMargins.setBottom(0);
    for (auto title : {
             d->accountTitle,
             d->subscriptionTitle,
         }) {
        title->setBackgroundColor(DesignSystem::color().background());
        title->setTextColor(DesignSystem::color().onBackground());
        title->setContentsMargins(titleMargins);
    }

    for (auto title : {
             d->sessionsTitle,
         }) {
        title->setBackgroundColor(DesignSystem::color().surface());
        title->setTextColor(titleColor);
        title->setContentsMargins(titleMargins);
    }

    for (auto textField : {
             d->name,
             d->description,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    d->avatar->setBackgroundColor(Ui::DesignSystem::color().background());
    d->avatar->setFixedSize((QSizeF(288, 288) * Ui::DesignSystem::scaleFactor()).toSize());

    d->accountInfoLayout->setSpacing(Ui::DesignSystem::layout().px24());
    d->accountInfoLayout->setRowMinimumHeight(d->accountInfoLastRow,
                                              static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->subscriptionInfoLayout->setSpacing(Ui::DesignSystem::layout().px24());
    d->subscriptionInfoLayout->setRowMinimumHeight(
        d->subscriptionInfoLastRow, static_cast<int>(Ui::DesignSystem::layout().px24()));
}

} // namespace Ui
