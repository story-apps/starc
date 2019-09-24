#include "create_project_dialog.h"

#include <ui/design_system/design_system.h>

#include <ui/widgets/button/button.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/toggle_button/toggle_button.h>

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

    QHBoxLayout* buttonsLayout = nullptr;
    ToggleButton* advancedSettingsButton = nullptr;
    Button* createButton = nullptr;
    Button* cancelButton = nullptr;
};

CreateProjectDialog::Implementation::Implementation(QWidget* _parent)
    : projectName(new TextField(_parent)),
      localProjectButton(new RadioButton(_parent)),
      remoteProjectButton(new RadioButton(_parent)),
      projectFilePath(new TextField(_parent)),
      importFilePath(new TextField(_parent)),
      advancedSettingsButton(new ToggleButton(_parent)),
      createButton(new Button(_parent)),
      cancelButton(new Button(_parent))
{
    localProjectButton->setChecked(true);

    projectFilePath->setTrailingIcon("\uf256");
    importFilePath->setTrailingIcon("\uf256");
    advancedSettingsButton->setIcon("\uf493");

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(advancedSettingsButton, Qt::AlignVCenter);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(createButton);

    RadioButtonGroup* projectLocationGroup = new RadioButtonGroup(_parent);
    projectLocationGroup->add(localProjectButton);
    projectLocationGroup->add(remoteProjectButton);

    projectFilePath->hide();
    importFilePath->hide();
}


// ****


CreateProjectDialog::CreateProjectDialog(QWidget* _parent)
    : AbstractDialog(_parent),
      d(new Implementation(this))
{
    contentsLayout()->addWidget(d->projectName, 0, 0);
    contentsLayout()->addWidget(d->localProjectButton, 1, 0);
    contentsLayout()->addWidget(d->remoteProjectButton, 2, 0);
    contentsLayout()->addWidget(d->projectFilePath, 3, 0);
    contentsLayout()->addWidget(d->importFilePath, 4, 0);
    contentsLayout()->setRowStretch(5, 1);
    contentsLayout()->addLayout(d->buttonsLayout, 6, 0);

    connect(d->advancedSettingsButton, &ToggleButton::checkedChanged, this, [this] (bool _checked) {
        d->projectFilePath->setVisible(_checked);
        d->importFilePath->setVisible(_checked);
    });
    connect(d->cancelButton, &Button::clicked, this, &CreateProjectDialog::hideDialog);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

QWidget* CreateProjectDialog::focusedWidgetAfterShow() const
{
    return d->projectName;
}

void CreateProjectDialog::updateTranslations()
{
    setTitle(tr("Create new story"));

    d->projectName->setLabel(tr("Enter name of the new story"));
    d->localProjectButton->setText(tr("Local project"));
    d->remoteProjectButton->setText(tr("Remote project"));
    d->projectFilePath->setLabel(tr("Location of the new story file"));
    d->importFilePath->setLabel(tr("Choose file with story to import"));
    d->createButton->setText(tr("Create"));
    d->cancelButton->setText(tr("Cancel"));
}

void CreateProjectDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    d->localProjectButton->setBackgroundColor(Ui::DesignSystem::color().background());
    d->localProjectButton->setTextColor(Ui::DesignSystem::color().onBackground());
    d->remoteProjectButton->setBackgroundColor(Ui::DesignSystem::color().background());
    d->remoteProjectButton->setTextColor(Ui::DesignSystem::color().onBackground());
    d->advancedSettingsButton->setBackgroundColor(Ui::DesignSystem::color().background());
    d->advancedSettingsButton->setTextColor(Ui::DesignSystem::color().onBackground());
    d->createButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->createButton->setTextColor(Ui::DesignSystem::color().secondary());
    d->cancelButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->cancelButton->setTextColor(Ui::DesignSystem::color().secondary());

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px8()).toMargins());
}

CreateProjectDialog::~CreateProjectDialog() = default;

}
