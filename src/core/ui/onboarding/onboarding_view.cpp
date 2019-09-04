#include "onboarding_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>

#include <QGridLayout>


namespace Ui
{

class OnboardingView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Настроить страницу выбора языка
     */
    void initLanguagePage();

    Widget* languagePage = nullptr;
    Widget* themePage = nullptr;
    Widget* finalPage = nullptr;
};

OnboardingView::Implementation::Implementation(QWidget* _parent)
    : languagePage(new Widget(_parent)),
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

    H5Label* languageTitle = new H5Label(languagePage);
    languageTitle->setBackgroundColor(DesignSystem::color().surface());
    languageTitle->setTextColor(DesignSystem::color().onSurface());
    languageTitle->setText("Choose preferred language");

    RadioButton* systemLanguage = new RadioButton(languagePage);
    systemLanguage->setBackgroundColor(DesignSystem::color().surface());
    systemLanguage->setTextColor(DesignSystem::color().onSurface());
    systemLanguage->setText("Detect language from system");

    RadioButton* englishLanguage = new RadioButton(languagePage);
    englishLanguage->setBackgroundColor(DesignSystem::color().surface());
    englishLanguage->setTextColor(DesignSystem::color().onSurface());
    englishLanguage->setText("English");

    RadioButton* russianLanguage = new RadioButton(languagePage);
    russianLanguage->setBackgroundColor(DesignSystem::color().surface());
    russianLanguage->setTextColor(DesignSystem::color().onSurface());
    russianLanguage->setText("Русский");

    RadioButtonGroup* languagesGroup = new RadioButtonGroup(languagePage);
    languagesGroup->add(systemLanguage);
    languagesGroup->add(englishLanguage);
    languagesGroup->add(russianLanguage);

    QGridLayout* languagePageLayout = new QGridLayout(languagePage);
    languagePageLayout->addWidget(languageTitle, 0, 0, 1, 3);
    languagePageLayout->addWidget(systemLanguage, 1, 0, 1, 3);
    languagePageLayout->addWidget(englishLanguage, 2, 0, 1, 1);
    languagePageLayout->addWidget(russianLanguage, 3, 0, 1, 1);
    languagePageLayout->setRowStretch(4, 1);
    languagePage->hide();
}


// ****


OnboardingView::OnboardingView(QWidget* _parent)
    : StackWidget(_parent),
      d(new Implementation(this))
{
    setBackgroundColor(DesignSystem::color().surface());

    showLanguagePage();
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

}

OnboardingView::~OnboardingView() = default;

} // namespace Ui
