#include "crash_report_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/text_field/text_field.h>

#include <QBoxLayout>


namespace Ui {

class CrashReportDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Body1Label* intro = nullptr;

    AbstractLabel* frequencyQuestion = nullptr;
    RadioButton* frequencyOnce = nullptr;
    RadioButton* frequencyTimeToTime = nullptr;
    RadioButton* frequencyOften = nullptr;
    RadioButton* frequencyOther = nullptr;
    TextField* frequencyOtherDetails = nullptr;

    AbstractLabel* crashSourceQuestion = nullptr;
    RadioButton* crashSourceDontKnow = nullptr;
    RadioButton* crashSourceAllFiles = nullptr;
    RadioButton* crashSourceSingleFile = nullptr;

    TextField* crashDetails = nullptr;
    TextField* contactEmail = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* sendButton = nullptr;
};

CrashReportDialog::Implementation::Implementation(QWidget* _parent)
    : intro(new Body1Label(_parent))
    , frequencyQuestion(new Subtitle1Label(_parent))
    , frequencyOnce(new RadioButton(_parent))
    , frequencyTimeToTime(new RadioButton(_parent))
    , frequencyOften(new RadioButton(_parent))
    , frequencyOther(new RadioButton(_parent))
    , frequencyOtherDetails(new TextField(_parent))
    , crashSourceQuestion(new Subtitle1Label(_parent))
    , crashSourceDontKnow(new RadioButton(_parent))
    , crashSourceAllFiles(new RadioButton(_parent))
    , crashSourceSingleFile(new RadioButton(_parent))
    , crashDetails(new TextField(_parent))
    , contactEmail(new TextField(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , sendButton(new Button(_parent))
{
    auto frequencyGroup = new RadioButtonGroup(_parent);
    frequencyGroup->add(frequencyOnce);
    frequencyGroup->add(frequencyTimeToTime);
    frequencyGroup->add(frequencyOften);
    frequencyGroup->add(frequencyOther);
    frequencyOnce->setChecked(true);
    frequencyOtherDetails->hide();

    auto crashSourceGroup = new RadioButtonGroup(_parent);
    crashSourceGroup->add(crashSourceDontKnow);
    crashSourceGroup->add(crashSourceAllFiles);
    crashSourceGroup->add(crashSourceSingleFile);
    crashSourceDontKnow->setChecked(true);

    crashDetails->setEnterMakesNewLine(true);

    contactEmail->setCapitalizeWords(false);
    contactEmail->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton, 0, Qt::AlignVCenter);
    buttonsLayout->addWidget(sendButton, 0, Qt::AlignVCenter);
}


// ****


CrashReportDialog::CrashReportDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->sendButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    int row = 0;
    contentsLayout()->addWidget(d->intro, row++, 0);
    contentsLayout()->addWidget(d->frequencyQuestion, row++, 0);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(d->frequencyOnce);
        layout->addWidget(d->frequencyTimeToTime);
        layout->addWidget(d->frequencyOften);
        layout->addWidget(d->frequencyOther);
        layout->addStretch();
        contentsLayout()->addLayout(layout, row++, 0);
    }
    contentsLayout()->addWidget(d->frequencyOtherDetails, row++, 0);
    contentsLayout()->addWidget(d->crashSourceQuestion, row++, 0);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(d->crashSourceDontKnow);
        layout->addWidget(d->crashSourceAllFiles);
        layout->addWidget(d->crashSourceSingleFile);
        layout->addStretch();
        contentsLayout()->addLayout(layout, row++, 0);
    }
    contentsLayout()->addWidget(d->crashDetails, row++, 0);
    contentsLayout()->addWidget(d->contactEmail, row++, 0);
    contentsLayout()->setRowStretch(row++, 1);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);

    connect(d->frequencyOther, &RadioButton::checkedChanged, d->frequencyOtherDetails,
            &TextField::setVisible);
    connect(d->cancelButton, &Button::clicked, this, [this] {
        d->crashDetails->setText("User decided to ignore filling error reason");
        emit sendReportPressed();
    });
    connect(d->sendButton, &Button::clicked, this, &CrashReportDialog::sendReportPressed);
}

