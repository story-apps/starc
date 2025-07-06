#include "create_draft_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>

#include <QBoxLayout>
#include <QStringListModel>


namespace Ui {

namespace {

/**
 * @brief Состояние диалога
 */
enum State {
    AddNew,
    Edit,
};

} // namespace

class CreateDraftDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    Body1Label* draftHint = nullptr;
    TextField* versionName = nullptr;
    ColorPickerPopup* versionColorPopup = nullptr;
    ComboBox* sourceVersion = nullptr;
    QStringListModel* sourceVersionModel = nullptr;
    CheckBox* lockEditingVersion = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* createButton = nullptr;

    State state = AddNew;
};

CreateDraftDialog::Implementation::Implementation(QWidget* _parent)
    : draftHint(new Body1Label(_parent))
    , versionName(new TextField(_parent))
    , versionColorPopup(new ColorPickerPopup(_parent))
    , sourceVersion(new ComboBox(_parent))
    , sourceVersionModel(new QStringListModel(sourceVersion))
    , lockEditingVersion(new CheckBox(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , createButton(new Button(_parent))
{
    versionColorPopup->setColorCanBeDeselected(false);
    versionColorPopup->setSelectedColor(Qt::white);
    versionName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    versionName->setTrailingIcon(u8"\U000F0765");
    versionName->setTrailingIconColor(versionColorPopup->selectedColor());
    sourceVersion->setModel(sourceVersionModel);
    lockEditingVersion->setChecked(false);
    createButton->setEnabled(false);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(createButton);
}


// ****


CreateDraftDialog::CreateDraftDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->createButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    int row = 0;
    contentsLayout()->addWidget(d->draftHint, row++, 0);
    contentsLayout()->addWidget(d->versionName, row++, 0);
    contentsLayout()->addWidget(d->sourceVersion, row++, 0);
    contentsLayout()->addWidget(d->lockEditingVersion, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);

    connect(d->versionName, &TextField::textChanged, this,
            [this] { d->createButton->setEnabled(!d->versionName->text().isEmpty()); });
    connect(d->versionName, &TextField::trailingIconPressed, this, [this] {
        d->versionColorPopup->showPopup(d->versionName, Qt::AlignBottom | Qt::AlignRight);
    });
    connect(d->versionColorPopup, &ColorPickerPopup::selectedColorChanged, this,
            [this](const QColor& _color) { d->versionName->setTrailingIconColor(_color); });
    connect(d->createButton, &Button::clicked, this, [this] {
        emit savePressed(d->versionName->text(), d->versionColorPopup->selectedColor(),
                         d->sourceVersion->currentIndex().row(),
                         d->lockEditingVersion->isChecked());
    });
    connect(d->cancelButton, &Button::clicked, this, &CreateDraftDialog::hideDialog);
}

CreateDraftDialog::~CreateDraftDialog() = default;

void CreateDraftDialog::setVersions(const QStringList& _versions, int _selectVersionIndex)
{
    d->sourceVersion->setVisible(_versions.size() > 1);

    d->sourceVersionModel->setStringList(_versions);
    d->sourceVersion->setCurrentText(_versions.at(_selectVersionIndex));
}

void CreateDraftDialog::edit(const QString& _name, const QColor& _color, bool _readOnly)
{
    d->state = Edit;
    updateTranslations();

    d->versionName->setText(_name);
    d->versionName->setTrailingIconColor(_color);
    d->sourceVersion->hide();
    d->versionColorPopup->setSelectedColor(_color);
    d->lockEditingVersion->setChecked(_readOnly);
}

QWidget* CreateDraftDialog::focusedWidgetAfterShow() const
{
    return d->versionName;
}

QWidget* CreateDraftDialog::lastFocusableWidget() const
{
    return d->createButton->isEnabled() ? d->createButton : d->cancelButton;
}

void CreateDraftDialog::updateTranslations()
{
    setTitle(d->state == AddNew ? tr("Create document draft") : tr("Edit document draft"));

    d->draftHint->setText(tr("Store actual draft as a separate document to keep your progress."));
    d->versionName->setLabel(tr("Draft name"));
    d->sourceVersion->setLabel(tr("Based on"));
    d->lockEditingVersion->setText(tr("Lock draft text editing"));
    d->cancelButton->setText(tr("Cancel"));
    d->createButton->setText(d->state == AddNew ? tr("Create") : tr("Save"));
}

void CreateDraftDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->draftHint->setContentsMargins(DesignSystem::layout().px24(), 0,
                                     DesignSystem::layout().px16(), DesignSystem::layout().px24());
    d->draftHint->setBackgroundColor(DesignSystem::color().background());
    d->draftHint->setTextColor(DesignSystem::color().onBackground());
    d->versionName->setTextColor(DesignSystem::color().onBackground());
    d->versionName->setBackgroundColor(DesignSystem::color().onBackground());
    d->versionColorPopup->setBackgroundColor(DesignSystem::color().background());
    d->versionColorPopup->setTextColor(DesignSystem::color().onBackground());
    d->sourceVersion->setTextColor(DesignSystem::color().onBackground());
    d->sourceVersion->setBackgroundColor(DesignSystem::color().onBackground());
    d->sourceVersion->setPopupBackgroundColor(DesignSystem::color().background());
    d->sourceVersion->setCustomMargins({ DesignSystem::layout().px24(),
                                         DesignSystem::layout().px12(),
                                         DesignSystem::layout().px24(), 0.0 });
    d->lockEditingVersion->setTextColor(DesignSystem::color().onBackground());
    d->lockEditingVersion->setBackgroundColor(DesignSystem::color().background());

    for (auto button : { d->cancelButton, d->createButton }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
    }

    d->buttonsLayout->setContentsMargins(
        QMarginsF(DesignSystem::layout().px12(), DesignSystem::layout().px12(),
                  DesignSystem::layout().px16(), DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
