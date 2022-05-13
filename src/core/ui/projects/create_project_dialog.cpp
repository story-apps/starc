#include "create_project_dialog.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/dialog/standard_dialog.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/dialog_helper.h>

#include <QFileDialog>
#include <QGridLayout>
#include <QStandardItemModel>
#include <QTimer>


namespace Ui {

namespace {
const int kTypeRole = Qt::UserRole + 1;
}

class CreateProjectDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    ComboBox* projectType = nullptr;
    QStandardItemModel* projectTypeModel = nullptr;
    TextField* projectName = nullptr;
    RadioButton* localProject = nullptr;
    RadioButton* cloudProject = nullptr;
    Body1Label* cloudProjectCreationNote = nullptr;
    Body1LinkLabel* cloudProjectCreationAction = nullptr;
    Body1Label* cloudProjectCreationActionNote = nullptr;
    TextField* projectFolder = nullptr;
    TextField* importFilePath = nullptr;
    QString importFolder;

    QHBoxLayout* buttonsLayout = nullptr;
    IconButton* advancedSettingsButton = nullptr;
    Button* cancelButton = nullptr;
    Button* createButton = nullptr;
};

CreateProjectDialog::Implementation::Implementation(QWidget* _parent)
    : projectType(new ComboBox(_parent))
    , projectTypeModel(new QStandardItemModel(projectType))
    , projectName(new TextField(_parent))
    , localProject(new RadioButton(_parent))
    , cloudProject(new RadioButton(_parent))
    , cloudProjectCreationNote(new Body1Label(_parent))
    , cloudProjectCreationAction(new Body1LinkLabel(_parent))
    , cloudProjectCreationActionNote(new Body1Label(_parent))
    , projectFolder(new TextField(_parent))
    , importFilePath(new TextField(_parent))
    , buttonsLayout(new QHBoxLayout)
    , advancedSettingsButton(new IconButton(_parent))
    , cancelButton(new Button(_parent))
    , createButton(new Button(_parent))
{
    projectName->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    auto makeItem = [](Domain::DocumentObjectType _type) {
        auto item = new QStandardItem;
        item->setData(static_cast<int>(_type), kTypeRole);
        item->setEditable(false);
        return item;
    };

    projectTypeModel->appendRow(makeItem(Domain::DocumentObjectType::Undefined));
    if (settingsValue(DataStorageLayer::kComponentsScreenplayAvailableKey).toBool()) {
        projectTypeModel->appendRow(makeItem(Domain::DocumentObjectType::Screenplay));
    }
    if (settingsValue(DataStorageLayer::kComponentsComicBookAvailableKey).toBool()) {
        projectTypeModel->appendRow(makeItem(Domain::DocumentObjectType::ComicBook));
    }
    if (settingsValue(DataStorageLayer::kComponentsAudioplayAvailableKey).toBool()) {
        projectTypeModel->appendRow(makeItem(Domain::DocumentObjectType::Audioplay));
    }
    if (settingsValue(DataStorageLayer::kComponentsStageplayAvailableKey).toBool()) {
        projectTypeModel->appendRow(makeItem(Domain::DocumentObjectType::Stageplay));
    }

    localProject->setChecked(true);

    projectType->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    projectType->setModel(projectTypeModel);
    projectFolder->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    projectFolder->setTrailingIcon(u8"\U000f0256");
    importFilePath->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    importFilePath->setTrailingIcon(u8"\U000f0256");
    advancedSettingsButton->setCheckable(true);
    advancedSettingsButton->setIcon(u8"\U000f0493");

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(advancedSettingsButton, Qt::AlignVCenter);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton, 0, Qt::AlignVCenter);
    buttonsLayout->addWidget(createButton, 0, Qt::AlignVCenter);

    RadioButtonGroup* projectLocationGroup = new RadioButtonGroup(_parent);
    projectLocationGroup->add(localProject);
    projectLocationGroup->add(cloudProject);

    projectFolder->hide();
    importFilePath->hide();
}


// ****


