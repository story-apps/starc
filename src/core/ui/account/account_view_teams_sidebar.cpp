#include "account_view_teams_sidebar.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/tools/debouncer.h>

#include <QVBoxLayout>


namespace Ui {

class AccountViewTeamsSidebar::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    int currentMemberRole = -1;

    Widget* emptyPage = nullptr;

    Widget* memberPage = nullptr;
    Subtitle2Label* memberTitle = nullptr;
    TextField* email = nullptr;
    TextField* nameForTeam = nullptr;
    CaptionLabel* accessTitle = nullptr;
    RadioButton* accessToAllProjects = nullptr;
    RadioButton* accessToSharedProjects = nullptr;
    CaptionLabel* permissionsTitle = nullptr;
    CheckBox* allowGrantAccessToProjects = nullptr;

    Debouncer changeDebouncer;
};

AccountViewTeamsSidebar::Implementation::Implementation(QWidget* _parent)
    : emptyPage(new Widget(_parent))
    , memberPage(new Widget(_parent))
    , memberTitle(new Subtitle2Label(_parent))
    , email(new TextField(_parent))
    , nameForTeam(new TextField(_parent))
    , accessTitle(new CaptionLabel(_parent))
    , accessToAllProjects(new RadioButton(_parent))
    , accessToSharedProjects(new RadioButton(_parent))
    , permissionsTitle(new CaptionLabel(_parent))
    , allowGrantAccessToProjects(new CheckBox(_parent))
    , changeDebouncer(500)
{
    email->setEnabled(false);
    nameForTeam->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    nameForTeam->setCapitalizeWords(false);
    accessToSharedProjects->setChecked(true);
    auto accessGroup = new RadioButtonGroup(_parent);
    accessGroup->add(accessToAllProjects);
    accessGroup->add(accessToSharedProjects);

    auto layout = new QVBoxLayout;
    layout->addWidget(memberTitle);
    layout->addWidget(email);
    layout->addWidget(nameForTeam);
    layout->addWidget(accessTitle);
    layout->addWidget(accessToAllProjects);
    layout->addWidget(accessToSharedProjects);
    layout->addWidget(permissionsTitle);
    layout->addWidget(allowGrantAccessToProjects);
    layout->addStretch();
    memberPage->setLayout(layout);
}


// ****


AccountViewTeamsSidebar::AccountViewTeamsSidebar(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setAnimationType(StackWidget::AnimationType::Slide);

    setCurrentWidget(d->emptyPage);
    addWidget(d->memberPage);


    connect(&d->changeDebouncer, &Debouncer::gotWork, this, [this] {
        emit teamMemberChanged(d->email->text(), d->nameForTeam->text(), d->currentMemberRole,
                               d->accessToAllProjects->isChecked(),
                               d->allowGrantAccessToProjects->isChecked());
    });
    connect(d->nameForTeam, &TextField::textChanged, &d->changeDebouncer, &Debouncer::orderWork);
    connect(d->accessToAllProjects, &RadioButton::checkedChanged, &d->changeDebouncer,
            &Debouncer::orderWork);
    connect(d->allowGrantAccessToProjects, &CheckBox::checkedChanged, &d->changeDebouncer,
            &Debouncer::orderWork);
}

AccountViewTeamsSidebar::~AccountViewTeamsSidebar() = default;

void AccountViewTeamsSidebar::showEmptyPage()
{
    setCurrentWidget(d->emptyPage);
}

void AccountViewTeamsSidebar::setTeamMember(const Domain::TeamMemberInfo& _member)
{
    //
    // Если были изменения для предыдущего установленного участника команды - отправляем их
    //
    d->changeDebouncer.forceWork();

    //
    // Разрешаем изменять пермишены только для участников, но не владельца
    //
    d->currentMemberRole = _member.role;
    const auto isMember = d->currentMemberRole != 0;
    d->accessToAllProjects->setEnabled(isMember);
    d->accessToSharedProjects->setEnabled(isMember);
    d->allowGrantAccessToProjects->setEnabled(isMember);

    //
    // Собственно отображение параметров
    //
    d->changeDebouncer.setAcceptWorkOrders(false);
    //
    if (d->email->text() != _member.email) {
        d->email->setText(_member.email);
    }
    if (d->nameForTeam->text() != _member.nameForTeam) {
        d->nameForTeam->setText(_member.nameForTeam);
    }
    if (_member.hasAccessToAllProjects) {
        d->accessToAllProjects->setChecked(true);
    } else {
        d->accessToSharedProjects->setChecked(true);
    }
    d->allowGrantAccessToProjects->setChecked(_member.allowGrantAccessToProjects);
    //
    d->changeDebouncer.setAcceptWorkOrders(true);

    setCurrentWidget(d->memberPage);
}

void AccountViewTeamsSidebar::updateTranslations()
{
    d->memberTitle->setText("Member parameters");
    d->email->setLabel(tr("Email"));
    d->nameForTeam->setLabel(tr("Name for team"));
    d->accessTitle->setText(tr("Access to team projects"));
    d->accessToAllProjects->setText(tr("All projects"));
    d->accessToSharedProjects->setText(tr("Only shared projects"));
    d->permissionsTitle->setText(tr("Account permissions"));
    d->allowGrantAccessToProjects->setText(tr("Allow grant access to projects"));
}

void AccountViewTeamsSidebar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    d->emptyPage->setBackgroundColor(DesignSystem::color().primary());
    d->memberPage->setBackgroundColor(DesignSystem::color().primary());

    for (auto widget : std::vector<Widget*>{
             d->memberTitle,
             d->accessTitle,
             d->accessToAllProjects,
             d->accessToSharedProjects,
             d->permissionsTitle,
             d->allowGrantAccessToProjects,
         }) {
        widget->setBackgroundColor(DesignSystem::color().primary());
        widget->setTextColor(DesignSystem::color().onPrimary());
    }

    d->memberTitle->setContentsMarginsF(DesignSystem::label().margins());
    const QMarginsF labelMargins(DesignSystem::layout().px24(),
                                 DesignSystem::compactLayout().px16(),
                                 DesignSystem::layout().px24(), 0);
    d->accessTitle->setContentsMarginsF(labelMargins);
    d->permissionsTitle->setContentsMarginsF(labelMargins);

    for (auto textField : {
             d->email,
             d->nameForTeam,
         }) {
        textField->setBackgroundColor(DesignSystem::color().onPrimary());
        textField->setTextColor(DesignSystem::color().onPrimary());
    }
    d->nameForTeam->setCustomMargins({ DesignSystem::layout().px24(),
                                       DesignSystem::compactLayout().px16(),
                                       DesignSystem::layout().px24(), 0 });
}

} // namespace Ui
