#include "account_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <ui/widgets/image/image_card.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/tools/debouncer.h>

#include <QAction>
#include <QGridLayout>
#include <QScrollArea>


namespace Ui {

class AccountView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QScrollArea* content = nullptr;

    Card* accountInfo = nullptr;
    QGridLayout* accountInfoLayout = nullptr;
    int accountInfoLastRow = 0;
    H6Label* email = nullptr;
    TextField* name = nullptr;
    Debouncer changeNameDebouncer{ 500 };
    TextField* description = nullptr;
    Debouncer changeDescriptionDebouncer{ 500 };

    ImageCard* avatar = nullptr;
};

AccountView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , accountInfo(new Card(_parent))
    , accountInfoLayout(new QGridLayout)
    , email(new H6Label(accountInfo))
    , name(new TextField(accountInfo))
    , description(new TextField(accountInfo))
    , avatar(new ImageCard(accountInfo))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    name->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    description->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    avatar->setDecorationIcon(u8"\U000F0004");

    accountInfoLayout->setContentsMargins({});
    accountInfoLayout->setSpacing(0);
    int row = 0;
    accountInfoLayout->addWidget(email, row++, 0);
    accountInfoLayout->addWidget(name, row++, 0);
    accountInfoLayout->addWidget(description, row++, 0);
    accountInfoLastRow = row;
    accountInfoLayout->setRowMinimumHeight(row++,
                                           1); // добавляем пустую строку, вместо отступа снизу
    accountInfoLayout->addWidget(avatar, 0, 1, row, 1);
    accountInfoLayout->setColumnStretch(0, 1);
    accountInfo->setLayoutReimpl(accountInfoLayout);

    auto contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    auto layout = new QGridLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(accountInfo, 0, 0);
    layout->addWidget(avatar, 0, 1, 2, 1, Qt::AlignTop);
    layout->setRowStretch(2, 1);
    contentWidget->setLayout(layout);
}


// ****


AccountView::AccountView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    layout->addStretch();
    setLayout(layout);

    connect(d->name, &TextField::textChanged, &d->changeNameDebouncer, &Debouncer::orderWork);
    connect(&d->changeNameDebouncer, &Debouncer::gotWork, this, [this] {
        if (d->name->text().isEmpty()) {
            d->name->setError(tr("Username can't be empty, please fill it"));
            return;
        }

        d->name->setError({});
        emit nameChanged(d->name->text());
    });
    //
    connect(d->description, &TextField::textChanged, &d->changeDescriptionDebouncer,
            &Debouncer::orderWork);
    connect(&d->changeDescriptionDebouncer, &Debouncer::gotWork, this,
            [this] { emit descriptionChanged(d->description->text()); });
    //
    connect(d->avatar, &ImageCard::imageChanged, this, &AccountView::avatarChanged);
}

AccountView::~AccountView() = default;

void AccountView::setEmail(const QString& _email)
{
    d->email->setText(_email);
}

void AccountView::setName(const QString& _name)
{
    if (d->name->text() == _name) {
        return;
    }

    QSignalBlocker blocker(d->name);
    d->name->setText(_name);
}

void AccountView::setDescription(const QString& _description)
{
    if (d->description->text() == _description) {
        return;
    }

    QSignalBlocker blocker(d->description);
    d->description->setText(_description);
}

void AccountView::setAvatar(const QPixmap& _avatar)
{
    QSignalBlocker blocker(d->avatar);
    d->avatar->setImage(_avatar);
}

void AccountView::updateTranslations()
{
    d->name->setLabel(tr("Your name"));
    d->description->setLabel(tr("Your bio"));
    d->avatar->setSupportingText(tr("Add avatar +"), tr("Change avatar..."),
                                 tr("Do you want to delete your avatar?"));
    d->avatar->setImageCroppingText(tr("Select an area for the avatar"));
}

void AccountView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());

    for (auto card : { d->accountInfo }) {
        card->setBackgroundColor(DesignSystem::color().background());
    }

    auto titleLabelMargins = Ui::DesignSystem::label().margins();
    titleLabelMargins.setBottom(0);
    for (auto title : { d->email }) {
        title->setBackgroundColor(DesignSystem::color().background());
        title->setTextColor(DesignSystem::color().onBackground());
        title->setContentsMargins(titleLabelMargins.toMargins());
    }
    d->accountInfoLayout->setSpacing(Ui::DesignSystem::layout().px24());
    d->accountInfoLayout->setRowMinimumHeight(d->accountInfoLastRow,
                                              static_cast<int>(Ui::DesignSystem::layout().px24()));

    for (auto textField : { d->name, d->description }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    d->avatar->setBackgroundColor(Ui::DesignSystem::color().background());
    d->avatar->setFixedSize((QSizeF(288, 288) * Ui::DesignSystem::scaleFactor()).toSize());
}

} // namespace Ui
