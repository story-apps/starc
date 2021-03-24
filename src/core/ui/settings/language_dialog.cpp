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
    RadioButton* belarusian = nullptr;
    RadioButton* english = nullptr;
    RadioButton* french = nullptr;
    RadioButton* galician = nullptr;
    RadioButton* german = nullptr;
    RadioButton* hebrew = nullptr;
    RadioButton* hindi = nullptr;
    RadioButton* hungarian = nullptr;
    RadioButton* indonesian = nullptr;
    RadioButton* italian = nullptr;
    RadioButton* persian = nullptr;
    RadioButton* polish = nullptr;
    RadioButton* portugueseBrazil = nullptr;
    RadioButton* romanian = nullptr;
    RadioButton* russian = nullptr;
    RadioButton* slovenian = nullptr;
    RadioButton* spanish = nullptr;
    RadioButton* turkish = nullptr;
    RadioButton* ukrainian = nullptr;

    Body1LinkLabel* languageHowToAddLink = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* okButton = nullptr;
};

LanguageDialog::Implementation::Implementation(QWidget* _parent)
    : azerbaijani(new RadioButton(_parent)),
      belarusian(new RadioButton(_parent)),
      english(new RadioButton(_parent)),
      french(new RadioButton(_parent)),
      galician(new RadioButton(_parent)),
      german(new RadioButton(_parent)),
      hebrew(new RadioButton(_parent)),
      hindi(new RadioButton(_parent)),
      hungarian(new RadioButton(_parent)),
      indonesian(new RadioButton(_parent)),
      italian(new RadioButton(_parent)),
      persian(new RadioButton(_parent)),
      polish(new RadioButton(_parent)),
      portugueseBrazil(new RadioButton(_parent)),
      romanian(new RadioButton(_parent)),
      russian(new RadioButton(_parent)),
      slovenian(new RadioButton(_parent)),
      spanish(new RadioButton(_parent)),
      turkish(new RadioButton(_parent)),
      ukrainian(new RadioButton(_parent)),
      languageHowToAddLink(new Body1LinkLabel(_parent)),
      okButton(new Button(_parent))
{
    azerbaijani->setText("Azərbaycan");
    azerbaijani->setProperty(kLanguageKey, QLocale::Azerbaijani);
    belarusian->setText("Беларуский");
    belarusian->setProperty(kLanguageKey, QLocale::Belarusian);
    english->setChecked(true);
    english->setText("English");
    english->setProperty(kLanguageKey, QLocale::English);
    french->setText("Français");
    french->setProperty(kLanguageKey, QLocale::French);
    galician->setText("Galego");
    galician->setProperty(kLanguageKey, QLocale::Galician);
    german->setText("Deutsch");
    german->setProperty(kLanguageKey, QLocale::German);
    hebrew ->setText("עִבְרִית");
    hebrew->setProperty(kLanguageKey, QLocale::Hebrew);
    hindi ->setText("हिन्दी");
    hindi->setProperty(kLanguageKey, QLocale::Hindi);
    hungarian->setText("Magyar");
    hungarian->setProperty(kLanguageKey, QLocale::Hungarian);
    indonesian->setText("Indonesian");
    indonesian->setProperty(kLanguageKey, QLocale::Indonesian);
    italian->setText("Italiano");
    italian->setProperty(kLanguageKey, QLocale::Italian);
    persian->setText("فارسی");
    persian->setProperty(kLanguageKey, QLocale::Persian);
    polish->setText("Polski");
    polish->setProperty(kLanguageKey, QLocale::Polish);
    portugueseBrazil->setText("Português Brasileiro");
    portugueseBrazil->setProperty(kLanguageKey, QLocale::Portuguese);
    romanian->setText("Română");
    romanian->setProperty(kLanguageKey, QLocale::Romanian);
    russian->setText("Русский");
    russian->setProperty(kLanguageKey, QLocale::Russian);
    slovenian->setText("Slovenski");
    slovenian->setProperty(kLanguageKey, QLocale::Slovenian);
    spanish->setText("Español");
    spanish->setProperty(kLanguageKey, QLocale::Spanish);
    turkish->setText("Türkçe");
    turkish->setProperty(kLanguageKey, QLocale::Turkish);
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
                belarusian,
                english,
                french,
                galician,
                german,
                hebrew,
                hindi,
                hungarian,
                indonesian,
                italian,
                persian,
                polish,
                portugueseBrazil,
                romanian,
                russian,
                slovenian,
                spanish,
                turkish,
                ukrainian };
}


// ****


LanguageDialog::LanguageDialog(QWidget* _parent)
    : AbstractDialog(_parent),
      d(new Implementation(this))
{
    int row = 0;
    contentsLayout()->addWidget(d->azerbaijani, row++, 0);
    contentsLayout()->addWidget(d->belarusian, row++, 0);
    contentsLayout()->addWidget(d->german, row++, 0);
    contentsLayout()->addWidget(d->english, row++, 0);
    contentsLayout()->addWidget(d->spanish, row++, 0);
    contentsLayout()->addWidget(d->french, row++, 0);
    contentsLayout()->addWidget(d->galician, row++, 0);
    contentsLayout()->addWidget(d->indonesian, row++, 0);
    contentsLayout()->addWidget(d->italian, row++, 0);
    //
    int rowForSecondColumn = 0;
    contentsLayout()->addWidget(d->hungarian, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->polish, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->portugueseBrazil, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->romanian, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->russian, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->slovenian, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->turkish, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->ukrainian, rowForSecondColumn++, 1);
    //
    int rowForThirdColumn = 0;
    contentsLayout()->addWidget(d->hebrew, rowForThirdColumn++, 2);
    contentsLayout()->addWidget(d->hindi, rowForThirdColumn++, 2);
    contentsLayout()->addWidget(d->persian, rowForThirdColumn++, 2);
    //
    contentsLayout()->setRowStretch(row++, 1);
    contentsLayout()->setColumnStretch(3, 1);
    contentsLayout()->addWidget(d->languageHowToAddLink, row++, 0, 1, 4);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0, 1, 4);

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

QWidget* LanguageDialog::lastFocusableWidget() const
{
    return d->okButton;
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
