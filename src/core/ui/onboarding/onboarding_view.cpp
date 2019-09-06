#include "onboarding_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>

#include <QGridLayout>
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

    OnboardingView* q = nullptr;

    Widget* languagePage = nullptr;
    H5Label* languageTitleButton = nullptr;
    RadioButton* systemLanguageButton = nullptr;
    Body1LinkLabel* languageHowToAddLink = nullptr;
    Button* goToThemeButton = nullptr;
    Button* skipOnboardingButton = nullptr;

    Widget* themePage = nullptr;
    Widget* finalPage = nullptr;
};

OnboardingView::Implementation::Implementation(OnboardingView* _parent)
    : q(_parent),
      languagePage(new Widget(_parent)),
      themePage(new Widget(_parent)),
      finalPage(new Widget(_parent))
{
    initLanguagePage();

    themePage->setBackgroundColor(DesignSystem::color().surface());
    themePage->hide();

    finalPage->setBackgroundColor(DesignSystem::color().surface());
    finalPage->hide();
}

void OnboardingView::Implementation::initLanguagePage()
{
    languagePage->setBackgroundColor(DesignSystem::color().surface());

    languageTitleButton = new H5Label(languagePage);
    languageTitleButton->setBackgroundColor(DesignSystem::color().surface());
    languageTitleButton->setTextColor(DesignSystem::color().onSurface());

    auto initLanguage = [this] (const QString& _name, QLocale::Language _language) {
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
    systemLanguageButton = initLanguage("Use system locale settings", QLocale::AnyLanguage);
    systemLanguageButton->setChecked(true);
    RadioButton* englishLanguage = initLanguage("English", QLocale::English);
    RadioButton* russianLanguage = initLanguage("Русский", QLocale::Russian);

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
    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->setSpacing(static_cast<int>(Ui::DesignSystem::layout().buttonsSpacing()));
    buttonsLayout->setContentsMargins({0, 0, static_cast<int>(Ui::DesignSystem::layout().px24()),
                                       static_cast<int>(Ui::DesignSystem::layout().px12())});
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(goToThemeButton);
    buttonsLayout->addWidget(skipOnboardingButton);


    QGridLayout* languagePageLayout = new QGridLayout(languagePage);
    languagePageLayout->setSpacing(0);
    languagePageLayout->setContentsMargins({});
    languagePageLayout->addWidget(languageTitleButton, 0, 0, 1, 3);
    languagePageLayout->addWidget(systemLanguageButton, 1, 0, 1, 3);
    languagePageLayout->addWidget(englishLanguage, 2, 0, 1, 1);
    languagePageLayout->addWidget(russianLanguage, 3, 0, 1, 1);
    languagePageLayout->setRowStretch(4, 1);
    languagePageLayout->addWidget(languageHowToAddLink, 5, 0, 1, 3);
    languagePageLayout->addLayout(buttonsLayout, 6, 0, 1, 3);
    languagePage->hide();
}


// ****


OnboardingView::OnboardingView(QWidget* _parent)
    : StackWidget(_parent),
      d(new Implementation(this))
{
    setBackgroundColor(DesignSystem::color().surface());

    showLanguagePage();

    updateTranslations();
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
    d->languageTitleButton->setText(tr("Choose preferred language"));
    d->systemLanguageButton->setText(tr("Use system locale settings"));
    d->languageHowToAddLink->setText(tr("Did not find your preffered language? Read how you can add it yourself."));
    d->goToThemeButton->setText(tr("Continue"));
    d->skipOnboardingButton->setText(tr("Skip onboarding"));
}

OnboardingView::~OnboardingView() = default;

} // namespace Ui
