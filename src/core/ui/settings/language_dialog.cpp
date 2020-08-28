#include "language_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>

#include <QGridLayout>
#include <QUrl>


namespace Ui
{

namespace {
const char* kLanguageKey = "language";
}

class LanguageDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить список всех кнопок языков
     */
    QVector<RadioButton*> languages() const;


    RadioButton* azerbaijani = nullptr;
    RadioButton* english = nullptr;
    RadioButton* german = nullptr;
    RadioButton* hebrew = nullptr;
    RadioButton* hungarian = nullptr;
    RadioButton* italian = nullptr;
    RadioButton* russian = nullptr;
    RadioButton* slovenian = nullptr;
    RadioButton* spanish = nullptr;
    RadioButton* ukrainian = nullptr;

    Body1LinkLabel* languageHowToAddLink = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* okButton = nullptr;
};

LanguageDialog::Implementation::Implementation(QWidget* _parent)
    : azerbaijani(new RadioButton(_parent)),
      english(new RadioButton(_parent)),
      german(new RadioButton(_parent)),
      hebrew(new RadioButton(_parent)),
      hungarian(new RadioButton(_parent)),
      italian(new RadioButton(_parent)),
      russian(new RadioButton(_parent)),
      slovenian(new RadioButton(_parent)),
      spanish(new RadioButton(_parent)),
      ukrainian(new RadioButton(_parent)),
      languageHowToAddLink(new Body1LinkLabel(_parent)),
      okButton(new Button(_parent))
{
    azerbaijani->setText("Azərbaycan");
    azerbaijani->setProperty(kLanguageKey, QLocale::Azerbaijani);
    english->setChecked(true);
    english->setText("English");
    english->setProperty(kLanguageKey, QLocale::English);
    german->setText("Deutsch");
    german->setProperty(kLanguageKey, QLocale::German);
    hebrew ->setText("עִבְרִית");
    hebrew->setProperty(kLanguageKey, QLocale::Hebrew);
    hungarian->setText("Magyar");
    hungarian->setProperty(kLanguageKey, QLocale::Hungarian);
    italian->setText("Italiano");
    italian->setProperty(kLanguageKey, QLocale::Italian);
    russian->setText("Русский");
    russian->setProperty(kLanguageKey, QLocale::Russian);
    slovenian->setText("Slovenski");
    slovenian->setProperty(kLanguageKey, QLocale::Slovenian);
    spanish->setText("Español");
    spanish->setProperty(kLanguageKey, QLocale::Spanish);
    ukrainian->setText("Українська");
    ukrainian->setProperty(kLanguageKey, QLocale::Ukrainian);

    languageHowToAddLink->setLink(QUrl("https://github.com/dimkanovikov/starc/wiki/How-to-add-the-translation-of-Story-Architect-to-your-native-language-or-improve-one-of-existing%3F"));

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(okButton);

    RadioButtonGroup* projectLocationGroup = new RadioButtonGroup(_parent);
    for (auto language : languages()) {
        projectLocationGroup->add(language);
    }
}

QVector<RadioButton*> LanguageDialog::Implementation::languages() const
{
    return { azerbaijani,
             english,
             german,
             hebrew,
             hungarian,
             italian,
             russian,
             slovenian,
             spanish,
             ukrainian };
}


// ****


LanguageDialog::LanguageDialog(QWidget* _parent)
    : AbstractDialog(_parent),
      d(new Implementation(this))
{
    int row = 0;
    contentsLayout()->addWidget(d->azerbaijani, row++, 0);
    contentsLayout()->addWidget(d->german, row++, 0);
    contentsLayout()->addWidget(d->english, row++, 0);
    contentsLayout()->addWidget(d->spanish, row++, 0);
    contentsLayout()->addWidget(d->italian, row++, 0);
    contentsLayout()->addWidget(d->hungarian, row++, 0);
    contentsLayout()->addWidget(d->russian, row++, 0);
    contentsLayout()->addWidget(d->slovenian, row++, 0);
    contentsLayout()->addWidget(d->ukrainian, row++, 0);
    //
    contentsLayout()->addWidget(d->hebrew, 0, 1);
    //
    contentsLayout()->setRowStretch(row++, 1);
    contentsLayout()->setColumnStretch(2, 1);
    contentsLayout()->addWidget(d->languageHowToAddLink, row++, 0, 1, 3);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0, 1, 3);

    for (auto radioButton : d->languages()) {
        connect(radioButton, &RadioButton::checkedChanged, this, [this, radioButton] (bool _checked) {
            if (_checked) {
                auto language = radioButton->property(kLanguageKey).toInt();
                emit languageChanged(static_cast<QLocale::Language>(language));
            }
        });
    }
    connect(d->okButton, &Button::clicked, this, &LanguageDialog::hideDialog);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

void LanguageDialog::setCurrentLanguage(QLocale::Language _language)
{
    for (auto radioButton : d->languages()) {
        if (radioButton->property(kLanguageKey).toInt() == _language) {
            radioButton->setChecked(true);
            break;
        }
    }
}

LanguageDialog::~LanguageDialog() = default;

QWidget* LanguageDialog::focusedWidgetAfterShow() const
{
    return d->english;
}

void LanguageDialog::updateTranslations()
{
    setTitle(tr("Change application language"));

    d->languageHowToAddLink->setText(tr("Did not find your preffered language? Read how you can add it yourself."));

    d->okButton->setText(tr("Close"));
}

void LanguageDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    for (auto languge : d->languages()) {
        languge->setBackgroundColor(Ui::DesignSystem::color().background());
        languge->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    d->languageHowToAddLink->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    d->languageHowToAddLink->setBackgroundColor(DesignSystem::color().background());
    d->languageHowToAddLink->setTextColor(DesignSystem::color().secondary());

    d->okButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->okButton->setTextColor(Ui::DesignSystem::color().secondary());

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px8()).toMargins());
}

} // namespace Ui
