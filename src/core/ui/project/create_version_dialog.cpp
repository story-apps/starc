#include "create_version_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/text_field/text_field.h>

#include <QBoxLayout>


namespace Ui {

class CreateVersionDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    TextField* versionName = nullptr;
    ColorPickerPopup* versionColorPopup = nullptr;
    CheckBox* allowEditVersion = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* createButton = nullptr;
};

CreateVersionDialog::Implementation::Implementation(QWidget* _parent)
    : versionName(new TextField(_parent))
    , versionColorPopup(new ColorPickerPopup(_parent))
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
    contentsLayout()->addWidget(d->versionName, 0, 0);
    contentsLayout()->addWidget(d->allowEditVersion, 1, 0);
    contentsLayout()->addLayout(d->buttonsLayout, 2, 0);

    connect(d->versionName, &TextField::textChanged, this,
            [this] { d->createButton->setEnabled(!d->versionName->text().isEmpty()); });
    connect(d->versionName, &TextField::trailingIconPressed, this, [this] {
        d->versionColorPopup->showPopup(d->versionName, Qt::AlignBottom | Qt::AlignRight);
    });
    connect(d->versionColorPopup, &ColorPickerPopup::selectedColorChanged, this,
            [this](const QColor& _color) { d->versionName->setTrailingIconColor(_color); });
    connect(d->createButton, &Button::clicked, this, [this] {
        emit createPressed(d->versionName->text(), d->versionColorPopup->selectedColor(),
                           !d->allowEditVersion->isChecked());
    });
    connect(d->cancelButton, &Button::clicked, this, &CreateVersionDialog::hideDialog);
}

CreateVersionDialog::~CreateVersionDialog() = default;

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
    setTitle(tr("Create new document version"));

    d->versionName->setLabel(tr("New version name"));
    d->allowEditVersion->setText(tr("Allow to edit new version"));
    d->cancelButton->setText(tr("Cancel"));
    d->createButton->setText(tr("Create"));
}

void CreateVersionDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->versionName->setTextColor(Ui::DesignSystem::color().onBackground());
    d->versionName->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    d->versionColorPopup->setBackgroundColor(Ui::DesignSystem::color().background());
    d->versionColorPopup->setTextColor(Ui::DesignSystem::color().onBackground());
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
