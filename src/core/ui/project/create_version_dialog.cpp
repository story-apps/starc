#include "create_version_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/combo_box/combo_box.h>
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

class CreateVersionDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    TextField* versionName = nullptr;
    ColorPickerPopup* versionColorPopup = nullptr;
    ComboBox* sourceVersion = nullptr;
    QStringListModel* sourceVersionModel = nullptr;
    CheckBox* allowEditVersion = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* createButton = nullptr;

    State state = AddNew;
};

CreateVersionDialog::Implementation::Implementation(QWidget* _parent)
    : versionName(new TextField(_parent))
    , versionColorPopup(new ColorPickerPopup(_parent))
    , sourceVersion(new ComboBox(_parent))
    , sourceVersionModel(new QStringListModel(sourceVersion))
    , allowEditVersion(new CheckBox(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , createButton(new Button(_parent))
{
    versionColorPopup->setColorCanBeDeselected(false);
    versionColorPopup->setSelectedColor(Qt::red);
    versionName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    versionName->setTrailingIcon(u8"\U000F0765");
    versionName->setTrailingIconColor(versionColorPopup->selectedColor());
    sourceVersion->setModel(sourceVersionModel);
    createButton->setEnabled(false);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(createButton);
}


// ****


CreateVersionDialog::CreateVersionDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->createButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    int row = 0;
    contentsLayout()->addWidget(d->versionName, row++, 0);
    contentsLayout()->addWidget(d->sourceVersion, row++, 0);
    contentsLayout()->addWidget(d->allowEditVersion, row++, 0);
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
                         d->sourceVersion->currentIndex().row(), !d->allowEditVersion->isChecked());
    });
    connect(d->cancelButton, &Button::clicked, this, &CreateVersionDialog::hideDialog);
}

CreateVersionDialog::~CreateVersionDialog() = default;

void CreateVersionDialog::setVersions(const QStringList& _versions, int _selectVersionIndex)
{
    d->sourceVersion->setVisible(_versions.size() > 1);

    d->sourceVersionModel->setStringList(_versions);
    d->sourceVersion->setCurrentText(_versions.at(_selectVersionIndex));
}

void CreateVersionDialog::edit(const QString& _name, const QColor& _color, bool _readOnly)
{
    d->state = Edit;
    updateTranslations();

    d->versionName->setText(_name);
    d->versionName->setTrailingIconColor(_color);
    d->sourceVersion->hide();
    d->versionColorPopup->setSelectedColor(_color);
    d->allowEditVersion->setChecked(!_readOnly);
}

QWidget* CreateVersionDialog::focusedWidgetAfterShow() const
{
    return d->versionName;
}

QWidget* CreateVersionDialog::lastFocusableWidget() const
{
    return d->createButton;
}

void CreateVersionDialog::updateTranslations()
{
    setTitle(d->state == AddNew ? tr("Create new document draft") : tr("Edit document draft"));

    d->versionName->setLabel(tr("Draft name"));
    d->sourceVersion->setLabel(tr("New draft based on"));
    d->allowEditVersion->setText(tr("Allow to edit draft"));
    d->cancelButton->setText(tr("Cancel"));
    d->createButton->setText(d->state == AddNew ? tr("Create") : tr("Save"));
}

void CreateVersionDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->versionName->setTextColor(Ui::DesignSystem::color().onBackground());
    d->versionName->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    d->versionColorPopup->setBackgroundColor(Ui::DesignSystem::color().background());
    d->versionColorPopup->setTextColor(Ui::DesignSystem::color().onBackground());
    d->sourceVersion->setTextColor(Ui::DesignSystem::color().onBackground());
    d->sourceVersion->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    d->sourceVersion->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    d->sourceVersion->setCustomMargins({ Ui::DesignSystem::layout().px24(),
                                         Ui::DesignSystem::layout().px12(),
                                         Ui::DesignSystem::layout().px24(), 0.0 });
    d->allowEditVersion->setTextColor(Ui::DesignSystem::color().onBackground());
    d->allowEditVersion->setBackgroundColor(Ui::DesignSystem::color().background());

    for (auto button : { d->cancelButton, d->createButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
