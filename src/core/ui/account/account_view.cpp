#include "account_view.h"

#include "avatar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>

#include <QAction>
#include <QGridLayout>


namespace Ui
{

class AccountView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    FloatingToolBar* toolBar = nullptr;

    Card* userInfo = nullptr;
    QGridLayout* userInfoLayout = nullptr;
    H6Label* email = nullptr;
    TextField* username = nullptr;
    CheckBox* receiveEmailNotifications = nullptr;
    Avatar* avatar = nullptr;
};

AccountView::Implementation::Implementation(QWidget* _parent)
    : toolBar(new FloatingToolBar(_parent)),
      userInfo(new Card(_parent)),
      userInfoLayout(new QGridLayout),
      email(new H6Label(userInfo)),
      username(new TextField(userInfo)),
      receiveEmailNotifications(new CheckBox(userInfo)),
      avatar(new Avatar(userInfo))
{
    userInfoLayout->setContentsMargins({});
    userInfoLayout->setSpacing(0);
    userInfoLayout->addWidget(email, 0, 0);
    userInfoLayout->addWidget(username, 1, 0);
    userInfoLayout->addWidget(receiveEmailNotifications, 2, 0);
    userInfoLayout->setRowMinimumHeight(3, 1); // добавляем пустую строку, вместо отступа снизу
    userInfoLayout->addWidget(avatar, 0, 1, 4, 1);
    userInfoLayout->setColumnStretch(0, 1);
    userInfo->setLayoutReimpl(userInfoLayout);
}


// ****


AccountView::AccountView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QAction* changePasswordAction = new QAction;
    changePasswordAction->setIconText("\uf772");
    d->toolBar->addAction(changePasswordAction);
    connect(changePasswordAction, &QAction::triggered, this, &AccountView::changePasswordPressed);
    QAction* logoutAction = new QAction;
    logoutAction->setIconText("\uf343");
    d->toolBar->addAction(logoutAction);
    connect(logoutAction, &QAction::triggered, this, &AccountView::logoutPressed);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->userInfo);
    layout->addStretch();
    setLayout(layout);
}

void AccountView::setEmail(const QString& _email)
{
    d->email->setText(_email);
}

void AccountView::setUsername(const QString& _username)
{
    d->username->setText(_username);
}

void AccountView::setReceiveEmailNotifications(bool _receive)
{
    d->receiveEmailNotifications->setChecked(_receive);
}

void AccountView::setAvatar(const QPixmap& _avatar)
{
    d->avatar->setAvatar(_avatar);
}

AccountView::~AccountView() = default;

void AccountView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);
    d->toolBar->move(QPointF(Ui::DesignSystem::layout().px24(),
                             Ui::DesignSystem::layout().px24()).toPoint());
}

void AccountView::updateTranslations()
{
    d->username->setLabel(tr("User name"));
    d->receiveEmailNotifications->setText(tr("Receive email notifications"));
}

void AccountView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    setBackgroundColor(DesignSystem::color().surface());
    layout()->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                                           Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24()).toMargins());

    d->toolBar->resize(d->toolBar->sizeHint());
    d->toolBar->move(QPointF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px24()).toPoint());
    d->toolBar->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->toolBar->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->toolBar->raise();

    d->userInfo->setBackgroundColor(DesignSystem::color().surface());
    d->email->setBackgroundColor(DesignSystem::color().surface());
    d->email->setTextColor(DesignSystem::color().onSurface());
    d->email->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    d->userInfoLayout->setRowMinimumHeight(3, static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->receiveEmailNotifications->setBackgroundColor(DesignSystem::color().surface());
    d->receiveEmailNotifications->setTextColor(DesignSystem::color().onSurface());
}

} // namespace Ui
