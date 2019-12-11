#include "create_project_dialog.h"

#include <business_layer/import/abstract_importer.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/dialog/standard_dialog.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/toggle_button/toggle_button.h>

#include <QGridLayout>
#include <QFileDialog>
#include <QTimer>


namespace Ui
{

class CreateProjectDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    TextField* projectName = nullptr;
    RadioButton* localProjectButton = nullptr;
    RadioButton* cloudProjectButton = nullptr;
    Body1Label* cloudProjectCreationNote = nullptr;
    Body1LinkLabel* cloudProjectCreationAction = nullptr;
    Body1Label* cloudProjectCreationActionNote = nullptr;
    TextField* projectFilePath = nullptr;
    TextField* importFilePath = nullptr;
    QString importFolder;

    QHBoxLayout* buttonsLayout = nullptr;
    ToggleButton* advancedSettingsButton = nullptr;
    Button* cancelButton = nullptr;
    Button* createButton = nullptr;
};

CreateProjectDialog::Implementation::Implementation(QWidget* _parent)
    : projectName(new TextField(_parent)),
      localProjectButton(new RadioButton(_parent)),
      cloudProjectButton(new RadioButton(_parent)),
      cloudProjectCreationNote(new Body1Label(_parent)),
      cloudProjectCreationAction(new Body1LinkLabel(_parent)),
      cloudProjectCreationActionNote(new Body1Label(_parent)),
      projectFilePath(new TextField(_parent)),
      importFilePath(new TextField(_parent)),
      advancedSettingsButton(new ToggleButton(_parent)),
      cancelButton(new Button(_parent)),
      createButton(new Button(_parent))
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
    projectLocationGroup->add(cloudProjectButton);

    projectFilePath->hide();
    importFilePath->hide();
}


// ****


