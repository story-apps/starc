#include "account_view.h"

#include "avatar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>

#include <utils/tools/debouncer.h>

#include <QAction>
#include <QGridLayout>


namespace Ui
{

class AccountView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Debouncer changeNameDebouncer{500};

    FloatingToolBar* toolBar = nullptr;
    QAction* changePasswordAction = nullptr;
    QAction* logoutAction = nullptr;

    Card* userInfo = nullptr;
    QGridLayout* userInfoLayout = nullptr;
    H6Label* email = nullptr;
    TextField* userName = nullptr;
    CheckBox* receiveEmailNotifications = nullptr;
    Avatar* avatar = nullptr;
};

AccountView::Implementation::Implementation(QWidget* _parent)
    : toolBar(new FloatingToolBar(_parent)),
      changePasswordAction(new QAction(toolBar)),
      logoutAction(new QAction(toolBar)),
      userInfo(new Card(_parent)),
      userInfoLayout(new QGridLayout),
      email(new H6Label(userInfo)),
      userName(new TextField(userInfo)),
      receiveEmailNotifications(new CheckBox(userInfo)),
      avatar(new Avatar(userInfo))
{
    changePasswordAction->setIconText("\uf772");
    toolBar->addAction(changePasswordAction);
    logoutAction->setIconText("\uf343");
    toolBar->addAction(logoutAction);

    userInfoLayout->setContentsMargins({});
    userInfoLayout->setSpacing(0);
    userInfoLayout->addWidget(email, 0, 0);
    userInfoLayout->addWidget(userName, 1, 0);
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
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->userInfo);
    layout->addStretch();
    setLayout(layout);

    connect(d->changePasswordAction, &QAction::triggered, this, &AccountView::changePasswordPressed);
    connect(d->logoutAction, &QAction::triggered, this, &AccountView::logoutPressed);

    connect(d->userName, &TextField::textChanged, &d->changeNameDebouncer, &Debouncer::orderWork);
    connect(&d->changeNameDebouncer, &Debouncer::gotWork, this, [this] {
        if (d->userName->text().isEmpty()) {
            d->userName->setError(tr("Username can't be empty, please fill it"));
            return;
        }

        d->userName->setError({});
        emit userNameChanged(d->userName->text());
    });
    connect(d->receiveEmailNotifications, &CheckBox::checkedChanged, this, &AccountView::receiveEmailNotificationsChanged);
    connect(d->avatar, &Avatar::clicked, this, &AccountView::avatarChoosePressed);
}

void AccountView::setEmail(const QString& _email)
{
    d->email->setText(_email);
}

void AccountView::setUserName(const QString& _userName)
{
    QSignalBlocker blocker(d->userName);
    d->userName->setText(_userName);
}

void AccountView::setReceiveEmailNotifications(bool _receive)
{
    QSignalBlocker blocker(d->receiveEmailNotifications);
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
    d->changePasswordAction->setToolTip(tr("Change password"));
    d->logoutAction->setToolTip(tr("Log out"));
    d->userName->setLabel(tr("User name"));
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

    d->userInfo->setBackgroundColor(DesignSystem::color().background());
    d->email->setBackgroundColor(DesignSystem::color().background());
    d->email->setTextColor(DesignSystem::color().onBackground());
    d->email->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    d->userInfoLayout->setRowMinimumHeight(3, static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->receiveEmailNotifications->setBackgroundColor(DesignSystem::color().background());
    d->receiveEmailNotifications->setTextColor(DesignSystem::color().onBackground());
}

} // namespace Ui