CreateProjectDialog::CreateProjectDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->createButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    int row = 0;
    contentsLayout()->addWidget(d->projectType, row++, 0, 1, 2);
    contentsLayout()->addWidget(d->projectName, row++, 0, 1, 2);
    contentsLayout()->addWidget(d->localProject, row++, 0, 1, 2);
    contentsLayout()->addWidget(d->cloudProject, row++, 0, 1, 2);
    contentsLayout()->addWidget(d->cloudProjectCreationNote, row++, 0, 1, 2);
    contentsLayout()->addWidget(d->cloudProjectCreationActionNote, row, 1);
    contentsLayout()->addWidget(d->cloudProjectCreationAction, row++, 0);
    contentsLayout()->setColumnStretch(1, 1);
    contentsLayout()->addWidget(d->projectFolder, row++, 0, 1, 2);
    contentsLayout()->addWidget(d->importFilePath, row++, 0, 1, 2);
    contentsLayout()->setRowStretch(row++, 1);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0, 1, 2);

    connect(d->projectName, &TextField::textChanged, this,
            [this] { d->projectName->setError({}); });
    connect(d->localProject, &RadioButton::checkedChanged, this, [this](bool _checked) {
        d->projectFolder->setVisible(_checked && d->advancedSettingsButton->isChecked());
    });
    connect(d->advancedSettingsButton, &IconButton::checkedChanged, this, [this](bool _checked) {
        d->projectFolder->setVisible(_checked && d->localProject->isChecked());
        d->importFilePath->setVisible(_checked);
    });
    connect(d->projectFolder, &TextField::trailingIconPressed, this, [this] {
        const auto path = QFileDialog::getExistingDirectory(
            this, tr("Choose the folder where new story will be saved"), d->projectFolder->text());
        if (!path.isEmpty()) {
            d->projectFolder->setText(path);
        }
    });
    connect(d->importFilePath, &TextField::trailingIconPressed, this, [this] {
        const auto path = QFileDialog::getOpenFileName(
            this, tr("Choose the file to import"), d->importFolder, DialogHelper::importFilters());
        if (path.isEmpty()) {
            return;
        }

        //
        // Старый вордовский формат не поддерживаем
        //
        if (path.endsWith(".doc", Qt::CaseInsensitive)) {
            StandardDialog::information(parentWidget(), tr("File format not supported"),
                                        tr("Importing from DOC files is not supported. You need to "
                                           "save the file in DOCX format and repeat the import."));
            return;
        }

        //
        // Если всё в порядке, принимаем файл к импорту
        //
        d->importFilePath->setText(path);
    });
    connect(d->cancelButton, &Button::clicked, this, &CreateProjectDialog::hideDialog);
    connect(d->createButton, &Button::clicked, this, [this] {
        //
        // Запрещаем создавать проект с пустым названием
        //
        if (d->projectName->text().isEmpty()) {
            d->projectName->setError(tr("The story's name can't be empty. Fill it, please."));
            d->projectName->setFocus();
            return;
        }

        emit createProjectPressed();
    });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

CreateProjectDialog::~CreateProjectDialog() = default;

int CreateProjectDialog::projectType() const
{
    return d->projectType->currentIndex().data(kTypeRole).toInt();
}

void CreateProjectDialog::setProjectType(int _type)
{
    for (int row = 0; row < d->projectTypeModel->rowCount(); ++row) {
        if (d->projectTypeModel->index(row, 0).data(kTypeRole).toInt() == _type) {
            d->projectType->setCurrentIndex(d->projectTypeModel->index(row, 0));
            break;
        }
    }
}

QString CreateProjectDialog::projectName() const
{
    return d->projectName->text();
}

QString CreateProjectDialog::projectFolder() const
{
    return d->projectFolder->text();
}

void CreateProjectDialog::setProjectFolder(const QString& _path)
{
    if (_path.isEmpty()) {
        return;
    }

    d->projectFolder->setText(_path);
}

QString CreateProjectDialog::importFilePath() const
{
    return d->importFilePath->text();
}

void CreateProjectDialog::setImportFolder(const QString& _path)
{
    d->importFolder = _path;
}

