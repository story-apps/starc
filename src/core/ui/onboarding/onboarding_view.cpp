#include "onboarding_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/slider/slider.h>

#include <QGridLayout>
#include <QSlider>
#include <QTimer>
#include <QUrl>


namespace Ui
{

class OnboardingView::Implementation
{
public:
    explicit Implementation(OnboardingView* _parent);

    /**
     * @brief Настроить страницу выбора языка
     */
    void initLanguagePage();

    /**
     * @brief Обновить настройки UI страницы выбора языка
     */
    void updateLanguagePageUi();

    /**
     * @brief Настроить страницу настройки темы
     */
    void initThemePage();

    /**
     * @brief Обновить настройки UI страницы настройки темы
     */
    void updateThemePageUi();

    OnboardingView* q = nullptr;

    Widget* languagePage = nullptr;
    H5Label* languageTitleLabel = nullptr;
    QVector<RadioButton*> languageButtons;
    Body1LinkLabel* languageHowToAddLink = nullptr;
    Button* goToThemeButton = nullptr;
    Button* skipOnboardingButton = nullptr;
    QHBoxLayout* languagePageButtonsLayout = nullptr;

    Widget* themePage = nullptr;
    H5Label* themeTitleLabel = nullptr;
    RadioButton* lightThemeButton = nullptr;
    Body2Label* lightThemeInfoLabel = nullptr;
    RadioButton* darkAndLightThemeButton = nullptr;
    Body2Label* darkAndLightThemeInfoLabel = nullptr;
    RadioButton* darkThemeButton = nullptr;
    Body2Label* darkThemeInfoLabel = nullptr;
    H6Label* scaleFactorTitleLabel = nullptr;
    Slider* scaleFactorSlider = nullptr;
    Body2Label* scaleFactorSmallInfoLabel = nullptr;
    Body2Label* scaleFactorBigInfoLabel = nullptr;
    Button* finishOnboardingButton = nullptr;
    QHBoxLayout* themePageButtonsLayout = nullptr;
};

OnboardingView::Implementation::Implementation(OnboardingView* _parent)
    : q(_parent),
      languagePage(new Widget(_parent)),
      themePage(new Widget(_parent))
{
    initLanguagePage();
    initThemePage();
}

void OnboardingView::Implementation::initLanguagePage()
{
    languageTitleLabel = new H5Label(languagePage);

    auto initLanguageButton = [this] (const QString& _name, QLocale::Language _language) {
        RadioButton* languageButton = new RadioButton(languagePage);
        languageButton->setText(_name);
        languageButton->setChecked(QLocale().system().language() == _language);
        QObject::connect(languageButton, &RadioButton::checkedChanged, q,
            [this, _language] (bool _checked)
        {
            if (_checked) {
                emit q->languageChanged(_language);
            }
        });
        languageButtons.append(languageButton);
        return languageButton;
    };
    RadioButton* azerbaijaniLanguage = initLanguageButton("Azərbaycan", QLocale::Azerbaijani);
    RadioButton* belarusianLanguage = initLanguageButton("Беларуский", QLocale::Belarusian);
    RadioButton* englishLanguage = initLanguageButton("English", QLocale::English);
    RadioButton* frenchLanguage = initLanguageButton("Français", QLocale::French);
    RadioButton* galicianLanguage = initLanguageButton("Galego", QLocale::Galician);
    RadioButton* germanLanguage = initLanguageButton("Deutsch", QLocale::German);
    RadioButton* hebrewLanguage = initLanguageButton("עִבְרִית", QLocale::Hebrew);
    RadioButton* hindiLanguage = initLanguageButton("हिन्दी", QLocale::Hindi);
    RadioButton* hungarianLanguage = initLanguageButton("Magyar", QLocale::Hungarian);
    RadioButton* indonesianLanguage = initLanguageButton("Indonesian", QLocale::Indonesian);
    RadioButton* italianLanguage = initLanguageButton("Italiano", QLocale::Italian);
    RadioButton* persianLanguage = initLanguageButton("فارسی", QLocale::Persian);
    RadioButton* polishLanguage = initLanguageButton("Polski", QLocale::Polish);
    RadioButton* portugueseBrazilLanguage = initLanguageButton("Português Brasileiro", QLocale::Portuguese);
    RadioButton* romanianLanguage = initLanguageButton("Română", QLocale::Romanian);
    RadioButton* russianLanguage = initLanguageButton("Русский", QLocale::Russian);
    RadioButton* slovenianLanguage = initLanguageButton("Slovenski", QLocale::Slovenian);
    RadioButton* spanishLanguage = initLanguageButton("Español", QLocale::Spanish);
    RadioButton* turkishLanguage = initLanguageButton("Türkçe", QLocale::Turkish);
    RadioButton* ukrainianLanguage = initLanguageButton("Українська", QLocale::Ukrainian);
    //
    // Если мы умеем в язык системы, то оставляем выбранным его
    //
    bool isSystemLanguageSupported = false;
    for (auto language : std::as_const(languageButtons)) {
        if (language->isChecked()) {
            isSystemLanguageSupported = true;
            break;
        }
    }
    //
    // ... а если не умеем, то выбираем английский
    //
    if (!isSystemLanguageSupported) {
        englishLanguage->setChecked(true);
    }

    RadioButtonGroup* languagesGroup = new RadioButtonGroup(languagePage);
    for (auto languageButton : std::as_const(languageButtons)) {
        languagesGroup->add(languageButton);
    }

    languageHowToAddLink = new Body1LinkLabel(languagePage);
    languageHowToAddLink->setLink(QUrl("https://github.com/dimkanovikov/starc/wiki/How-to-add-the-translation-of-Story-Architect-to-your-native-language-or-improve-one-of-existing%3F"));

    goToThemeButton = new Button(languagePage);
    goToThemeButton->setContained(true);
    QObject::connect(goToThemeButton, &Button::clicked, q, &OnboardingView::showThemePageRequested);
    skipOnboardingButton = new Button(languagePage);
    QObject::connect(skipOnboardingButton, &Button::clicked, q, &OnboardingView::skipOnboardingRequested);
    languagePageButtonsLayout = new QHBoxLayout;
    languagePageButtonsLayout->addWidget(goToThemeButton);
    languagePageButtonsLayout->addWidget(skipOnboardingButton);
    languagePageButtonsLayout->addStretch();


    QGridLayout* languagePageLayout = new QGridLayout(languagePage);
    languagePageLayout->setSpacing(0);
    languagePageLayout->setContentsMargins({});
    int row = 0;
    languagePageLayout->addWidget(languageTitleLabel, row++, 0, 1, 4);
    languagePageLayout->addWidget(azerbaijaniLanguage, row++, 0);
    languagePageLayout->addWidget(belarusianLanguage, row++, 0);
    languagePageLayout->addWidget(germanLanguage, row++, 0);
    languagePageLayout->addWidget(englishLanguage, row++, 0);
    languagePageLayout->addWidget(spanishLanguage, row++, 0);
    languagePageLayout->addWidget(frenchLanguage, row++, 0);
    languagePageLayout->addWidget(galicianLanguage, row++, 0);
    languagePageLayout->addWidget(indonesianLanguage, row++, 0);
    languagePageLayout->addWidget(italianLanguage, row++, 0);
    int rowForSecondColumn = 1;
    languagePageLayout->addWidget(hungarianLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(polishLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(portugueseBrazilLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(romanianLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(russianLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(slovenianLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(turkishLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(ukrainianLanguage, rowForSecondColumn++, 1);
    int rowForThirdColumn = 1;
    languagePageLayout->addWidget(hebrewLanguage, rowForThirdColumn++, 2);
    languagePageLayout->addWidget(hindiLanguage, rowForThirdColumn++, 2);
    languagePageLayout->addWidget(persianLanguage, rowForThirdColumn++, 2);
    languagePageLayout->setRowStretch(row++, 1);
    languagePageLayout->setColumnStretch(3, 1);
    languagePageLayout->addWidget(languageHowToAddLink, row++, 0, 1, 4);
    languagePageLayout->addLayout(languagePageButtonsLayout, row++, 0, 1, 4);

    languagePage->hide();
}

void OnboardingView::Implementation::updateLanguagePageUi()
{
    languagePage->setBackgroundColor(DesignSystem::color().surface());
    languageTitleLabel->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    languageTitleLabel->setBackgroundColor(DesignSystem::color().surface());
    languageTitleLabel->setTextColor(DesignSystem::color().onSurface());
    for (auto languageButton : languageButtons) {
        languageButton->setBackgroundColor(DesignSystem::color().surface());
        languageButton->setTextColor(DesignSystem::color().onSurface());
    }
    languageHowToAddLink->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    languageHowToAddLink->setBackgroundColor(DesignSystem::color().surface());
    languageHowToAddLink->setTextColor(DesignSystem::color().secondary());
    goToThemeButton->setBackgroundColor(DesignSystem::color().secondary());
    goToThemeButton->setTextColor(DesignSystem::color().onSecondary());
    skipOnboardingButton->setBackgroundColor(DesignSystem::color().secondary());
    skipOnboardingButton->setTextColor(DesignSystem::color().secondary());
    languagePageButtonsLayout->setSpacing(static_cast<int>(Ui::DesignSystem::layout().buttonsSpacing()));
    languagePageButtonsLayout->setContentsMargins({static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                                   static_cast<int>(Ui::DesignSystem::layout().px24()),
                                                   static_cast<int>(Ui::DesignSystem::layout().px12())});
}

void OnboardingView::Implementation::initThemePage()
{
    themeTitleLabel = new H5Label(themePage);

    auto initThemeButton = [this] (ApplicationTheme _theme) {
        RadioButton* radioButton = new RadioButton(themePage);
        QObject::connect(radioButton, &RadioButton::checkedChanged, q,
            [this, _theme] (bool _checked)
        {
            if (_checked) {
                emit q->themeChanged(_theme);
            }
        });
        return radioButton;
    };
    lightThemeButton = initThemeButton(ApplicationTheme::Light);
    lightThemeButton->setChecked(true);
    darkAndLightThemeButton = initThemeButton(ApplicationTheme::DarkAndLight);
    darkThemeButton = initThemeButton(ApplicationTheme::Dark);

    RadioButtonGroup* themesGroup = new RadioButtonGroup(languagePage);
    themesGroup->add(lightThemeButton);
    themesGroup->add(darkAndLightThemeButton);
    themesGroup->add(darkThemeButton);

    lightThemeInfoLabel = new Body2Label(themePage);
    darkAndLightThemeInfoLabel = new Body2Label(themePage);
    darkThemeInfoLabel = new Body2Label(themePage);

    scaleFactorTitleLabel = new H6Label(themePage);
    scaleFactorSlider = new Slider(themePage);
    scaleFactorSlider->setMaximumValue(3500);
    scaleFactorSlider->setValue(500);
    QObject::connect(scaleFactorSlider, &Slider::valueChanged, q, [this] (int _value) {
        emit q->scaleFactorChanged(0.5 + static_cast<qreal>(_value) / 1000.0);
    });
    scaleFactorSmallInfoLabel = new Body2Label(themePage);
    scaleFactorBigInfoLabel = new Body2Label(themePage);

    finishOnboardingButton = new Button(themePage);
    finishOnboardingButton->setContained(true);
    QObject::connect(finishOnboardingButton, &Button::clicked, q, &OnboardingView::finishOnboardingRequested);
    themePageButtonsLayout = new QHBoxLayout;
    themePageButtonsLayout->addWidget(finishOnboardingButton);
    themePageButtonsLayout->addStretch();


    QGridLayout* themePageLayout = new QGridLayout(themePage);
    themePageLayout->setSpacing(0);
    themePageLayout->setContentsMargins({});
    themePageLayout->addWidget(themeTitleLabel, 0, 0, 1, 3);
    themePageLayout->addWidget(lightThemeButton, 1, 0, 1, 3);
    themePageLayout->addWidget(lightThemeInfoLabel, 2, 0, 1, 3);
    themePageLayout->addWidget(darkAndLightThemeButton, 3, 0, 1, 3);
    themePageLayout->addWidget(darkAndLightThemeInfoLabel, 4, 0, 1, 3);
    themePageLayout->addWidget(darkThemeButton, 5, 0, 1, 3);
    themePageLayout->addWidget(darkThemeInfoLabel, 6, 0, 1, 3);
    themePageLayout->addWidget(scaleFactorTitleLabel, 7, 0, 1, 3);
    themePageLayout->addWidget(scaleFactorSlider, 8, 0, 1, 3);
    themePageLayout->addWidget(scaleFactorSmallInfoLabel, 9, 0, 1, 1);
    themePageLayout->setColumnStretch(1, 1);
    themePageLayout->addWidget(scaleFactorBigInfoLabel, 9, 2, 1, 1);
    themePageLayout->setRowStretch(10, 1);
    themePageLayout->addLayout(themePageButtonsLayout, 11, 0, 1, 3);

    themePage->hide();
}

void OnboardingView::Implementation::updateThemePageUi()
{
    themePage->setBackgroundColor(DesignSystem::color().surface());
    themeTitleLabel->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    themeTitleLabel->setBackgroundColor(DesignSystem::color().surface());
    themeTitleLabel->setTextColor(DesignSystem::color().onSurface());
    for (auto themeButton : {darkAndLightThemeButton, darkThemeButton, lightThemeButton}) {
        themeButton->setBackgroundColor(DesignSystem::color().surface());
        themeButton->setTextColor(DesignSystem::color().onSurface());
    }
    QMarginsF themeInfoLabelMargins = Ui::DesignSystem::label().margins();
    themeInfoLabelMargins.setLeft(Ui::DesignSystem::layout().px62());
    themeInfoLabelMargins.setTop(0);
    QColor themeInfoLabelTextColor = DesignSystem::color().onSurface();
    themeInfoLabelTextColor.setAlphaF(Ui::DesignSystem::disabledTextOpacity());
    for (auto label : {darkAndLightThemeInfoLabel, darkThemeInfoLabel, lightThemeInfoLabel}) {
        label->setContentsMargins(themeInfoLabelMargins.toMargins());
        label->setBackgroundColor(DesignSystem::color().surface());
        label->setTextColor(themeInfoLabelTextColor);
    }
    scaleFactorTitleLabel->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    scaleFactorTitleLabel->setBackgroundColor(DesignSystem::color().surface());
    scaleFactorTitleLabel->setTextColor(DesignSystem::color().onSurface());
    scaleFactorSlider->setBackgroundColor(DesignSystem::color().surface());
    scaleFactorSlider->setContentsMargins({static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                           static_cast<int>(Ui::DesignSystem::layout().px24()), 0});
    QMarginsF themeScaleFactorInfoLabelMargins = Ui::DesignSystem::label().margins();
    themeScaleFactorInfoLabelMargins.setTop(0);
    for (auto label : {scaleFactorSmallInfoLabel, scaleFactorBigInfoLabel}) {
        label->setContentsMargins(themeScaleFactorInfoLabelMargins.toMargins());
        label->setBackgroundColor(DesignSystem::color().surface());
        label->setTextColor(themeInfoLabelTextColor);
    }
    finishOnboardingButton->setBackgroundColor(DesignSystem::color().secondary());
    finishOnboardingButton->setTextColor(DesignSystem::color().onSecondary());
    themePageButtonsLayout->setSpacing(static_cast<int>(Ui::DesignSystem::layout().buttonsSpacing()));
    themePageButtonsLayout->setContentsMargins({static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                                static_cast<int>(Ui::DesignSystem::layout().px24()),
                                                static_cast<int>(Ui::DesignSystem::layout().px12())});
}


// ****


OnboardingView::OnboardingView(QWidget* _parent)
    : StackWidget(_parent),
      d(new Implementation(this))
{
    showLanguagePage();

    designSystemChangeEvent(nullptr);
}

OnboardingView::~OnboardingView() = default;

void OnboardingView::showLanguagePage()
{
    setCurrentWidget(d->languagePage);
}

void OnboardingView::showThemePage()
{
    setCurrentWidget(d->themePage);
}

void OnboardingView::updateTranslations()
{
    d->languageTitleLabel->setText(tr("Choose preferred language"));
    d->languageHowToAddLink->setText(tr("Did not find your preffered language? Read how you can add it yourself."));
    d->goToThemeButton->setText(tr("Continue"));
    d->skipOnboardingButton->setText(tr("Skip initial setup"));

    d->themeTitleLabel->setText(tr("Choose application theme"));
    d->darkAndLightThemeButton->setText(tr("Dark & light theme"));
    d->darkAndLightThemeInfoLabel->setText(tr("Modern theme which combines dark and light colors for better concentration on the documents you work."));
    d->darkThemeButton->setText(tr("Dark theme"));
    d->darkThemeInfoLabel->setText(tr("Theme is more suitable for work in dimly lit rooms, and also in the evening or night."));
    d->lightThemeButton->setText(tr("Light theme"));
    d->lightThemeInfoLabel->setText(tr("Theme is convenient for work with sufficient light."));
    d->scaleFactorTitleLabel->setText(tr("Setup size of the user interface elements"));
    d->scaleFactorSmallInfoLabel->setText(tr("small"));
    d->scaleFactorBigInfoLabel->setText(tr("big"));
    d->finishOnboardingButton->setText(tr("Start writing"));
}

void OnboardingView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    setBackgroundColor(DesignSystem::color().surface());

    d->updateLanguagePageUi();
    d->updateThemePageUi();
}

} // namespace Ui
