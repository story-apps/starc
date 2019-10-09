#include "account_manager.h"

#include <ui/account/login_dialog.h>

#include <QWidget>


namespace ManagementLayer
{

class AccountManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    QWidget* topLevelWidget = nullptr;
    QWidget* toolBar = nullptr;
    QWidget* navigator = nullptr;
    QWidget* view = nullptr;
};

AccountManager::Implementation::Implementation(QWidget* _parent)
    : topLevelWidget(_parent),
      toolBar(new QWidget(topLevelWidget)),
      navigator(new QWidget(topLevelWidget)),
      view(new QWidget(topLevelWidget))
{
}


// ****


AccountManager::AccountManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{
}

AccountManager::~AccountManager() = default;

QWidget* AccountManager::toolBar() const
{
    return d->toolBar;
}

QWidget* AccountManager::navigator() const
{
    return d->navigator;
}

QWidget* AccountManager::view() const
{
    return d->view;
}

void AccountManager::authorize()
{
    Ui::LoginDialog* dlg = new Ui::LoginDialog(d->topLevelWidget);
    dlg->showDialog();
}

} // namespace ManagementLayer
