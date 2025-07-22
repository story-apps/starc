#include "team_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/image/image_card.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QBoxLayout>
#include <QEvent>


namespace Ui {

class TeamDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief настроить размер виджета с фоткой персонажа
     */
    void updateTeamMainPhotoSize();


    DialogType dialogType = DialogType::CreateNew;

    TextField* teamName = nullptr;
    TextField* teamDescription = nullptr;
    ImageCard* teamAvatar = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* saveButton = nullptr;
};

TeamDialog::Implementation::Implementation(QWidget* _parent)
    : teamName(new TextField(_parent))
    , teamDescription(new TextField(_parent))
    , teamAvatar(new ImageCard(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , saveButton(new Button(_parent))
{
    teamName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    teamAvatar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    teamAvatar->setDecorationIcon(u8"\U000F0381");
    UiHelper::initSpellingFor(teamDescription);
    UiHelper::initOptionsFor(teamDescription);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton, 0, Qt::AlignVCenter);
    buttonsLayout->addWidget(saveButton, 0, Qt::AlignVCenter);
}

void TeamDialog::Implementation::updateTeamMainPhotoSize()
{
    teamAvatar->setFixedHeight(teamAvatar->width());
}


// ****


TeamDialog::TeamDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    d->teamAvatar->installEventFilter(this);

    setAcceptButton(d->saveButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    int row = 0;
    contentsLayout()->addWidget(d->teamName, row++, 0, 1, 2);
    contentsLayout()->addWidget(d->teamDescription, row++, 0, 1, 2);
    contentsLayout()->setRowStretch(row++, 1);
    contentsLayout()->addWidget(d->teamAvatar, 0, 2, row++, 1, Qt::AlignTop);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0, 1, 3);
    contentsLayout()->setColumnStretch(0, 2);
    contentsLayout()->setColumnStretch(1, 4);
    contentsLayout()->setColumnStretch(2, 3);


    connect(d->teamName, &TextField::textChanged, d->teamName, &TextField::clearError);
    connect(d->cancelButton, &Button::clicked, this, &TeamDialog::hideDialog);
    connect(d->saveButton, &Button::clicked, this, &TeamDialog::savePressed);
}

TeamDialog::~TeamDialog() = default;

void TeamDialog::setDialogType(DialogType _type)
{
    if (d->dialogType == _type) {
        return;
    }

    d->dialogType = _type;
    updateTranslations();
}

QString TeamDialog::teamName() const
{
    return d->teamName->text();
}

void TeamDialog::setTeamName(const QString& _name)
{
    d->teamName->setText(_name);
}

void TeamDialog::setTeamNameError(const QString& _error)
{
    d->teamName->setError(_error);
}

QString TeamDialog::teamDescription() const
{
    return d->teamDescription->text();
}

void TeamDialog::setteamDescription(const QString& _description)
{
    d->teamDescription->setText(_description);
}

QPixmap TeamDialog::teamAvatar() const
{
    return d->teamAvatar->image();
}

void TeamDialog::setTeamAvatar(const QPixmap& _photo)
{
    d->teamAvatar->setImage(_photo);
}

bool TeamDialog::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == d->teamAvatar && _event->type() == QEvent::Resize) {
        d->updateTeamMainPhotoSize();
    }

    return AbstractDialog::eventFilter(_watched, _event);
}

QWidget* TeamDialog::focusedWidgetAfterShow() const
{
    return d->teamName;
}

QWidget* TeamDialog::lastFocusableWidget() const
{
    return d->saveButton;
}

void TeamDialog::updateTranslations()
{
    setTitle(d->dialogType == DialogType::CreateNew ? tr("Create new team") : tr("Edit team"));
    d->teamName->setLabel(tr("Name"));
    d->teamDescription->setLabel(tr("Description"));
    d->teamAvatar->setSupportingText(tr("Add avatar +"), tr("Change avatar..."),
                                     tr("Do you want to delete the team's avatar?"));
    d->teamAvatar->setImageCroppingText(tr("Select an area for the team avatar"));
    d->cancelButton->setText(tr("Cancel"));
    d->saveButton->setText(d->dialogType == DialogType::CreateNew ? tr("Create") : tr("Update"));
}

void TeamDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentFixedWidth(DesignSystem::layout().px(800));

    for (auto textField : std::vector<TextField*>{
             d->teamName,
             d->teamDescription,
         }) {
        textField->setTextColor(DesignSystem::color().onBackground());
        textField->setBackgroundColor(DesignSystem::color().onBackground());
        textField->setDefaultMarginsEnabled(false);
    }
    d->teamName->setCustomMargins({ 0.0, DesignSystem::card().shadowMargins().top(), 0.0, 0.0 });

    d->teamAvatar->setBackgroundColor(DesignSystem::color().background());
    d->teamAvatar->setTextColor(DesignSystem::color().onBackground());
    d->teamAvatar->setMaximumWidth(DesignSystem::layout().px(340));
    d->updateTeamMainPhotoSize();

    UiHelper::initColorsFor(d->cancelButton, UiHelper::DialogDefault);
    UiHelper::initColorsFor(d->saveButton, UiHelper::DialogAccept);

    d->buttonsLayout->setContentsMargins(
        QMarginsF(DesignSystem::layout().px12(), DesignSystem::layout().px12(),
                  DesignSystem::layout().px16(), DesignSystem::layout().px12())
            .toMargins());
    contentsLayout()->setVerticalSpacing(DesignSystem::compactLayout().px16());
    contentsLayout()->setHorizontalSpacing(DesignSystem::compactLayout().px16());
    contentsLayout()->setContentsMargins(DesignSystem::layout().px24(), 0,
                                         DesignSystem::layout().px12(), 0);
}

} // namespace Ui
