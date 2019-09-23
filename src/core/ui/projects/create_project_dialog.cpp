#include "create_project_dialog.h"

#include <ui/design_system/design_system.h>

#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/radio_button/radio_button.h>

#include <QGridLayout>
#include <QTimer>

namespace Ui
{

class CreateProjectDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    TextField* projectName = nullptr;
    RadioButton* localProjectButton = nullptr;
    RadioButton* remoteProjectButton = nullptr;
    TextField* projectFilePath = nullptr;
    TextField* importFilePath = nullptr;
};

CreateProjectDialog::Implementation::Implementation(QWidget* _parent)
    : projectName(new TextField(_parent)),
      localProjectButton(new RadioButton(_parent)),
      remoteProjectButton(new RadioButton(_parent)),
      projectFilePath(new TextField(_parent))
{
    projectFilePath->setTrailingIcon("\uf256");
}


// ****


CreateProjectDialog::CreateProjectDialog(QWidget* _parent)
    : AbstractDialog(_parent),
      d(new Implementation(this))
{
    d->remoteProjectButton->hide();

    contentsLayout()->addWidget(d->projectName);
    contentsLayout()->addWidget(d->localProjectButton);
    contentsLayout()->addWidget(d->remoteProjectButton);
    contentsLayout()->addWidget(d->projectFilePath);
    contentsLayout()->setRowStretch(4, 1);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

void CreateProjectDialog::updateTranslations()
{
    setTitle(tr("Create new story"));

    d->projectName->setLabel(tr("Enter the name of new story"));
    d->localProjectButton->setText(tr("Local project"));
    d->remoteProjectButton->setText(tr("Remote project"));
    d->projectName->setLabel(tr("Location of the new story file"));
}

void CreateProjectDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    d->localProjectButton->setBackgroundColor(Ui::DesignSystem::color().background());
    d->localProjectButton->setTextColor(Ui::DesignSystem::color().onBackground());
    d->remoteProjectButton->setBackgroundColor(Ui::DesignSystem::color().background());
    d->remoteProjectButton->setTextColor(Ui::DesignSystem::color().onBackground());
}

CreateProjectDialog::~CreateProjectDialog() = default;

}
