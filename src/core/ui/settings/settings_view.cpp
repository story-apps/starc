#include "settings_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/slider/slider.h>
#include <ui/widgets/text_field/text_field.h>

#include <QGridLayout>
#include <QLocale>
#include <QScrollArea>
#include <QStandardItemModel>


namespace Ui
{

namespace {

/**
 * @brief Сформиовать компоновщик для строки настроек
 */
QHBoxLayout* makeLayout() {
    auto layout = new QHBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    return layout;
};

/**
 * @brief Получить название языка установленной локали
 */
QString currentLanguage() {
    switch (QLocale().language()) {
        case QLocale::English: {
            return "English";
        }
        case QLocale::Russian: {
            return "Русский";
        }
        default: {
            return QLocale::languageToString(QLocale().language());
        }
    }
}

QAbstractItemModel* buildSpellCheckerLanguagesModel() {
    auto model = new QStandardItemModel;
    model->appendRow(new QStandardItem("English AU"));
    model->appendRow(new QStandardItem("English UK"));
    model->appendRow(new QStandardItem("English US"));
    model->appendRow(new QStandardItem("Russian"));
    model->appendRow(new QStandardItem("Russian with Yo"));
    return model;
}

/**
 * @brief Получить название текущей темы
 */
QString currentTheme() {
    switch (Ui::DesignSystem::theme()) {
        case Ui::ApplicationTheme::Dark: {
            return SettingsView::tr("Dark");
        }
        case Ui::ApplicationTheme::Light: {
            return SettingsView::tr("Light");
        }
        case Ui::ApplicationTheme::DarkAndLight: {
            return SettingsView::tr("Dark and light");
        }
        default: {
            return SettingsView::tr("Custom");
        }
    }
}
}

class SettingsView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Настроить карточку параметров приложения
     */
    void initApplicationCard();

    /**
     * @brief Настроить карточку параметров компонентов
     */
    void initComponentsCard();

    /**
     * @brief Настроить карточку горячих клавиш
     */
    void initShortcutsCard();


    QScrollArea* content = nullptr;

    Card* applicationCard = nullptr;
    QGridLayout* applicationCardLayout = nullptr;
    //
    H5Label* applicationTitle = nullptr;
    Body1Label* language = nullptr;
    Button* changeLanuage = nullptr;
    CheckBox* useSpellChecker = nullptr;
    ComboBox* spellCheckerLanguage = nullptr;
    QAbstractItemModel* spellCheckerLanguagesModel = nullptr;
    //
    H6Label* applicationUserInterfaceTitle = nullptr;
    Body1Label* theme = nullptr;
    Button* changeTheme = nullptr;
    Body1Label* scaleFactorTitle = nullptr;
    Slider* scaleFactor = nullptr;
    Body2Label* scaleFactorSmallInfo = nullptr;
    Body2Label* scaleFactorBigInfo = nullptr;
    //
    H6Label* applicationSaveAndBackupTitle = nullptr;
    CheckBox* autoSave = nullptr;
    CheckBox* saveBackups = nullptr;
    TextField* backupsFolderPath = nullptr;
    //
    int applicationCardBottomSpacerIndex = 0;

    Card* componentsCard = nullptr;
    QGridLayout* componentsCardLayout = nullptr;
    //
    H5Label* componentsTitle = nullptr;
    //
    int componentsCardBottomSpacerIndex = 0;

    Card* shortcutsCard = nullptr;
    QGridLayout* shortcutsCardLayout = nullptr;
    //
    H5Label* shortcutsTitle = nullptr;
    //
    int shortcutsCardBottomSpacerIndex = 0;
};

