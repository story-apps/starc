#include "language_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/radio_button/percent_radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <utils/helpers/ui_helper.h>

#include <QCoreApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QStandardPaths>
#include <QUrl>


namespace Ui {

namespace {
const char* kLanguageKey = "language";
} // namespace

class LanguageDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить список всех кнопок языков
     */
    std::vector<PercentRadioButton*> languages() const;


    PercentRadioButton* arabic = nullptr;
    PercentRadioButton* azerbaijani = nullptr;
    PercentRadioButton* belarusian = nullptr;
    PercentRadioButton* catalan = nullptr;
    PercentRadioButton* chinese = nullptr;
    PercentRadioButton* croatian = nullptr;
    PercentRadioButton* danish = nullptr;
    PercentRadioButton* dutch = nullptr;
    PercentRadioButton* english = nullptr;
    PercentRadioButton* french = nullptr;
    PercentRadioButton* galician = nullptr;
    PercentRadioButton* german = nullptr;
    PercentRadioButton* greek = nullptr;
    PercentRadioButton* hebrew = nullptr;
    PercentRadioButton* hindi = nullptr;
    PercentRadioButton* hungarian = nullptr;
    PercentRadioButton* indonesian = nullptr;
    PercentRadioButton* italian = nullptr;
    PercentRadioButton* korean = nullptr;
    PercentRadioButton* persian = nullptr;
    PercentRadioButton* polish = nullptr;
    PercentRadioButton* portuguese = nullptr;
    PercentRadioButton* portugueseBrazil = nullptr;
    PercentRadioButton* romanian = nullptr;
    PercentRadioButton* russian = nullptr;
    PercentRadioButton* slovenian = nullptr;
    PercentRadioButton* spanish = nullptr;
    PercentRadioButton* swedish = nullptr;
    PercentRadioButton* tamil = nullptr;
    PercentRadioButton* telugu = nullptr;
    PercentRadioButton* turkish = nullptr;
    PercentRadioButton* ukrainian = nullptr;

    Body1LinkLabel* languageHowToAddLink = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Body2Label* translationProgressLabel = nullptr;
    Button* improveButton = nullptr;
    Button* browseButton = nullptr;
    Button* closeButton = nullptr;
};

