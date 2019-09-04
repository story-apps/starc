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
    explicit Implementation(OnboardingView* _parent);

    /**
     * @brief Настроить страницу выбора языка
     */
    void initLanguagePage();

    OnboardingView* q = nullptr;

    Widget* languagePage = nullptr;
    H5Label* languageTitle = nullptr;
    RadioButton* systemLanguage = nullptr;

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

    languageTitle = new H5Label(languagePage);
    languageTitle->setBackgroundColor(DesignSystem::color().surface());
    languageTitle->setTextColor(DesignSystem::color().onSurface());

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
    systemLanguage = initLanguage("System", QLocale::AnyLanguage);
    systemLanguage->setChecked(true);
    RadioButton* englishLanguage = initLanguage("English", QLocale::English);
    RadioButton* russianLanguage = initLanguage("Русский", QLocale::Russian);

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
    d->languageTitle->setText(tr("Choose preferred language"));
    d->systemLanguage->setText(tr("Use system locale settings"));
}

OnboardingView::~OnboardingView() = default;

} // namespace Ui