SettingsView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent)),
      applicationCard(new Card(content)),
      applicationCardLayout(new QGridLayout),
      applicationTitle(new H5Label(applicationCard)),
      language(new Body1Label(applicationCard)),
      changeLanuage(new Button(applicationCard)),
      useSpellChecker(new CheckBox(applicationCard)),
      spellCheckerLanguage(new ComboBox(applicationCard)),
      spellCheckerLanguagesModel(buildSpellCheckerLanguagesModel()),
      applicationUserInterfaceTitle(new H6Label(applicationCard)),
      theme(new Body1Label(applicationCard)),
      changeTheme(new Button(applicationCard)),
      scaleFactorTitle(new Body1Label(applicationCard)),
      scaleFactor(new Slider(applicationCard)),
      scaleFactorSmallInfo(new Body2Label(applicationCard)),
      scaleFactorBigInfo(new Body2Label(applicationCard)),
      applicationSaveAndBackupTitle(new H6Label(applicationCard)),
      autoSave(new CheckBox(applicationCard)),
      saveBackups(new CheckBox(applicationCard)),
      backupsFolderPath(new TextField(applicationCard)),
      componentsCard(new Card(content)),
      componentsCardLayout(new QGridLayout),
      componentsTitle(new H5Label(applicationCard)),
      shortcutsCard(new Card(content)),
      shortcutsCardLayout(new QGridLayout),
      shortcutsTitle(new H5Label(applicationCard))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setVerticalScrollBar(new ScrollBar);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    initApplicationCard();
    initComponentsCard();
    initShortcutsCard();

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(applicationCard);
    layout->addWidget(componentsCard);
    layout->addWidget(shortcutsCard);
    layout->addStretch();
    contentWidget->setLayout(layout);
}