LanguageDialog::Implementation::Implementation(QWidget* _parent)
    : arabic(new PercentRadioButton(_parent, 75))
    , azerbaijani(new PercentRadioButton(_parent, 74))
    , belarusian(new PercentRadioButton(_parent, 40))
    , catalan(new PercentRadioButton(_parent, 68))
    , chinese(new PercentRadioButton(_parent, 80))
    , croatian(new PercentRadioButton(_parent, 43))
    , danish(new PercentRadioButton(_parent, 71))
    , dutch(new PercentRadioButton(_parent, 90))
    , english(new PercentRadioButton(_parent, 100))
    , french(new PercentRadioButton(_parent, 69))
    , galician(new PercentRadioButton(_parent, 47))
    , german(new PercentRadioButton(_parent, 89))
    , greek(new PercentRadioButton(_parent, 83))
    , hebrew(new PercentRadioButton(_parent, 72))
    , hindi(new PercentRadioButton(_parent, 24))
    , hungarian(new PercentRadioButton(_parent, 27))
    , indonesian(new PercentRadioButton(_parent, 100))
    , italian(new PercentRadioButton(_parent, 51))
    , korean(new PercentRadioButton(_parent, 47))
    , persian(new PercentRadioButton(_parent, 50))
    , polish(new PercentRadioButton(_parent, 92))
    , portuguese(new PercentRadioButton(_parent, 9))
    , portugueseBrazil(new PercentRadioButton(_parent, 94))
    , romanian(new PercentRadioButton(_parent, 39))
    , russian(new PercentRadioButton(_parent, 100))
    , slovenian(new PercentRadioButton(_parent, 100))
    , spanish(new PercentRadioButton(_parent, 92))
    , swedish(new PercentRadioButton(_parent, 28))
    , tamil(new PercentRadioButton(_parent, 30))
    , telugu(new PercentRadioButton(_parent, 95))
    , turkish(new PercentRadioButton(_parent, 84))
    , ukrainian(new PercentRadioButton(_parent, 91))
    , languageHowToAddLink(new Body1LinkLabel(_parent))
    , translationProgressLabel(new Body2Label(_parent))
    , improveButton(new Button(_parent))
    , browseButton(new Button(_parent))
    , closeButton(new Button(_parent))
{
    arabic->setText("اَلْعَرَبِيَّةُ");
    arabic->setProperty(kLanguageKey, QLocale::Arabic);
    azerbaijani->setText("Azərbaycan");
    azerbaijani->setProperty(kLanguageKey, QLocale::Azerbaijani);
    belarusian->setText("Беларуский");
    belarusian->setProperty(kLanguageKey, QLocale::Belarusian);
    catalan->setText("Català");
    catalan->setProperty(kLanguageKey, QLocale::Catalan);
    chinese->setText("汉语");
    chinese->setProperty(kLanguageKey, QLocale::Chinese);
    croatian->setText("Hrvatski");
    croatian->setProperty(kLanguageKey, QLocale::Croatian);
    danish->setText("Dansk");
    danish->setProperty(kLanguageKey, QLocale::Danish);
    dutch->setText("Nederlands");
    dutch->setProperty(kLanguageKey, QLocale::Dutch);
    english->setChecked(true);
    english->setText("English");
    english->setProperty(kLanguageKey, QLocale::English);
    french->setText("Français");
    french->setProperty(kLanguageKey, QLocale::French);
    galician->setText("Galego");
    galician->setProperty(kLanguageKey, QLocale::Galician);
    german->setText("Deutsch");
    german->setProperty(kLanguageKey, QLocale::German);
    greek->setText("ελληνικά");
    greek->setProperty(kLanguageKey, QLocale::Greek);
    hebrew->setText("עִבְרִית");
    hebrew->setProperty(kLanguageKey, QLocale::Hebrew);
    hindi->setText("हिन्दी");
    hindi->setProperty(kLanguageKey, QLocale::Hindi);
    hungarian->setText("Magyar");
    hungarian->setProperty(kLanguageKey, QLocale::Hungarian);
    indonesian->setText("Indonesian");
    indonesian->setProperty(kLanguageKey, QLocale::Indonesian);
    italian->setText("Italiano");
    italian->setProperty(kLanguageKey, QLocale::Italian);
    korean->setText("한국어");
    korean->setProperty(kLanguageKey, QLocale::Korean);
    persian->setText("فارسی");
    persian->setProperty(kLanguageKey, QLocale::Persian);
    polish->setText("Polski");
    polish->setProperty(kLanguageKey, QLocale::Polish);
    portuguese->setText("Português");
    portuguese->setProperty(kLanguageKey, QLocale::LastLanguage + 1);
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
    swedish->setText("Svenska");
    swedish->setProperty(kLanguageKey, QLocale::Swedish);
    tamil->setText("தமிழ்");
    tamil->setProperty(kLanguageKey, QLocale::Tamil);
    telugu->setText("తెలుగు");
    telugu->setProperty(kLanguageKey, QLocale::Telugu);
    turkish->setText("Türkçe");
    turkish->setProperty(kLanguageKey, QLocale::Turkish);
    ukrainian->setText("Українська");
    ukrainian->setProperty(kLanguageKey, QLocale::Ukrainian);

    languageHowToAddLink->setLink(QUrl("https://starc.app/translate"));
    translationProgressLabel->hide();
    improveButton->hide();
    browseButton->setVisible(QCoreApplication::applicationVersion().contains("dev"));

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(translationProgressLabel, 0, Qt::AlignVCenter);
    buttonsLayout->addWidget(improveButton);
    buttonsLayout->addWidget(browseButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(closeButton);

    auto projectLocationGroup = new RadioButtonGroup(_parent);
    for (auto language : languages()) {
        projectLocationGroup->add(language);
    }
}

std::vector<PercentRadioButton*> LanguageDialog::Implementation::languages() const
{
    return {
        arabic,     azerbaijani,      belarusian, catalan,   chinese,   croatian, danish,
        dutch,      english,          french,     galician,  german,    greek,    hebrew,
        hindi,      hungarian,        indonesian, italian,   korean,    persian,  polish,
        portuguese, portugueseBrazil, romanian,   russian,   slovenian, spanish,  swedish,
        tamil,      telugu,           turkish,    ukrainian,
    };
}


// ****


LanguageDialog::LanguageDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setRejectButton(d->closeButton);

    auto buildFocusChain = [](const QVector<PercentRadioButton*>& _buttons) {
        PercentRadioButton* previousButton = nullptr;
        for (auto button : _buttons) {
            if (previousButton != nullptr) {
                setTabOrder(previousButton, button);
            }
            previousButton = button;
        }
    };
    buildFocusChain({
        d->azerbaijani, d->belarusian, d->catalan,
        d->danish,      d->german,     d->english,
        d->spanish,     d->greek,      d->french,
        d->galician,    d->croatian,   d->indonesian,
        d->italian,     d->hungarian,  d->dutch,
        d->polish,      d->portuguese, d->portugueseBrazil,
        d->romanian,    d->russian,    d->slovenian,
        d->swedish,     d->turkish,    d->ukrainian,
        d->arabic,      d->chinese,    d->hebrew,
        d->hindi,       d->persian,    d->tamil,
        d->telugu,      d->korean,
    });

    int row = 0;
    int column = 0;
    int maximumRow = 0;
    auto addButton = [this, &row, &column](PercentRadioButton* _button) {
        if (_button->percents <= 20) {
            _button->hide();
            return;
        }
        contentsLayout()->addWidget(_button, row++, column);
    };
    addButton(d->azerbaijani);
    addButton(d->belarusian);
    addButton(d->catalan);
    addButton(d->danish);
    addButton(d->german);
    addButton(d->english);
    addButton(d->spanish);
    addButton(d->greek);
    addButton(d->french);
    maximumRow = std::max(row, maximumRow);
    //
    row = 0;
    ++column;
    addButton(d->galician);
    addButton(d->croatian);
    addButton(d->indonesian);
    addButton(d->italian);
    addButton(d->hungarian);
    addButton(d->dutch);
    addButton(d->polish);
    addButton(d->portuguese);
    addButton(d->portugueseBrazil);
    maximumRow = std::max(row, maximumRow);
    //
    row = 0;
    ++column;
    addButton(d->romanian);
    addButton(d->russian);
    addButton(d->slovenian);
    addButton(d->swedish);
    addButton(d->turkish);
    addButton(d->ukrainian);
    maximumRow = std::max(row, maximumRow);
    //
    row = 0;
    ++column;
    addButton(d->arabic);
    addButton(d->hebrew);
    addButton(d->hindi);
    addButton(d->persian);
    addButton(d->tamil);
    addButton(d->telugu);
    addButton(d->chinese);
    addButton(d->korean);
    maximumRow = std::max(row, maximumRow);
    //
    contentsLayout()->setRowStretch(maximumRow++, 1);
    contentsLayout()->setColumnStretch(4, 1);
    contentsLayout()->addWidget(d->languageHowToAddLink, row++, 0, 1, 5);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0, 1, 5);

    for (auto radioButton : d->languages()) {
        connect(radioButton, &PercentRadioButton::checkedChanged, this,
                [this, radioButton](bool _checked) {
                    if (_checked) {
                        auto language = radioButton->property(kLanguageKey).toInt();
                        emit languageChanged(static_cast<QLocale::Language>(language));
                        if (radioButton->percents == 100) {
                            d->translationProgressLabel->hide();
                            d->improveButton->hide();
                        } else {
                            d->translationProgressLabel->setText(
                                tr("Translation is ready for %1%").arg(radioButton->percents));
                            d->translationProgressLabel->show();
                            d->improveButton->show();
                        }
                    }
                });
    }
    connect(d->improveButton, &Button::clicked, d->languageHowToAddLink, &Body1LinkLabel::clicked);
    connect(d->browseButton, &Button::clicked, this, [this] {
        const auto translationPath = QFileDialog::getOpenFileName(
            this, tr("Select file with translation"),
            QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).constFirst(),
            QString("%1 (*.qm)").arg(tr("Compiled Qt translation files")));
        if (translationPath.isEmpty()) {
            return;
        }

        emit languageFileChanged(translationPath);
    });
    connect(d->closeButton, &Button::clicked, this, &LanguageDialog::hideDialog);
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
    return d->azerbaijani;
}

