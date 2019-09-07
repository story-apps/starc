#include "onboarding_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>

#include <QGridLayout>
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
     * @brief Настроить страницу настройки темы
     */
    void initThemePage();

    OnboardingView* q = nullptr;

    Widget* languagePage = nullptr;
    H5Label* languageTitleLabel = nullptr;
    RadioButton* systemLanguageButton = nullptr;
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

    Widget* finalPage = nullptr;
};

OnboardingView::Implementation::Implementation(OnboardingView* _parent)
    : q(_parent),
      languagePage(new Widget(_parent)),
      themePage(new Widget(_parent)),
      finalPage(new Widget(_parent))
{
    initLanguagePage();
    initThemePage();

    finalPage->setBackgroundColor(DesignSystem::color().surface());
    finalPage->hide();
}

void OnboardingView::Implementation::initLanguagePage()
{
    languagePage->setBackgroundColor(DesignSystem::color().surface());

    languageTitleLabel = new H5Label(languagePage);
    languageTitleLabel->setBackgroundColor(DesignSystem::color().surface());
    languageTitleLabel->setTextColor(DesignSystem::color().onSurface());

    auto initLanguageButton = [this] (const QString& _name, QLocale::Language _language) {
        RadioButton* radioButton = new RadioButton(languagePage);
        radioButton->setBackgroundColor(DesignSystem::color().surface());
        radioButton->setTextColor(DesignSystem::color().onSurface());
        radioButton->setText(_name);
        QObject::connect(radioButton, &RadioButton::checkedChanged, q,
            [this, _language] (bool _checked)
        {
            if (_checked) {
                emit q->languageChanged(_language);
            }
        });
        return radioButton;
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
    languageHowToAddLink->setBackgroundColor(DesignSystem::color().surface());
    languageHowToAddLink->setTextColor(DesignSystem::color().secondary());
    languageHowToAddLink->setLink(QUrl("https://github.com/dimkanovikov/KITScenarist/wiki/How-to-add-the-translation-of-KIT-Scenarist-to-your-native-language-or-improve-one-of-existing%3F"));

    goToThemeButton = new Button(languagePage);
    goToThemeButton->setBackgroundColor(DesignSystem::color().secondary());
    goToThemeButton->setTextColor(DesignSystem::color().onSecondary());
    goToThemeButton->setContained(true);
    QObject::connect(goToThemeButton, &Button::clicked, q, &OnboardingView::showThemePageRequested);
    skipOnboardingButton = new Button(languagePage);
    skipOnboardingButton->setBackgroundColor(DesignSystem::color().secondary());
    skipOnboardingButton->setTextColor(DesignSystem::color().secondary());
    languagePageButtonsLayout = new QHBoxLayout;
    languagePageButtonsLayout->setSpacing(0);
    languagePageButtonsLayout->setContentsMargins({});
    languagePageButtonsLayout->addStretch();
    languagePageButtonsLayout->addWidget(goToThemeButton);
    languagePageButtonsLayout->addWidget(skipOnboardingButton);


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

void OnboardingView::Implementation::initThemePage()
{
    themePage->setBackgroundColor(DesignSystem::color().surface());

    themeTitleLabel = new H5Label(themePage);
    themeTitleLabel->setBackgroundColor(DesignSystem::color().surface());
    themeTitleLabel->setTextColor(DesignSystem::color().onSurface());

    auto initThemeButton = [this] (ApplicationTheme _theme) {
        RadioButton* radioButton = new RadioButton(themePage);
        radioButton->setBackgroundColor(DesignSystem::color().surface());
        radioButton->setTextColor(DesignSystem::color().onSurface());
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

    auto initThemeInfoLabel = [this] {
        Body2Label* label = new Body2Label(themePage);
        label->setBackgroundColor(DesignSystem::color().surface());
        QColor textColor = DesignSystem::color().onSurface();
        textColor.setAlphaF(Ui::DesignSystem::disabledTextOpacity());
        label->setTextColor(textColor);
        return label;
    };
    darkAndLightThemeInfoLabel = initThemeInfoLabel();
    darkThemeInfoLabel = initThemeInfoLabel();
    lightThemeInfoLabel = initThemeInfoLabel();


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
    themePageLayout->setRowStretch(7, 1);
//    themePageLayout->addWidget(languageHowToAddLink, 5, 0, 1, 3);
//    themePageLayout->addLayout(languagePageButtonsLayout, 6, 0, 1, 3);
    themePage->hide();
}


// ****


OnboardingView::OnboardingView(QWidget* _parent)
    : StackWidget(_parent),
      d(new Implementation(this))
{
    setBackgroundColor(DesignSystem::color().surface());

    showLanguagePage();

    updateTranslations();
    designSysemChangeEvent(nullptr);
}

void OnboardingView::showLanguagePage()
{
    setCurrentWidget(d->languagePage);
}

void OnboardingView::showThemePage()
{
    setCurrentWidget(d->themePage);
}

void OnboardingView::showFinalPage()
{
    setCurrentWidget(d->finalPage);
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
}

void OnboardingView::designSysemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    d->languagePageButtonsLayout->setSpacing(static_cast<int>(Ui::DesignSystem::layout().buttonsSpacing()));
    d->languagePageButtonsLayout->setContentsMargins({0, 0, static_cast<int>(Ui::DesignSystem::layout().px24()),
                                       static_cast<int>(Ui::DesignSystem::layout().px12())});

    d->languageTitleLabel->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    d->languageHowToAddLink->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());

    d->themeTitleLabel->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    QMarginsF themeInfoLabelMargins = Ui::DesignSystem::label().margins();
    themeInfoLabelMargins.setLeft(Ui::DesignSystem::layout().px62());
    themeInfoLabelMargins.setTop(Ui::DesignSystem::layout().px4());
    d->darkAndLightThemeInfoLabel->setContentsMargins(themeInfoLabelMargins.toMargins());
    d->darkThemeInfoLabel->setContentsMargins(themeInfoLabelMargins.toMargins());
    d->lightThemeInfoLabel->setContentsMargins(themeInfoLabelMargins.toMargins());
}

OnboardingView::~OnboardingView() = default;

} // namespace Ui