void SettingsView::Implementation::initApplicationCard()
{
    spellCheckerLanguage->setEnabled(false);
    spellCheckerLanguage->setModel(spellCheckerLanguagesModel);
    scaleFactor->setMaximumValue(4000);
    scaleFactor->setValue(1000);
    backupsFolderPath->setEnabled(false);
    backupsFolderPath->setTrailingIcon("\uf256");

    //
    // Компоновка
    //
    applicationCardLayout->setContentsMargins({});
    applicationCardLayout->setSpacing(0);
    int itemIndex = 0;
    applicationCardLayout->addWidget(applicationTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(language, 0, Qt::AlignCenter);
        layout->addWidget(changeLanuage);
        layout->addStretch();
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    {
        spellCheckerLanguage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto layout = makeLayout();
        layout->addWidget(useSpellChecker, 0, Qt::AlignCenter);
        layout->addWidget(spellCheckerLanguage);
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... интерфейс
    //
    applicationCardLayout->addWidget(applicationUserInterfaceTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(theme, 0, Qt::AlignCenter);
        layout->addWidget(changeTheme);
        layout->addStretch();
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    applicationCardLayout->addWidget(scaleFactorTitle, itemIndex++, 0);
    applicationCardLayout->addWidget(scaleFactor, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(scaleFactorSmallInfo);
        layout->addStretch();
        layout->addWidget(scaleFactorBigInfo);
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... сохранение и бэкапы
    //
    applicationCardLayout->addWidget(applicationSaveAndBackupTitle, itemIndex++, 0);
    applicationCardLayout->addWidget(autoSave, itemIndex++, 0);
    {
        backupsFolderPath->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto layout = makeLayout();
        layout->addWidget(saveBackups, 0, Qt::AlignCenter);
        layout->addWidget(backupsFolderPath);
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    applicationCardBottomSpacerIndex = itemIndex;
    applicationCard->setLayoutReimpl(applicationCardLayout);
}

void SettingsView::Implementation::initComponentsCard()
{
    componentsCardLayout->setContentsMargins({});
    componentsCardLayout->setSpacing(0);
    int itemIndex = 0;
    componentsCardLayout->addWidget(componentsTitle, itemIndex++, 0);
    componentsCardBottomSpacerIndex = itemIndex;
    componentsCard->setLayoutReimpl(componentsCardLayout);
}

void SettingsView::Implementation::initShortcutsCard()
{
    shortcutsCardLayout->setContentsMargins({});
    shortcutsCardLayout->setSpacing(0);
    int itemIndex = 0;
    shortcutsCardLayout->addWidget(shortcutsTitle, itemIndex++, 0);
    shortcutsCardBottomSpacerIndex = itemIndex;
    shortcutsCard->setLayoutReimpl(shortcutsCardLayout);
}


// ****


SettingsView::SettingsView(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->useSpellChecker, &CheckBox::checkedChanged, d->spellCheckerLanguage, &ComboBox::setEnabled);
    connect(d->saveBackups, &CheckBox::checkedChanged, d->backupsFolderPath, &ComboBox::setEnabled);

    designSystemChangeEvent(nullptr);
}

void SettingsView::updateTranslations()
{
    d->applicationTitle->setText(tr("Application settings"));
    d->language->setText(tr("Language"));
    d->changeLanuage->setText(currentLanguage());
    d->useSpellChecker->setText(tr("Spell check"));
    d->spellCheckerLanguage->setLabel(tr("Spelling dictionary"));
    d->applicationUserInterfaceTitle->setText(tr("User interface"));
    d->theme->setText(tr("Theme"));
    d->changeTheme->setText(currentTheme());
    d->scaleFactorTitle->setText(tr("Size of the user interface elements:"));
    d->scaleFactorSmallInfo->setText(tr("small"));
    d->scaleFactorBigInfo->setText(tr("big"));
    d->applicationSaveAndBackupTitle->setText(tr("Save changes & backups"));
    d->autoSave->setText(tr("Automatically save changes as soon as possible"));
    d->autoSave->setToolTip(tr("Autosave works very accurately.\n"
                               "It saves the project every 3 seconds if you do not use your mouse or keyboard.\n"
                               "If you work with no interruptions it saves the project every 3 minutes."));
    d->saveBackups->setText(tr("Save backups"));
    d->backupsFolderPath->setLabel(tr("Backups folder path"));

    d->componentsTitle->setText(tr("Components"));

    d->shortcutsTitle->setText(tr("Shortcuts"));
}

SettingsView::~SettingsView() = default;

void SettingsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().surface());
    d->content->widget()->layout()->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                                                                 Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24()).toMargins());

    for (auto card : { d->applicationCard }) {
        card->setBackgroundColor(DesignSystem::color().background());
    }

    auto titleColor = DesignSystem::color().onBackground();
    titleColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    for (auto title : QVector<Widget*>{ d->applicationTitle,
                                        d->applicationUserInterfaceTitle,
                                        d->applicationSaveAndBackupTitle,
                                        d->componentsTitle,
                                        d->shortcutsTitle }) {
        title->setBackgroundColor(DesignSystem::color().background());
        title->setTextColor(titleColor);
        title->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    }

    auto labelMargins = Ui::DesignSystem::label().margins().toMargins();
    labelMargins.setTop(static_cast<int>(Ui::DesignSystem::button().shadowMargins().top()));
    labelMargins.setBottom(static_cast<int>(Ui::DesignSystem::button().shadowMargins().bottom()));
    for (auto label : QVector<Widget*>{ d->language,
                                        d->theme,
                                        d->scaleFactorTitle, d->scaleFactorSmallInfo, d->scaleFactorBigInfo }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(DesignSystem::color().onBackground());
        label->setContentsMargins(labelMargins);
    }

    for (auto checkBox : { d->useSpellChecker,
                           d->autoSave,
                           d->saveBackups }) {
        checkBox->setBackgroundColor(DesignSystem::color().background());
        checkBox->setTextColor(DesignSystem::color().onBackground());
    }

    for (auto button : { d->changeLanuage,
                         d->changeTheme }) {
        button->setBackgroundColor(DesignSystem::color().secondary());
        button->setTextColor(DesignSystem::color().secondary());
    }

    d->scaleFactor->setBackgroundColor(DesignSystem::color().background());
    d->scaleFactor->setContentsMargins({static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                           static_cast<int>(Ui::DesignSystem::layout().px24()), 0});

    d->applicationCardLayout->setRowMinimumHeight(d->applicationCardBottomSpacerIndex,
                                                  static_cast<int>(Ui::DesignSystem::layout().px24()));
}

} // namespace Ui
