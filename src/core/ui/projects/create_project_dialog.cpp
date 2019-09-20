#include "create_project_dialog.h"

#include <ui/widgets/radio_button/radio_button.h>

#include <QGridLayout>
#include <QTimer>

namespace Ui
{

class CreateProjectDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    RadioButton* localProjectButton = nullptr;
    RadioButton* remoteProjectButton = nullptr;
};

CreateProjectDialog::Implementation::Implementation(QWidget* _parent)
    : localProjectButton(new RadioButton(_parent)),
      remoteProjectButton(new RadioButton(_parent))
{
}


// ****


CreateProjectDialog::CreateProjectDialog(QWidget* _parent)
    : AbstractDialog(_parent),
      d(new Implementation(this))
{
    setTitle(tr("Create new story df asd fas fds fa s!"));
    d->localProjectButton->setText(tr("Local project"));
    d->localProjectButton->setBackgroundColor(Qt::white);
    d->localProjectButton->setTextColor(Qt::black);
    d->remoteProjectButton->setText(tr("Remote project"));
    d->remoteProjectButton->setBackgroundColor(Qt::white);
    d->remoteProjectButton->setTextColor(Qt::black);
    d->remoteProjectButton->hide();

    contentsLayout()->addWidget(d->localProjectButton);
    contentsLayout()->addWidget(d->remoteProjectButton);
    contentsLayout()->setRowStretch(2, 1);

    QTimer::singleShot(1000, d->remoteProjectButton, &RadioButton::show);
    QTimer::singleShot(2000, d->remoteProjectButton, &RadioButton::hide);
}

CreateProjectDialog::~CreateProjectDialog() = default;

}
