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
    RadioButton* systemLanguageButton = nullptr;
    QVector<RadioButton*> languageButtons;
    Body1LinkLabel* languageHowToAddLink = nullptr;
    Button* goToThemeButton = nullptr;
    Button* skipOnboardingButton = nullptr;
    QHBoxLayout* languagePageButtonsLayout = nullptr;

    Widget* themePage = nullptr;
    H5Label* themeTitleLabel = nullptr;
    RadioButton* darkAndLightThemeButton = nullptr;
    Body2Label* darkAndLightThemeInfoLabel = nullptr;
    RadioButton* darkThemeButton = nullptr;
    Body2Label* darkThemeInfoLabel = nullptr;
    RadioButton* lightThemeButton = nullptr;
    Body2Label* lightThemeInfoLabel = nullptr;
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
    systemLanguageButton = initLanguageButton("Use system locale settings", QLocale::AnyLanguage);
    systemLanguageButton->setChecked(true);
    RadioButton* englishLanguage = initLanguageButton("English", QLocale::English);
    RadioButton* russianLanguage = initLanguageButton("Русский", QLocale::Russian);

    RadioButtonGroup* languagesGroup = new RadioButtonGroup(languagePage);
    languagesGroup->add(systemLanguageButton);
    languagesGroup->add(englishLanguage);
    languagesGroup->add(russianLanguage);

    languageHowToAddLink = new Body1LinkLabel(languagePage);
    languageHowToAddLink->setLink(QUrl("https://github.com/dimkanovikov/KITScenarist/wiki/How-to-add-the-translation-of-KIT-Scenarist-to-your-native-language-or-improve-one-of-existing%3F"));

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
    languagePageLayout->addWidget(languageTitleLabel, 0, 0, 1, 3);
    languagePageLayout->addWidget(systemLanguageButton, 1, 0, 1, 3);
    languagePageLayout->addWidget(englishLanguage, 2, 0, 1, 1);
    languagePageLayout->addWidget(russianLanguage, 3, 0, 1, 1);
    languagePageLayout->setRowStretch(4, 1);
    languagePageLayout->addWidget(languageHowToAddLink, 5, 0, 1, 3);
    languagePageLayout->addLayout(languagePageButtonsLayout, 6, 0, 1, 3);

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
    languagePageButtonsLayout->setContentsMargins({static_cast<int>(Ui::DesignSystem::layout().px24()), 0, 0,
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
    darkAndLightThemeButton = initThemeButton(ApplicationTheme::DarkAndLight);
    darkAndLightThemeButton->setChecked(true);
    darkThemeButton = initThemeButton(ApplicationTheme::Dark);
    lightThemeButton = initThemeButton(ApplicationTheme::Light);

    RadioButtonGroup* themesGroup = new RadioButtonGroup(languagePage);
    themesGroup->add(darkAndLightThemeButton);
    themesGroup->add(darkThemeButton);
    themesGroup->add(lightThemeButton);

    darkAndLightThemeInfoLabel = new Body2Label(themePage);
    darkThemeInfoLabel = new Body2Label(themePage);
    lightThemeInfoLabel = new Body2Label(themePage);

    scaleFactorTitleLabel = new H6Label(themePage);
    scaleFactorSlider = new Slider(themePage);
    scaleFactorSlider->setMaximumValue(4000);
    scaleFactorSlider->setValue(1000);
    QObject::connect(scaleFactorSlider, &Slider::valueChanged, q, [this] (int _value) {
        emit q->scaleFactorChanged(static_cast<qreal>(std::max(1, _value)) / 1000.0);
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
    themePageLayout->addWidget(darkAndLightThemeButton, 1, 0, 1, 3);
    themePageLayout->addWidget(darkAndLightThemeInfoLabel, 2, 0, 1, 3);
    themePageLayout->addWidget(darkThemeButton, 3, 0, 1, 3);
    themePageLayout->addWidget(darkThemeInfoLabel, 4, 0, 1, 3);
    themePageLayout->addWidget(lightThemeButton, 5, 0, 1, 3);
    themePageLayout->addWidget(lightThemeInfoLabel, 6, 0, 1, 3);
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
    themePageButtonsLayout->setContentsMargins({static_cast<int>(Ui::DesignSystem::layout().px24()), 0, 0,
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
    d->systemLanguageButton->setText(tr("Use system locale settings"));
    d->languageHowToAddLink->setText(tr("Did not find your preffered language? Read how you can add it yourself."));
    d->goToThemeButton->setText(tr("Continue"));
    d->skipOnboardingButton->setText(tr("Skip onboarding"));

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

OnboardingView::~OnboardingView() = default;

} // namespace Ui
