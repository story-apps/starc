#include "onboarding_view.h"

#include <ui/design_system/design_system.h>
#include <ui/settings/widgets/theme_preview.h>
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


namespace Ui {

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
    ThemePreview* lightTheme = nullptr;
    ThemePreview* darkAndLightTheme = nullptr;
    ThemePreview* darkTheme = nullptr;
    QHBoxLayout* themesLayout = nullptr;
    H6Label* scaleFactorTitleLabel = nullptr;
    Slider* scaleFactorSlider = nullptr;
    Body2Label* scaleFactorSmallInfoLabel = nullptr;
    Body2Label* scaleFactorBigInfoLabel = nullptr;
    Button* finishOnboardingButton = nullptr;
    QHBoxLayout* themePageButtonsLayout = nullptr;
};

OnboardingView::Implementation::Implementation(OnboardingView* _parent)
    : q(_parent)
    , languagePage(new Widget(_parent))
    , themePage(new Widget(_parent))
{
    initLanguagePage();
    initThemePage();
}

void OnboardingView::Implementation::initLanguagePage()
{
    languageTitleLabel = new H5Label(languagePage);

    auto initLanguageButton = [this](const QString& _name, QLocale::Language _language) {
        RadioButton* languageButton = new RadioButton(languagePage);
        languageButton->setText(_name);
        languageButton->setChecked(QLocale().system().language() == _language);
        QObject::connect(languageButton, &RadioButton::checkedChanged, q,
                         [this, _language](bool _checked) {
                             if (_checked) {
                                 emit q->languageChanged(_language);
                             }
                         });
        languageButtons.append(languageButton);
        return languageButton;
    };
    RadioButton* azerbaijaniLanguage = initLanguageButton("Azərbaycan", QLocale::Azerbaijani);
    RadioButton* belarusianLanguage = initLanguageButton("Беларуский", QLocale::Belarusian);
    RadioButton* catalanLanguage = initLanguageButton("Català", QLocale::Catalan);
    RadioButton* croatianLanguage = initLanguageButton("Hrvatski", QLocale::Croatian);
    RadioButton* danishLanguage = initLanguageButton("Dansk", QLocale::Danish);
    RadioButton* englishLanguage = initLanguageButton("English", QLocale::English);
    RadioButton* esperantoLanguage = initLanguageButton("Esperanto", QLocale::Esperanto);
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
    RadioButton* portugueseLanguage = initLanguageButton(
        "Português", static_cast<QLocale::Language>(QLocale::LastLanguage + 1));
    RadioButton* portugueseBrazilLanguage
        = initLanguageButton("Português Brasileiro", QLocale::Portuguese);
    RadioButton* romanianLanguage = initLanguageButton("Română", QLocale::Romanian);
    RadioButton* russianLanguage = initLanguageButton("Русский", QLocale::Russian);
    RadioButton* slovenianLanguage = initLanguageButton("Slovenski", QLocale::Slovenian);
    RadioButton* spanishLanguage = initLanguageButton("Español", QLocale::Spanish);
    RadioButton* tagalogLanguage = initLanguageButton("Tagalog", QLocale::Filipino);
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

    //
    // Настроим цепочку переходов фокуса
    //
    auto buildFocusChain = [](const QVector<RadioButton*>& _buttons) {
        RadioButton* previousButton = nullptr;
        for (auto button : _buttons) {
            if (previousButton != nullptr) {
                setTabOrder(previousButton, button);
            }
            previousButton = button;
        }
    };
    buildFocusChain({
        azerbaijaniLanguage, belarusianLanguage,       catalanLanguage,
        danishLanguage,      germanLanguage,           englishLanguage,
        spanishLanguage,     esperantoLanguage,        frenchLanguage,
        galicianLanguage,    croatianLanguage,         indonesianLanguage,
        italianLanguage,     hungarianLanguage,        polishLanguage,
        portugueseLanguage,  portugueseBrazilLanguage, romanianLanguage,
        russianLanguage,     slovenianLanguage,        tagalogLanguage,
        turkishLanguage,     ukrainianLanguage,        hebrewLanguage,
        hindiLanguage,       persianLanguage,
    });

    languageHowToAddLink = new Body1LinkLabel(languagePage);
    languageHowToAddLink->setLink(QUrl("https://starc.app/translate"));

    goToThemeButton = new Button(languagePage);
    goToThemeButton->setContained(true);
    QObject::connect(goToThemeButton, &Button::clicked, q, &OnboardingView::showThemePageRequested);
    skipOnboardingButton = new Button(languagePage);
    QObject::connect(skipOnboardingButton, &Button::clicked, q,
                     &OnboardingView::skipOnboardingRequested);
    languagePageButtonsLayout = new QHBoxLayout;
    languagePageButtonsLayout->addWidget(goToThemeButton);
    languagePageButtonsLayout->addWidget(skipOnboardingButton);
    languagePageButtonsLayout->addStretch();


    QGridLayout* languagePageLayout = new QGridLayout(languagePage);
    languagePageLayout->setSpacing(0);
    languagePageLayout->setContentsMargins({});
    int row = 0;
    languagePageLayout->addWidget(languageTitleLabel, row++, 0, 1, 5);
    languagePageLayout->addWidget(azerbaijaniLanguage, row++, 0);
    languagePageLayout->addWidget(belarusianLanguage, row++, 0);
    languagePageLayout->addWidget(catalanLanguage, row++, 0);
    languagePageLayout->addWidget(danishLanguage, row++, 0);
    languagePageLayout->addWidget(germanLanguage, row++, 0);
    languagePageLayout->addWidget(englishLanguage, row++, 0);
    languagePageLayout->addWidget(spanishLanguage, row++, 0);
    languagePageLayout->addWidget(esperantoLanguage, row++, 0);
    int rowForSecondColumn = 1;
    languagePageLayout->addWidget(frenchLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(galicianLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(croatianLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(indonesianLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(italianLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(hungarianLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(polishLanguage, rowForSecondColumn++, 1);
    languagePageLayout->addWidget(portugueseLanguage, rowForSecondColumn++, 1);
    int rowForThirdColumn = 1;
    languagePageLayout->addWidget(portugueseBrazilLanguage, rowForThirdColumn++, 2);
    languagePageLayout->addWidget(romanianLanguage, rowForThirdColumn++, 2);
    languagePageLayout->addWidget(russianLanguage, rowForThirdColumn++, 2);
    languagePageLayout->addWidget(slovenianLanguage, rowForThirdColumn++, 2);
    languagePageLayout->addWidget(tagalogLanguage, rowForThirdColumn++, 2);
    languagePageLayout->addWidget(turkishLanguage, rowForThirdColumn++, 2);
    languagePageLayout->addWidget(ukrainianLanguage, rowForThirdColumn++, 2);
    int rowForFifthColumn = 1;
    languagePageLayout->addWidget(hebrewLanguage, rowForFifthColumn++, 3);
    languagePageLayout->addWidget(hindiLanguage, rowForFifthColumn++, 3);
    languagePageLayout->addWidget(persianLanguage, rowForFifthColumn++, 3);
    languagePageLayout->setRowStretch(row++, 1);
    languagePageLayout->setColumnStretch(4, 1);
    languagePageLayout->addWidget(languageHowToAddLink, row++, 0, 1, 5);
    languagePageLayout->addLayout(languagePageButtonsLayout, row++, 0, 1, 5);

    languagePage->hide();
}

void OnboardingView::Implementation::updateLanguagePageUi()
{
    languagePage->setBackgroundColor(DesignSystem::color().surface());
    languageTitleLabel->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    languageTitleLabel->setBackgroundColor(DesignSystem::color().surface());
    languageTitleLabel->setTextColor(DesignSystem::color().onSurface());
    for (auto languageButton : std::as_const(languageButtons)) {
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
    languagePageButtonsLayout->setSpacing(
        static_cast<int>(Ui::DesignSystem::layout().buttonsSpacing()));
    languagePageButtonsLayout->setContentsMargins(
        { static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
          static_cast<int>(Ui::DesignSystem::layout().px24()),
          static_cast<int>(Ui::DesignSystem::layout().px12()) });
}

void OnboardingView::Implementation::initThemePage()
{
    themeTitleLabel = new H5Label(themePage);

    auto initThemePreview = [this](ApplicationTheme _theme) {
        auto themePreview = new ThemePreview(themePage);
        themePreview->setTheme(_theme);
        QObject::connect(themePreview, &ThemePreview::themePressed, q,
                         &OnboardingView::themeChanged);
        return themePreview;
    };
    lightTheme = initThemePreview(ApplicationTheme::Light);
    darkAndLightTheme = initThemePreview(ApplicationTheme::DarkAndLight);
    darkTheme = initThemePreview(ApplicationTheme::Dark);

    scaleFactorTitleLabel = new H6Label(themePage);
    scaleFactorSlider = new Slider(themePage);
    scaleFactorSlider->setMaximumValue(3500);
    scaleFactorSlider->setValue(500);
    scaleFactorSlider->setDefaultPosition(500);
    QObject::connect(scaleFactorSlider, &Slider::valueChanged, q, [this](int _value) {
        emit q->scaleFactorChanged(0.5 + static_cast<qreal>(_value) / 1000.0);
    });
    scaleFactorSmallInfoLabel = new Body2Label(themePage);
    scaleFactorBigInfoLabel = new Body2Label(themePage);

    finishOnboardingButton = new Button(themePage);
    finishOnboardingButton->setContained(true);
    QObject::connect(finishOnboardingButton, &Button::clicked, q,
                     &OnboardingView::finishOnboardingRequested);
    themePageButtonsLayout = new QHBoxLayout;
    themePageButtonsLayout->addWidget(finishOnboardingButton);
    themePageButtonsLayout->addStretch();


    QGridLayout* themePageLayout = new QGridLayout(themePage);
    themePageLayout->setSpacing(0);
    themePageLayout->setContentsMargins({});
    int row = 0;
    themePageLayout->addWidget(themeTitleLabel, row++, 0, 1, 3);
    {
        themesLayout = new QHBoxLayout;
        themesLayout->setContentsMargins({});
        themesLayout->setSpacing(0);
        themesLayout->addWidget(lightTheme);
        themesLayout->addWidget(darkAndLightTheme);
        themesLayout->addWidget(darkTheme);
        themesLayout->addStretch();
        themePageLayout->addLayout(themesLayout, row++, 0, 1, 3);
    }
    themePageLayout->addWidget(scaleFactorTitleLabel, row++, 0, 1, 3);
    themePageLayout->addWidget(scaleFactorSlider, row++, 0, 1, 3);
    themePageLayout->addWidget(scaleFactorSmallInfoLabel, row, 0, 1, 1);
    themePageLayout->setColumnStretch(1, 1);
    themePageLayout->addWidget(scaleFactorBigInfoLabel, row++, 2, 1, 1);
    themePageLayout->setRowStretch(row++, 1);
    themePageLayout->addLayout(themePageButtonsLayout, row++, 0, 1, 3);

    themePage->hide();
}

void OnboardingView::Implementation::updateThemePageUi()
{
    themePage->setBackgroundColor(DesignSystem::color().surface());
    themeTitleLabel->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    themeTitleLabel->setBackgroundColor(DesignSystem::color().surface());
    themeTitleLabel->setTextColor(DesignSystem::color().onSurface());
    for (auto themePreview : { lightTheme, darkAndLightTheme, darkTheme }) {
        themePreview->setBackgroundColor(DesignSystem::color().surface());
        themePreview->setTextColor(DesignSystem::color().onSurface());
    }
    themesLayout->setContentsMargins(
        QMargins(Ui::DesignSystem::layout().px24(), 0, Ui::DesignSystem::layout().px24(), 0));
    themesLayout->setSpacing(Ui::DesignSystem::layout().px24());
    QMarginsF themeInfoLabelMargins = Ui::DesignSystem::label().margins();
    themeInfoLabelMargins.setLeft(Ui::DesignSystem::layout().px62());
    themeInfoLabelMargins.setTop(0);
    QColor themeInfoLabelTextColor = DesignSystem::color().onSurface();
    themeInfoLabelTextColor.setAlphaF(Ui::DesignSystem::disabledTextOpacity());
    scaleFactorTitleLabel->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    scaleFactorTitleLabel->setBackgroundColor(DesignSystem::color().surface());
    scaleFactorTitleLabel->setTextColor(DesignSystem::color().onSurface());
    scaleFactorSlider->setBackgroundColor(DesignSystem::color().surface());
    scaleFactorSlider->setContentsMargins({ static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                            static_cast<int>(Ui::DesignSystem::layout().px24()),
                                            0 });
    QMarginsF themeScaleFactorInfoLabelMargins = Ui::DesignSystem::label().margins();
    themeScaleFactorInfoLabelMargins.setTop(0);
    for (auto label : { scaleFactorSmallInfoLabel, scaleFactorBigInfoLabel }) {
        label->setContentsMargins(themeScaleFactorInfoLabelMargins.toMargins());
        label->setBackgroundColor(DesignSystem::color().surface());
        label->setTextColor(themeInfoLabelTextColor);
    }
    finishOnboardingButton->setBackgroundColor(DesignSystem::color().secondary());
    finishOnboardingButton->setTextColor(DesignSystem::color().onSecondary());
    themePageButtonsLayout->setSpacing(
        static_cast<int>(Ui::DesignSystem::layout().buttonsSpacing()));
    themePageButtonsLayout->setContentsMargins(
        { static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
          static_cast<int>(Ui::DesignSystem::layout().px24()),
          static_cast<int>(Ui::DesignSystem::layout().px12()) });
}


// ****


OnboardingView::OnboardingView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    showLanguagePage();

    updateTranslations();
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
    d->languageHowToAddLink->setText(
        tr("Did not find your preffered language? Read how you can add it yourself."));
    d->goToThemeButton->setText(tr("Continue"));
    d->skipOnboardingButton->setText(tr("Skip initial setup"));

    d->themeTitleLabel->setText(tr("Choose application theme"));
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