CrashReportDialog::~CrashReportDialog() = default;

QString CrashReportDialog::frequency() const
{
    if (d->frequencyOnce->isChecked()) {
        return "once";
    } else if (d->frequencyTimeToTime->isChecked()) {
        return "time to time";
    } else if (d->frequencyOften->isChecked()) {
        return "often";
    } else {
        return d->frequencyOtherDetails->text();
    }
}

QString CrashReportDialog::crashSource() const
{
    if (d->crashSourceAllFiles->isChecked()) {
        return "all files";
    } else if (d->crashSourceSingleFile->isChecked()) {
        return "single file";
    } else {
        return "undefined";
    }
}

QString CrashReportDialog::crashDetails() const
{
    return d->crashDetails->text();
}

QString CrashReportDialog::contactEmail() const
{
    return d->contactEmail->text();
}

void CrashReportDialog::setContactEmail(const QString& _email)
{
    d->contactEmail->setText(_email);
}

QWidget* CrashReportDialog::focusedWidgetAfterShow() const
{
    return d->frequencyOnce;
}

QWidget* CrashReportDialog::lastFocusableWidget() const
{
    return d->sendButton;
}

void CrashReportDialog::updateTranslations()
{
    setTitle(tr("Oops.. looks like a crash has happened"));

    d->intro->setText(tr("We are sorry that someting went wrong. Please help us fix this issue and "
                         "share some details so it'll never bother you again."));
    d->frequencyQuestion->setText(tr("How often do crashes happen?"));
    d->frequencyOnce->setText(tr("Once"));
    d->frequencyTimeToTime->setText(tr("Time to time"));
    d->frequencyOften->setText(tr("Often"));
    d->frequencyOther->setText(tr("Other"));
    //    d->frequencyOtherDetails->setLabel(tr("Describe here"));
    d->crashSourceQuestion->setText(
        tr("Does the issue happen within all projects or with the last edited project?"));
    d->crashSourceDontKnow->setText(tr("I don't know"));
    d->crashSourceAllFiles->setText(tr("All projects"));
    d->crashSourceSingleFile->setText(tr("Single project"));
    d->crashDetails->setLabel(tr("What did you do when this issue happened?"));
    d->contactEmail->setLabel(tr("Contact e-mail"));
    d->cancelButton->setText(tr("Ignore error"));
    d->sendButton->setText(tr("Send report"));
}

void CrashReportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    for (auto widget : std::vector<Widget*>{
             d->intro,
             d->frequencyQuestion,
             d->frequencyOnce,
             d->frequencyTimeToTime,
             d->frequencyOften,
             d->frequencyOther,
             d->crashSourceQuestion,
             d->crashSourceDontKnow,
             d->crashSourceAllFiles,
             d->crashSourceSingleFile,
         }) {
        widget->setTextColor(Ui::DesignSystem::color().onBackground());
        widget->setBackgroundColor(Ui::DesignSystem::color().background());
    }

    for (auto textField : std::vector<TextField*>{
             d->frequencyOtherDetails,
             d->crashDetails,
             d->contactEmail,
         }) {
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto button : { d->cancelButton, d->sendButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    d->intro->setContentsMargins(Ui::DesignSystem::layout().px24(), 0,
                                 Ui::DesignSystem::layout().px24(),
                                 Ui::DesignSystem::layout().px24());
    d->frequencyQuestion->setContentsMargins(Ui::DesignSystem::layout().px24(), 0,
                                             Ui::DesignSystem::layout().px24(), 0);
    d->crashSourceQuestion->setContentsMargins(Ui::DesignSystem::layout().px24(),
                                               Ui::DesignSystem::layout().px16(),
                                               Ui::DesignSystem::layout().px24(), 0);

    contentsLayout()->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px12())
            .toMargins());
}

} // namespace Ui