CreateProjectDialog::CreateProjectDialog(QWidget* _parent)
    : AbstractDialog(_parent),
      d(new Implementation(this))
{
    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    contentsLayout()->addWidget(d->projectName, 0, 0, 1, 2);
    contentsLayout()->addWidget(d->localProjectButton, 1, 0, 1, 2);
    contentsLayout()->addWidget(d->cloudProjectButton, 2, 0, 1, 2);
    contentsLayout()->addWidget(d->cloudProjectCreationNote, 3, 0, 1, 2);
    contentsLayout()->addWidget(d->cloudProjectCreationActionNote, 4, 1);
    contentsLayout()->addWidget(d->cloudProjectCreationAction, 4, 0);
    contentsLayout()->setColumnStretch(1, 1);
    contentsLayout()->addWidget(d->projectFilePath, 5, 0, 1, 2);
    contentsLayout()->addWidget(d->importFilePath, 6, 0, 1, 2);
    contentsLayout()->setRowStretch(7, 1);
    contentsLayout()->addLayout(d->buttonsLayout, 8, 0, 1, 2);

    connect(d->localProjectButton, &RadioButton::checkedChanged, this, [this] (bool _checked) {
        d->projectFilePath->setVisible(_checked && d->advancedSettingsButton->isChecked());
    });
    connect(d->advancedSettingsButton, &ToggleButton::checkedChanged, this, [this] (bool _checked) {
        d->projectFilePath->setVisible(_checked && d->localProjectButton->isChecked());
        d->importFilePath->setVisible(_checked);
    });
    connect(d->projectFilePath, &TextField::trailingIconPressed, this, [this] {
       const auto path
               = QFileDialog::getExistingDirectory(this,
                    tr("Choose the folder where new story will be saved"), d->projectFilePath->text());
       if (!path.isEmpty()) {
           d->projectFilePath->setText(path);
       }
    });
    connect(d->importFilePath, &TextField::trailingIconPressed, this, [this] {
       const auto path
               = QFileDialog::getOpenFileName(this, tr("Choose the file to import"),
                        d->importFolder, BusinessLayer::AbstractImporter::filters());
       if (path.isEmpty()) {
           return;
       }

       //
       // Старый вордовский формат не поддерживаем
       //
       if (path.toLower().endsWith(".doc")) {
           StandardDialog::information(parentWidget(), tr("File format not supported"),
               tr("Importing from DOC files is not supported. You need to save the file in DOCX format and repeat the import."));
           return;
       }

       //
       // Если всё в порядке, принимаем файл к импорту
       //
       d->importFilePath->setText(path);
    });
    connect(d->createButton, &Button::clicked, this, &CreateProjectDialog::createProjectPressed);
    connect(d->cancelButton, &Button::clicked, this, &CreateProjectDialog::hideDialog);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

void CreateProjectDialog::configureCloudProjectCreationAbility(bool _isLogged, bool _isSubscriptionActive)
{
    //
    // TODO: Обработать вариант, когда пользователь в офлайне
    //       - нужно запретить создавать проект в облаке, но показать другую подсказку
    //

    //
    // Если пользователь может создавать проект в облаке
    //
    if (_isLogged && _isSubscriptionActive) {
        //
        // ... скроем виджеты с посказками
        //
        d->cloudProjectCreationNote->hide();
        d->cloudProjectCreationAction->hide();
        d->cloudProjectCreationActionNote->hide();
        return;
    }

    //
    // Если пользователь не может создавать проект в облаке скроем переключатели хранения
    //
    d->localProjectButton->hide();
    d->cloudProjectButton->hide();
    //
    // ... и настроим подсказки
    //

    d->cloudProjectCreationNote->setText(tr("The story will be created on the local computer."));
    d->cloudProjectCreationActionNote->setText(tr("to create stories on the cloud."));
    if (_isLogged) {
        d->cloudProjectCreationAction->setText(tr("Renew subscription"));
        connect(d->cloudProjectCreationAction, &Body1LinkLabel::clicked,
                this, &CreateProjectDialog::renewSubscriptionPressed);
    } else {
        d->cloudProjectCreationAction->setText(tr("Sign in"));
        connect(d->cloudProjectCreationAction, &Body1LinkLabel::clicked,
                this, &CreateProjectDialog::loginPressed);
    }
}

CreateProjectDialog::~CreateProjectDialog() = default;

QWidget* CreateProjectDialog::focusedWidgetAfterShow() const
{
    return d->projectName;
}

void CreateProjectDialog::updateTranslations()
{
    setTitle(tr("Create new story"));

    d->projectName->setLabel(tr("Enter name of the new story"));
    d->localProjectButton->setText(tr("Place story on the local computer"));
    d->cloudProjectButton->setText(tr("Place story on the cloud"));
    d->projectFilePath->setLabel(tr("Location of the new story file"));
    d->importFilePath->setLabel(tr("Choose file with story to import"));
    d->cancelButton->setText(tr("Cancel"));
    d->createButton->setText(tr("Create"));
}

void CreateProjectDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    for (auto widget : QVector<Widget*>{ d->localProjectButton, d->cloudProjectCreationNote, d->cloudProjectButton,
                                         d->cloudProjectCreationAction,
                                         d->cloudProjectCreationActionNote,
                                         d->advancedSettingsButton}) {
        widget->setTextColor(Ui::DesignSystem::color().onBackground());
        widget->setBackgroundColor(Ui::DesignSystem::color().background());
    }

    d->cloudProjectCreationNote->setContentsMargins(
                QMarginsF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px12(),
                          Ui::DesignSystem::layout().px24(),
                          0)
                .toMargins());
    d->cloudProjectCreationAction->setContentsMargins(
                QMarginsF(Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px2(),
                          0,
                          Ui::DesignSystem::layout().px12())
                .toMargins());
    d->cloudProjectCreationAction->setTextColor(Ui::DesignSystem::color().secondary());
    d->cloudProjectCreationActionNote->setContentsMargins(
                QMarginsF(0,
                          Ui::DesignSystem::layout().px2(),
                          Ui::DesignSystem::layout().px24(),
                          Ui::DesignSystem::layout().px12())
                .toMargins());
    d->cancelButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->cancelButton->setTextColor(Ui::DesignSystem::color().secondary());
    d->createButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->createButton->setTextColor(Ui::DesignSystem::color().secondary());

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px8()).toMargins());
}

} // namespace Ui
