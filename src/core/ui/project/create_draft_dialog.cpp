#include "create_draft_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/ui_helper.h>

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
    TextField* draftName = nullptr;
    ColorPickerPopup* draftColorPopup = nullptr;
    ComboBox* sourceDraft = nullptr;
    QStringListModel* sourceDraftModel = nullptr;
    CheckBox* lockEditingDraft = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* createButton = nullptr;

    State state = AddNew;
};

CreateDraftDialog::Implementation::Implementation(QWidget* _parent)
    : draftHint(new Body1Label(_parent))
    , draftName(new TextField(_parent))
    , draftColorPopup(new ColorPickerPopup(_parent))
    , sourceDraft(new ComboBox(_parent))
    , sourceDraftModel(new QStringListModel(sourceDraft))
    , lockEditingDraft(new CheckBox(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , createButton(new Button(_parent))
{
    draftColorPopup->setColorCanBeDeselected(false);
    draftColorPopup->setSelectedColor(Qt::white);
    draftName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    draftName->setTrailingIcon(u8"\U000F0765");
    draftName->setTrailingIconColor(draftColorPopup->selectedColor());
    sourceDraft->setModel(sourceDraftModel);
    lockEditingDraft->setChecked(false);
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
    contentsLayout()->addWidget(d->draftName, row++, 0);
    contentsLayout()->addWidget(d->sourceDraft, row++, 0);
    contentsLayout()->addWidget(d->lockEditingDraft, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);

    connect(d->draftName, &TextField::textChanged, this,
            [this] { d->createButton->setEnabled(!d->draftName->text().isEmpty()); });
    connect(d->draftName, &TextField::trailingIconPressed, this, [this] {
        d->draftColorPopup->showPopup(d->draftName, Qt::AlignBottom | Qt::AlignRight);
    });
    connect(d->draftColorPopup, &ColorPickerPopup::selectedColorChanged, this,
            [this](const QColor& _color) { d->draftName->setTrailingIconColor(_color); });
    connect(d->createButton, &Button::clicked, this, [this] {
        emit savePressed(d->draftName->text(), d->draftColorPopup->selectedColor(),
                         d->sourceDraft->currentIndex().row(), d->lockEditingDraft->isChecked());
    });
    connect(d->cancelButton, &Button::clicked, this, &CreateDraftDialog::hideDialog);
}

CreateDraftDialog::~CreateDraftDialog() = default;

void CreateDraftDialog::setDrafts(const QStringList& _drafts, int _selectDraftIndex)
{
    d->sourceDraft->setVisible(_drafts.size() > 1);

    d->sourceDraftModel->setStringList(_drafts);
    d->sourceDraft->setCurrentText(_drafts.at(_selectDraftIndex));
}

void CreateDraftDialog::edit(const QString& _name, const QColor& _color, bool _readOnly,
                             bool _comparison)
{
    d->state = Edit;
    updateTranslations();

    d->draftName->setText(_name);
    d->draftName->setTrailingIconColor(_color);
    d->sourceDraft->hide();
    d->draftColorPopup->setSelectedColor(_color);
    d->lockEditingDraft->setChecked(_readOnly);
    d->lockEditingDraft->setVisible(_comparison == false);
}

QWidget* CreateDraftDialog::focusedWidgetAfterShow() const
{
    return d->draftName;
}

QWidget* CreateDraftDialog::lastFocusableWidget() const
{
    return d->createButton->isEnabled() ? d->createButton : d->cancelButton;
}

void CreateDraftDialog::updateTranslations()
{
    setTitle(d->state == AddNew ? tr("Create document draft") : tr("Edit document draft"));

    d->draftHint->setText(tr("Store actual draft as a separate document to keep your progress."));
    d->draftName->setLabel(tr("Draft name"));
    d->sourceDraft->setLabel(tr("Based on"));
    d->lockEditingDraft->setText(tr("Lock draft text editing"));
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
    d->draftName->setTextColor(DesignSystem::color().onBackground());
    d->draftName->setBackgroundColor(DesignSystem::color().onBackground());
    d->draftColorPopup->setBackgroundColor(DesignSystem::color().background());
    d->draftColorPopup->setTextColor(DesignSystem::color().onBackground());
    d->sourceDraft->setTextColor(DesignSystem::color().onBackground());
    d->sourceDraft->setBackgroundColor(DesignSystem::color().onBackground());
    d->sourceDraft->setPopupBackgroundColor(DesignSystem::color().background());
    d->sourceDraft->setCustomMargins({ DesignSystem::layout().px24(), DesignSystem::layout().px12(),
                                       DesignSystem::layout().px24(), 0.0 });
    d->lockEditingDraft->setTextColor(DesignSystem::color().onBackground());
    d->lockEditingDraft->setBackgroundColor(DesignSystem::color().background());

    UiHelper::initColorsFor(d->cancelButton, UiHelper::DialogDefault);
    UiHelper::initColorsFor(d->createButton, UiHelper::DialogAccept);

    d->buttonsLayout->setContentsMargins(
        QMarginsF(DesignSystem::layout().px12(), DesignSystem::layout().px12(),
                  DesignSystem::layout().px16(), DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