void CreateProjectDialog::configureCloudProjectCreationAbility(bool _isLogged,
                                                               bool _isSubscriptionActive)
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
    d->localProject->hide();
    d->cloudProject->hide();
    //
    // ... и настроим подсказки
    //

    d->cloudProjectCreationNote->setText(tr("The story will be created on the local computer."));
    d->cloudProjectCreationActionNote->setText(tr("to create stories on the cloud."));
    if (_isLogged) {
        d->cloudProjectCreationAction->setText(tr("Renew subscription"));
        connect(d->cloudProjectCreationAction, &Body1LinkLabel::clicked, this,
                &CreateProjectDialog::renewSubscriptionPressed);
    } else {
        d->cloudProjectCreationAction->setText(tr("Sign in"));
        connect(d->cloudProjectCreationAction, &Body1LinkLabel::clicked, this,
                &CreateProjectDialog::loginPressed);
    }

    //
    // FIXME: открыть, когда будет работать облако
    //
    //    d->cloudProjectCreationNote->hide();
    d->cloudProjectCreationAction->hide();
    d->cloudProjectCreationActionNote->hide();
    return;
}

bool CreateProjectDialog::isLocal() const
{
    return d->localProject->isChecked();
}

QWidget* CreateProjectDialog::focusedWidgetAfterShow() const
{
    return d->projectType;
}

QWidget* CreateProjectDialog::lastFocusableWidget() const
{
    return d->createButton;
}

void CreateProjectDialog::updateTranslations()
{
    setTitle(tr("Create new story"));

    d->projectType->setLabel(tr("Type of story"));
    for (int row = 0; row < d->projectTypeModel->rowCount(); ++row) {
        auto item = d->projectTypeModel->item(row);
        switch (static_cast<Domain::DocumentObjectType>(item->data(kTypeRole).toInt())) {
        default:
        case Domain::DocumentObjectType::Undefined: {
            item->setText(tr("Not set"));
            break;
        }

        case Domain::DocumentObjectType::Screenplay: {
            item->setText(tr("Screenplay"));
            break;
        }

        case Domain::DocumentObjectType::ComicBook: {
            item->setText(tr("Comic book"));
            break;
        }

        case Domain::DocumentObjectType::Audioplay: {
            item->setText(tr("Audioplay"));
            break;
        }

        case Domain::DocumentObjectType::Stageplay: {
            item->setText(tr("Stageplay"));
            break;
        }
        }
    }

    d->projectName->setLabel(tr("Name of the story"));
    d->localProject->setText(tr("Save story in the local computer"));
    d->cloudProject->setText(tr("Save story in the cloud"));
    d->projectFolder->setLabel(tr("Location of the new story file"));
    d->projectFolder->setTrailingIconToolTip(
        tr("Choose the folder where the new story will be placed"));
    d->importFilePath->setLabel(tr("Choose file with story to import"));
    d->importFilePath->setTrailingIconToolTip(tr("Choose file for importing"));
    d->advancedSettingsButton->setToolTip(tr("Advanced options"));
    d->cancelButton->setText(tr("Cancel"));
    d->createButton->setText(tr("Create"));
}

void CreateProjectDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    for (auto widget : std::vector<Widget*>{
             d->localProject,
             d->cloudProjectCreationNote,
             d->cloudProject,
             d->cloudProjectCreationAction,
             d->cloudProjectCreationActionNote,
             d->advancedSettingsButton,
         }) {
        widget->setTextColor(Ui::DesignSystem::color().onBackground());
        widget->setBackgroundColor(Ui::DesignSystem::color().background());
    }

    for (auto textField : std::vector<TextField*>{
             d->projectType,
             d->projectName,
             d->projectFolder,
             d->importFilePath,
         }) {
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    }
    for (auto combobox : {
             d->projectType,
         }) {
        combobox->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    }

    d->cloudProjectCreationNote->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px24(),
                                                              Ui::DesignSystem::layout().px12(),
                                                              Ui::DesignSystem::layout().px24(), 0)
                                                        .toMargins());
    d->cloudProjectCreationAction->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px2(),
                  Ui::DesignSystem::layout().px4(), Ui::DesignSystem::layout().px12())
            .toMargins());
    d->cloudProjectCreationAction->setTextColor(Ui::DesignSystem::color().secondary());
    d->cloudProjectCreationActionNote->setContentsMargins(
        QMarginsF(0, Ui::DesignSystem::layout().px2(), Ui::DesignSystem::layout().px24(),
                  Ui::DesignSystem::layout().px12())
            .toMargins());
    for (auto button : { d->cancelButton, d->createButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    contentsLayout()->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px12())
            .toMargins());
}

} // namespace Ui