QWidget* LanguageDialog::lastFocusableWidget() const
{
    return d->closeButton;
}

void LanguageDialog::updateTranslations()
{
    setTitle(tr("Change application language"));

    d->languageHowToAddLink->setText(
        tr("Did not find your preffered language? Read how you can add it yourself."));

    d->improveButton->setText(tr("Improve"));
    d->browseButton->setText(tr("Browse"));
    d->closeButton->setText(tr("Close"));
}

void LanguageDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentMaximumWidth(DesignSystem::layout().px(800));

    const auto languages = d->languages();
    for (auto languge : languages) {
        languge->setBackgroundColor(DesignSystem::color().background());
        languge->setTextColor(DesignSystem::color().onBackground());
    }

    d->languageHowToAddLink->setContentsMargins(DesignSystem::label().margins().toMargins());
    d->languageHowToAddLink->setBackgroundColor(DesignSystem::color().background());
    d->languageHowToAddLink->setTextColor(DesignSystem::color().accent());
    d->translationProgressLabel->setContentsMargins(DesignSystem::layout().px12(), 0,
                                                    DesignSystem::layout().px12(), 0);
    d->translationProgressLabel->setBackgroundColor(DesignSystem::color().background());
    d->translationProgressLabel->setTextColor(DesignSystem::color().onBackground());

    for (auto button : {
             d->improveButton,
             d->browseButton,
             d->closeButton,
         }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
    }
    UiHelper::initColorsFor(d->improveButton, UiHelper::DialogDefault);
    UiHelper::initColorsFor(d->browseButton, UiHelper::DialogDefault);
    UiHelper::initColorsFor(d->closeButton, UiHelper::DialogAccept);

    contentsLayout()->setSpacing(static_cast<int>(DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(DesignSystem::layout().px12(), DesignSystem::layout().px12(),
                  DesignSystem::layout().px16(), DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
