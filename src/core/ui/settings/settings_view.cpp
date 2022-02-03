#include "settings_view.h"

#include "widgets/theme_preview.h"

#include <business_layer/chronometry/chronometer.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/slider/slider.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/tree/tree.h>
#include <ui/widgets/tree/tree_delegate.h>
#include <ui/widgets/tree/tree_header_view.h>

#include <QAction>
#include <QFileDialog>
#include <QGridLayout>
#include <QLocale>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QVariantAnimation>
#include <QtMath>


namespace Ui {

namespace {

/**
 * @brief Сформиовать компоновщик для строки настроек
 */
QHBoxLayout* makeLayout()
{
    auto layout = new QHBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    return layout;
};

/**
 * @brief Карта соответствия названий языков для проверки орфографии с их кодами
 * @link https://github.com/wooorm/dictionaries/
 * @link https://cgit.freedesktop.org/libreoffice/dictionaries/tree/
 * @link https://www.softmaker.com/en/dictionaries
 */
const QVector<QString> kSpellCheckerLanguagesNameToCode = {
    "af",         "an",          "ar",    "az",       "be",    "bg",    "bn",    "bo",    "br",
    "bs",         "ca-valencia", "ca",    "cs",       "cy",    "da",    "de-AT", "de-CH", "de",
    "el-polyton", "el",          "en-AU", "en-CA",    "en-GB", "en-NZ", "en-ZA", "en",    "eo",
    "es-AR",      "es-BO",       "es-CL", "es-CO",    "es-CR", "es-CU", "es-DO", "es-EC", "es-GT",
    "es-HN",      "es-MX",       "es-NI", "es-PA",    "es-PE", "es-PH", "es-PR", "es-PY", "es-SV",
    "es-US",      "es-UY",       "es-VE", "es",       "et",    "eu",    "fa",    "fo",    "fr",
    "fur",        "fy",          "ga",    "gd",       "gl",    "gu",    "gug",   "he",    "hi",
    "hr",         "hu",          "hy",    "hyw",      "ia",    "id",    "is",    "it",    "ka",
    "kk",         "kmr",         "ko",    "la",       "lb",    "lo",    "lt",    "ltg",   "lv",
    "mk",         "mn",          "mt",    "nb",       "nds",   "ne",    "nl",    "nn",    "oc",
    "pl",         "pt-BR",       "pt",    "qu",       "ro",    "ru-yo", "ru",    "rw",    "si",
    "sk",         "sl",          "sq",    "sr-Latn",  "sr",    "sv-FI", "sv",    "sw",    "ta",
    "te",         "th",          "tk",    "tlh-Latn", "tlh",   "tr",    "uk",    "vi"
};

/**
 * @brief Индекс для сохранения в модели информации о коде языка
 */
const int kSpellCheckerLanguageCodeRole = Qt::UserRole + 1;

/**
 * @brief Построить модель для всех доступных справочников проверки орфографии
 */
QStandardItemModel* buildSpellCheckerLanguagesModel(QObject* _parent)
{
    auto model = new QStandardItemModel(_parent);
    for (const auto& language : kSpellCheckerLanguagesNameToCode) {
        auto item = new QStandardItem(language);
        item->setData(language, kSpellCheckerLanguageCodeRole);
        model->appendRow(item);
    }
    return model;
}

/**
 * @brief Построить модель типов абзацев сценария
 */
QStringListModel* buildScreenplayParagraphTypesModel(QObject* _parent,
                                                     QStringListModel* _model = nullptr)
{
    using namespace BusinessLayer;
    const QStringList paragraphTypes = {
        toDisplayString(ScreenplayParagraphType::SceneHeading),
        toDisplayString(ScreenplayParagraphType::SceneCharacters),
        toDisplayString(ScreenplayParagraphType::Action),
        toDisplayString(ScreenplayParagraphType::Character),
        toDisplayString(ScreenplayParagraphType::Parenthetical),
        toDisplayString(ScreenplayParagraphType::Dialogue),
        toDisplayString(ScreenplayParagraphType::Lyrics),
        toDisplayString(ScreenplayParagraphType::Transition),
        toDisplayString(ScreenplayParagraphType::Shot),
        toDisplayString(ScreenplayParagraphType::InlineNote),
        toDisplayString(ScreenplayParagraphType::FolderHeader),
        toDisplayString(ScreenplayParagraphType::UnformattedText),
    };

    if (!_model) {
        _model = new QStringListModel(_parent);
    }
    _model->setStringList(paragraphTypes);

    return _model;
}

/**
 * @brief Колонки таблицы горячих клавиш специализированного редактора текста
 */
enum TextEditorShortcuts {
    Type = 0,
    Shortcut,
    JumpByTab,
    JumpByEnter,
    ChangeByTab,
    ChangeByEnter,
};

} // namespace

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
    void initSimpleTextCard();
    void initScreenplayCard();
    void initComicBookCard();

    /**
     * @brief Настроить карточку горячих клавиш
     */
    void initShortcutsCard();

    /**
     * @brief Проскролить представление до заданного виджета
     */
    void scrollToTitle(AbstractLabel* title);

    /**
     * @brief Обновить размеры колонок в таблицах
     */
    void updateTablesGeometry();


    QScrollArea* content = nullptr;
    QVariantAnimation scrollAnimation;
    ContextMenu* contextMenu = nullptr;

    QVariantAnimation colorAnimation;
    AbstractLabel* colorableTitle = nullptr;

    //
    // Application
    //
    Card* applicationCard = nullptr;
    QGridLayout* applicationCardLayout = nullptr;
    H5Label* applicationTitle = nullptr;
    //
    // ... Common
    //
    Body1Label* language = nullptr;
    Button* changeLanuage = nullptr;
    //
    // ... User interface
    //
    H6Label* applicationUserInterfaceTitle = nullptr;
    QHBoxLayout* applicationThemesLayout = nullptr;
    ThemePreview* lightTheme = nullptr;
    ThemePreview* darkAndLightTheme = nullptr;
    ThemePreview* darkTheme = nullptr;
    ThemePreview* customTheme = nullptr;
    Body1Label* scaleFactorTitle = nullptr;
    Slider* scaleFactor = nullptr;
    Body2Label* scaleFactorSmallInfo = nullptr;
    Body2Label* scaleFactorBigInfo = nullptr;
    //
    // ... Save changes & backups
    //
    H6Label* applicationSaveAndBackupTitle = nullptr;
    CheckBox* autoSave = nullptr;
    CheckBox* saveBackups = nullptr;
    TextField* backupsFolderPath = nullptr;
    //
    // ... Text editing
    //
    H6Label* applicationTextEditingTitle = nullptr;
    CheckBox* showDocumentsPages = nullptr;
    CheckBox* useTypewriterSound = nullptr;
    CheckBox* useSpellChecker = nullptr;
    ComboBox* spellCheckerLanguage = nullptr;
    QStandardItemModel* spellCheckerLanguagesModel = nullptr;
    IconButton* spellCheckerUserDictionary = nullptr;
    CheckBox* highlightCurrentLine = nullptr;
    CheckBox* focusCurrentParagraph = nullptr;
    CheckBox* useTypewriterScrolling = nullptr;
    //
    int applicationCardBottomSpacerIndex = 0;

    H5Label* componentsTitle = nullptr;

    //
    // Comonents
    //
    // Simple text
    //
    Card* simpleTextCard = nullptr;
    QGridLayout* simpleTextCardLayout = nullptr;
    H5Label* simpleTextTitle = nullptr;
    //
    // ... Simple text editor
    //
    H6Label* simpleTextEditorTitle = nullptr;
    ComboBox* simpleTextEditorDefaultTemplate = nullptr;
    IconButton* simpleTextEditorDefaultTemplateOptions = nullptr;
    //
    // ... Simple text navigator
    //
    H6Label* simpleTextNavigatorTitle = nullptr;
    CheckBox* simpleTextNavigatorShowSceneText = nullptr;
    RadioButton* simpleTextNavigatorSceneDescriptionLines1 = nullptr;
    RadioButton* simpleTextNavigatorSceneDescriptionLines2 = nullptr;
    RadioButton* simpleTextNavigatorSceneDescriptionLines3 = nullptr;
    RadioButton* simpleTextNavigatorSceneDescriptionLines4 = nullptr;
    RadioButton* simpleTextNavigatorSceneDescriptionLines5 = nullptr;
    //
    int simpleTextCardBottomSpacerIndex = 0;
    //
    // Screenplay
    //
    Card* screenplayCard = nullptr;
    QGridLayout* screenplayCardLayout = nullptr;
    H5Label* screenplayTitle = nullptr;
    //
    // ... Screenplay editor
    //
    H6Label* screenplayEditorTitle = nullptr;
    ComboBox* screenplayEditorDefaultTemplate = nullptr;
    IconButton* screenplayEditorDefaultTemplateOptions = nullptr;
    CheckBox* screenplayEditorShowSceneNumber = nullptr;
    CheckBox* screenplayEditorShowSceneNumberOnLeft = nullptr;
    CheckBox* screenplayEditorShowSceneNumberOnRight = nullptr;
    CheckBox* screenplayEditorShowDialogueNumber = nullptr;
    CheckBox* screenplayEditorContinueDialogue = nullptr;
    CheckBox* screenplayEditorUseCharactersFromText = nullptr;
    //
    // ... Screenplay navigator
    //
    H6Label* screenplayNavigatorTitle = nullptr;
    CheckBox* screenplayNavigatorShowSceneNumber = nullptr;
    CheckBox* screenplayNavigatorShowSceneText = nullptr;
    RadioButton* screenplayNavigatorSceneDescriptionLines1 = nullptr;
    RadioButton* screenplayNavigatorSceneDescriptionLines2 = nullptr;
    RadioButton* screenplayNavigatorSceneDescriptionLines3 = nullptr;
    RadioButton* screenplayNavigatorSceneDescriptionLines4 = nullptr;
    RadioButton* screenplayNavigatorSceneDescriptionLines5 = nullptr;
    //
    // ... Screenplay duration
    //
    H6Label* screenplayDurationTitle = nullptr;
    RadioButton* screenplayDurationByPage = nullptr;
    TextField* screenplayDurationByPagePage = nullptr;
    TextField* screenplayDurationByPageDuration = nullptr;
    RadioButton* screenplayDurationByCharacters = nullptr;
    TextField* screenplayDurationByCharactersCharacters = nullptr;
    CheckBox* screenplayDurationByCharactersIncludingSpaces = nullptr;
    TextField* screenplayDurationByCharactersDuration = nullptr;
    //
    int screenplayCardBottomSpacerIndex = 0;
    //
    // Comic book
    //
    Card* comicBookCard = nullptr;
    QGridLayout* comicBookCardLayout = nullptr;
    H5Label* comicBookTitle = nullptr;
    //
    // ... Comic book editor
    //
    H6Label* comicBookEditorTitle = nullptr;
    ComboBox* comicBookEditorDefaultTemplate = nullptr;
    IconButton* comicBookEditorDefaultTemplateOptions = nullptr;
    //
    // ... Comic book navigator
    //
    H6Label* comicBookNavigatorTitle = nullptr;
    CheckBox* comicBookNavigatorShowSceneText = nullptr;
    RadioButton* comicBookNavigatorSceneDescriptionLines1 = nullptr;
    RadioButton* comicBookNavigatorSceneDescriptionLines2 = nullptr;
    RadioButton* comicBookNavigatorSceneDescriptionLines3 = nullptr;
    RadioButton* comicBookNavigatorSceneDescriptionLines4 = nullptr;
    RadioButton* comicBookNavigatorSceneDescriptionLines5 = nullptr;
    //
    int comicBookCardBottomSpacerIndex = 0;

    Card* shortcutsCard = nullptr;
    QGridLayout* shortcutsCardLayout = nullptr;
    //
    H5Label* shortcutsTitle = nullptr;
    H6Label* shortcutsForScreenplayTitle = nullptr;
    Tree* shortcutsForScreenplay = nullptr;
    HierarchicalModel* shortcutsForScreenplayModel = nullptr;
    QStringListModel* screenplayParagraphTypesModel = nullptr;
    ComboBoxItemDelegate* screenplayParagraphAddTypeDelegate = nullptr;
    ComboBoxItemDelegate* screenplayParagraphChangeTypeDelegate = nullptr;
    //
    int shortcutsCardBottomSpacerIndex = 0;
};

SettingsView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , contextMenu(new ContextMenu(_parent))
    //
    , applicationCard(new Card(content))
    , applicationCardLayout(new QGridLayout)
    , applicationTitle(new H5Label(applicationCard))
    , language(new Body1Label(applicationCard))
    , changeLanuage(new Button(applicationCard))
    , applicationUserInterfaceTitle(new H6Label(applicationCard))
    , applicationThemesLayout(new QHBoxLayout)
    , lightTheme(new ThemePreview(applicationCard))
    , darkAndLightTheme(new ThemePreview(applicationCard))
    , darkTheme(new ThemePreview(applicationCard))
    , customTheme(new ThemePreview(applicationCard))
    , scaleFactorTitle(new Body1Label(applicationCard))
    , scaleFactor(new Slider(applicationCard))
    , scaleFactorSmallInfo(new Body2Label(applicationCard))
    , scaleFactorBigInfo(new Body2Label(applicationCard))
    , applicationSaveAndBackupTitle(new H6Label(applicationCard))
    , autoSave(new CheckBox(applicationCard))
    , saveBackups(new CheckBox(applicationCard))
    , backupsFolderPath(new TextField(applicationCard))
    , applicationTextEditingTitle(new H6Label(applicationCard))
    , showDocumentsPages(new CheckBox(applicationCard))
    , useTypewriterSound(new CheckBox(applicationCard))
    , useSpellChecker(new CheckBox(applicationCard))
    , spellCheckerLanguage(new ComboBox(applicationCard))
    , spellCheckerLanguagesModel(buildSpellCheckerLanguagesModel(spellCheckerLanguage))
    , spellCheckerUserDictionary(new IconButton(applicationCard))
    , highlightCurrentLine(new CheckBox(applicationCard))
    , focusCurrentParagraph(new CheckBox(applicationCard))
    , useTypewriterScrolling(new CheckBox(applicationCard))
    //
    , componentsTitle(new H5Label(content))
    //
    , simpleTextCard(new Card(content))
    , simpleTextCardLayout(new QGridLayout)
    , simpleTextTitle(new H5Label(simpleTextCard))
    , simpleTextEditorTitle(new H6Label(simpleTextCard))
    , simpleTextEditorDefaultTemplate(new ComboBox(simpleTextCard))
    , simpleTextEditorDefaultTemplateOptions(new IconButton(simpleTextCard))
    , simpleTextNavigatorTitle(new H6Label(simpleTextCard))
    , simpleTextNavigatorShowSceneText(new CheckBox(simpleTextCard))
    , simpleTextNavigatorSceneDescriptionLines1(new RadioButton(simpleTextCard))
    , simpleTextNavigatorSceneDescriptionLines2(new RadioButton(simpleTextCard))
    , simpleTextNavigatorSceneDescriptionLines3(new RadioButton(simpleTextCard))
    , simpleTextNavigatorSceneDescriptionLines4(new RadioButton(simpleTextCard))
    , simpleTextNavigatorSceneDescriptionLines5(new RadioButton(simpleTextCard))
    //
    , screenplayCard(new Card(content))
    , screenplayCardLayout(new QGridLayout)
    , screenplayTitle(new H5Label(screenplayCard))
    , screenplayEditorTitle(new H6Label(screenplayCard))
    , screenplayEditorDefaultTemplate(new ComboBox(screenplayCard))
    , screenplayEditorDefaultTemplateOptions(new IconButton(screenplayCard))
    , screenplayEditorShowSceneNumber(new CheckBox(screenplayCard))
    , screenplayEditorShowSceneNumberOnLeft(new CheckBox(screenplayCard))
    , screenplayEditorShowSceneNumberOnRight(new CheckBox(screenplayCard))
    , screenplayEditorShowDialogueNumber(new CheckBox(screenplayCard))
    , screenplayEditorContinueDialogue(new CheckBox(screenplayCard))
    , screenplayEditorUseCharactersFromText(new CheckBox(screenplayCard))
    , screenplayNavigatorTitle(new H6Label(screenplayCard))
    , screenplayNavigatorShowSceneNumber(new CheckBox(screenplayCard))
    , screenplayNavigatorShowSceneText(new CheckBox(screenplayCard))
    , screenplayNavigatorSceneDescriptionLines1(new RadioButton(screenplayCard))
    , screenplayNavigatorSceneDescriptionLines2(new RadioButton(screenplayCard))
    , screenplayNavigatorSceneDescriptionLines3(new RadioButton(screenplayCard))
    , screenplayNavigatorSceneDescriptionLines4(new RadioButton(screenplayCard))
    , screenplayNavigatorSceneDescriptionLines5(new RadioButton(screenplayCard))
    , screenplayDurationTitle(new H6Label(screenplayCard))
    , screenplayDurationByPage(new RadioButton(screenplayCard))
    , screenplayDurationByPagePage(new TextField(screenplayCard))
    , screenplayDurationByPageDuration(new TextField(screenplayCard))
    , screenplayDurationByCharacters(new RadioButton(screenplayCard))
    , screenplayDurationByCharactersCharacters(new TextField(screenplayCard))
    , screenplayDurationByCharactersIncludingSpaces(new CheckBox(screenplayCard))
    , screenplayDurationByCharactersDuration(new TextField(screenplayCard))
    //
    , comicBookCard(new Card(content))
    , comicBookCardLayout(new QGridLayout)
    , comicBookTitle(new H5Label(comicBookCard))
    , comicBookEditorTitle(new H6Label(comicBookCard))
    , comicBookEditorDefaultTemplate(new ComboBox(comicBookCard))
    , comicBookEditorDefaultTemplateOptions(new IconButton(comicBookCard))
    , comicBookNavigatorTitle(new H6Label(comicBookCard))
    , comicBookNavigatorShowSceneText(new CheckBox(comicBookCard))
    , comicBookNavigatorSceneDescriptionLines1(new RadioButton(comicBookCard))
    , comicBookNavigatorSceneDescriptionLines2(new RadioButton(comicBookCard))
    , comicBookNavigatorSceneDescriptionLines3(new RadioButton(comicBookCard))
    , comicBookNavigatorSceneDescriptionLines4(new RadioButton(comicBookCard))
    , comicBookNavigatorSceneDescriptionLines5(new RadioButton(comicBookCard))
    //
    , shortcutsCard(new Card(content))
    , shortcutsCardLayout(new QGridLayout)
    , shortcutsTitle(new H5Label(shortcutsCard))
    , shortcutsForScreenplayTitle(new H6Label(shortcutsCard))
    , shortcutsForScreenplay(new Tree(shortcutsCard))
    , screenplayParagraphTypesModel(buildScreenplayParagraphTypesModel(shortcutsForScreenplay))
    , screenplayParagraphAddTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForScreenplay, screenplayParagraphTypesModel))
    , screenplayParagraphChangeTypeDelegate(
          new ComboBoxItemDelegate(shortcutsForScreenplay, screenplayParagraphTypesModel))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);
    scrollAnimation.setEasingCurve(QEasingCurve::OutQuad);
    scrollAnimation.setDuration(180);
    colorAnimation.setEasingCurve(QEasingCurve::InBack);
    colorAnimation.setDuration(1400);

    initApplicationCard();
    initSimpleTextCard();
    initScreenplayCard();
    initComicBookCard();
    initShortcutsCard();

    QWidget* contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    QVBoxLayout* layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(applicationCard);
    layout->addWidget(componentsTitle);
    layout->addWidget(simpleTextCard);
    layout->addWidget(screenplayCard);
    layout->addWidget(comicBookCard);
    layout->addWidget(shortcutsCard);
    layout->addStretch();
}

void SettingsView::Implementation::initApplicationCard()
{
    lightTheme->setTheme(ApplicationTheme::Light);
    darkAndLightTheme->setTheme(ApplicationTheme::DarkAndLight);
    darkTheme->setTheme(ApplicationTheme::Dark);
    customTheme->setTheme(ApplicationTheme::Custom);
    // 0 - 0.5, 500 - 1, 3500 - 4
    scaleFactor->setMaximumValue(3500);
    scaleFactor->setValue(500);
    scaleFactor->setDefaultPosition(500);
    backupsFolderPath->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    backupsFolderPath->setEnabled(false);
    backupsFolderPath->setTrailingIcon(u8"\U000f0256");
    spellCheckerLanguage->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    spellCheckerLanguage->setEnabled(false);
    spellCheckerLanguage->setModel(spellCheckerLanguagesModel);
    spellCheckerUserDictionary->setIcon(u8"\U000F0900");
    spellCheckerUserDictionary->hide();

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
    //
    // ... интерфейс
    //
    applicationCardLayout->addWidget(applicationUserInterfaceTitle, itemIndex++, 0);
    {
        applicationThemesLayout->setContentsMargins({});
        applicationThemesLayout->setSpacing(0);
        applicationThemesLayout->addWidget(lightTheme);
        applicationThemesLayout->addWidget(darkAndLightTheme);
        applicationThemesLayout->addWidget(darkTheme);
        applicationThemesLayout->addWidget(customTheme);
        applicationThemesLayout->addStretch();
        applicationCardLayout->addLayout(applicationThemesLayout, itemIndex++, 0);
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
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        policy.setHeightForWidth(true);
        backupsFolderPath->setSizePolicy(policy);

        auto layout = makeLayout();
        layout->addWidget(saveBackups, 0, Qt::AlignCenter);
        layout->addWidget(backupsFolderPath);
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... параметры редакторов текста
    //
    applicationCardLayout->addWidget(applicationTextEditingTitle, itemIndex++, 0);
    {
        spellCheckerLanguage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto layout = makeLayout();
        layout->addWidget(useSpellChecker, 0, Qt::AlignCenter);
        layout->addWidget(spellCheckerLanguage);
        layout->addWidget(spellCheckerUserDictionary);
        applicationCardLayout->addLayout(layout, itemIndex++, 0);
    }
    applicationCardLayout->addWidget(showDocumentsPages, itemIndex++, 0);
    applicationCardLayout->addWidget(useTypewriterSound, itemIndex++, 0);
    applicationCardLayout->addWidget(useTypewriterScrolling, itemIndex++, 0);
    applicationCardLayout->addWidget(focusCurrentParagraph, itemIndex++, 0);
    applicationCardLayout->addWidget(highlightCurrentLine, itemIndex++, 0);

    applicationCardBottomSpacerIndex = itemIndex;
    applicationCard->setLayoutReimpl(applicationCardLayout);
}

void SettingsView::Implementation::initSimpleTextCard()
{
    simpleTextEditorDefaultTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    simpleTextEditorDefaultTemplate->setModel(
        BusinessLayer::TemplatesFacade::simpleTextTemplates());
    simpleTextEditorDefaultTemplateOptions->setIcon(u8"\U000F01D9");
    simpleTextEditorDefaultTemplateOptions->hide();
    //
    auto linesGroup = new RadioButtonGroup(simpleTextCard);
    linesGroup->add(simpleTextNavigatorSceneDescriptionLines1);
    linesGroup->add(simpleTextNavigatorSceneDescriptionLines2);
    linesGroup->add(simpleTextNavigatorSceneDescriptionLines3);
    linesGroup->add(simpleTextNavigatorSceneDescriptionLines4);
    linesGroup->add(simpleTextNavigatorSceneDescriptionLines5);
    simpleTextNavigatorSceneDescriptionLines1->setEnabled(false);
    simpleTextNavigatorSceneDescriptionLines1->setChecked(true);
    simpleTextNavigatorSceneDescriptionLines2->setEnabled(false);
    simpleTextNavigatorSceneDescriptionLines3->setEnabled(false);
    simpleTextNavigatorSceneDescriptionLines4->setEnabled(false);
    simpleTextNavigatorSceneDescriptionLines5->setEnabled(false);


    //
    // Компоновка
    //
    simpleTextCardLayout->setContentsMargins({});
    simpleTextCardLayout->setSpacing(0);
    int itemIndex = 0;
    simpleTextCardLayout->addWidget(simpleTextTitle, itemIndex++, 0);
    //
    // ... редактор текста
    //
    simpleTextCardLayout->addWidget(simpleTextEditorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(simpleTextEditorDefaultTemplate, 1);
        layout->addWidget(simpleTextEditorDefaultTemplateOptions);
        simpleTextCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... навигатор текста
    //
    simpleTextCardLayout->addWidget(simpleTextNavigatorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(simpleTextNavigatorShowSceneText);
        layout->addWidget(simpleTextNavigatorSceneDescriptionLines1);
        layout->addWidget(simpleTextNavigatorSceneDescriptionLines2);
        layout->addWidget(simpleTextNavigatorSceneDescriptionLines3);
        layout->addWidget(simpleTextNavigatorSceneDescriptionLines4);
        layout->addWidget(simpleTextNavigatorSceneDescriptionLines5);
        layout->addStretch();
        simpleTextCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    simpleTextCardBottomSpacerIndex = itemIndex;
    simpleTextCard->setLayoutReimpl(simpleTextCardLayout);
}

void SettingsView::Implementation::initScreenplayCard()
{
    screenplayEditorDefaultTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayEditorDefaultTemplate->setModel(
        BusinessLayer::TemplatesFacade::screenplayTemplates());
    screenplayEditorDefaultTemplateOptions->setIcon(u8"\U000F01D9");
    screenplayEditorShowSceneNumberOnLeft->setEnabled(false);
    screenplayEditorShowSceneNumberOnLeft->setChecked(true);
    screenplayEditorShowSceneNumberOnRight->setEnabled(false);
    //
    auto linesGroup = new RadioButtonGroup(screenplayCard);
    linesGroup->add(screenplayNavigatorSceneDescriptionLines1);
    linesGroup->add(screenplayNavigatorSceneDescriptionLines2);
    linesGroup->add(screenplayNavigatorSceneDescriptionLines3);
    linesGroup->add(screenplayNavigatorSceneDescriptionLines4);
    linesGroup->add(screenplayNavigatorSceneDescriptionLines5);
    screenplayNavigatorSceneDescriptionLines1->setEnabled(false);
    screenplayNavigatorSceneDescriptionLines1->setChecked(true);
    screenplayNavigatorSceneDescriptionLines2->setEnabled(false);
    screenplayNavigatorSceneDescriptionLines3->setEnabled(false);
    screenplayNavigatorSceneDescriptionLines4->setEnabled(false);
    screenplayNavigatorSceneDescriptionLines5->setEnabled(false);
    //
    auto durationGroup = new RadioButtonGroup(screenplayCard);
    durationGroup->add(screenplayDurationByPage);
    durationGroup->add(screenplayDurationByCharacters);
    screenplayDurationByPage->setChecked(true);
    screenplayDurationByPagePage->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayDurationByPagePage->setText("1");
    screenplayDurationByPagePage->setReadOnly(true);
    screenplayDurationByPageDuration->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayDurationByCharactersCharacters->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayDurationByCharactersCharacters->setEnabled(false);
    screenplayDurationByCharactersIncludingSpaces->setEnabled(false);
    screenplayDurationByCharactersDuration->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    screenplayDurationByCharactersDuration->setEnabled(false);


    //
    // Компоновка
    //
    screenplayCardLayout->setContentsMargins({});
    screenplayCardLayout->setSpacing(0);
    int itemIndex = 0;
    screenplayCardLayout->addWidget(screenplayTitle, itemIndex++, 0);
    //
    // ... редактор сценария
    //
    screenplayCardLayout->addWidget(screenplayEditorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayEditorDefaultTemplate, 1);
        layout->addWidget(screenplayEditorDefaultTemplateOptions);
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayEditorShowSceneNumber);
        layout->addWidget(screenplayEditorShowSceneNumberOnLeft);
        layout->addWidget(screenplayEditorShowSceneNumberOnRight);
        layout->addStretch();
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    screenplayCardLayout->addWidget(screenplayEditorShowDialogueNumber, itemIndex++, 0);
    screenplayCardLayout->addWidget(screenplayEditorContinueDialogue, itemIndex++, 0);
    screenplayCardLayout->addWidget(screenplayEditorUseCharactersFromText, itemIndex++, 0);
    //
    // ... навигатор сценария
    //
    screenplayCardLayout->addWidget(screenplayNavigatorTitle, itemIndex++, 0);
    screenplayCardLayout->addWidget(screenplayNavigatorShowSceneNumber, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayNavigatorShowSceneText);
        layout->addWidget(screenplayNavigatorSceneDescriptionLines1);
        layout->addWidget(screenplayNavigatorSceneDescriptionLines2);
        layout->addWidget(screenplayNavigatorSceneDescriptionLines3);
        layout->addWidget(screenplayNavigatorSceneDescriptionLines4);
        layout->addWidget(screenplayNavigatorSceneDescriptionLines5);
        layout->addStretch();
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... счётчики хронометража
    //
    screenplayCardLayout->addWidget(screenplayDurationTitle, itemIndex++, 0);
    screenplayCardLayout->addWidget(screenplayDurationByPage, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayDurationByPagePage);
        layout->addWidget(screenplayDurationByPageDuration);
        layout->addStretch();
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    screenplayCardLayout->addWidget(screenplayDurationByCharacters, itemIndex++, 0,
                                    Qt::AlignBottom);
    {
        auto layout = makeLayout();
        layout->addWidget(screenplayDurationByCharactersCharacters);
        layout->addWidget(screenplayDurationByCharactersIncludingSpaces, 0, Qt::AlignCenter);
        layout->addWidget(screenplayDurationByCharactersDuration);
        layout->addStretch();
        screenplayCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    screenplayCardBottomSpacerIndex = itemIndex;
    screenplayCard->setLayoutReimpl(screenplayCardLayout);
}

void SettingsView::Implementation::initComicBookCard()
{
    comicBookEditorDefaultTemplate->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    comicBookEditorDefaultTemplate->setModel(BusinessLayer::TemplatesFacade::comicBookTemplates());
    comicBookEditorDefaultTemplateOptions->setIcon(u8"\U000F01D9");
    comicBookEditorDefaultTemplateOptions->hide();
    //
    auto linesGroup = new RadioButtonGroup(comicBookCard);
    linesGroup->add(comicBookNavigatorSceneDescriptionLines1);
    linesGroup->add(comicBookNavigatorSceneDescriptionLines2);
    linesGroup->add(comicBookNavigatorSceneDescriptionLines3);
    linesGroup->add(comicBookNavigatorSceneDescriptionLines4);
    linesGroup->add(comicBookNavigatorSceneDescriptionLines5);
    comicBookNavigatorSceneDescriptionLines1->setEnabled(false);
    comicBookNavigatorSceneDescriptionLines1->setChecked(true);
    comicBookNavigatorSceneDescriptionLines2->setEnabled(false);
    comicBookNavigatorSceneDescriptionLines3->setEnabled(false);
    comicBookNavigatorSceneDescriptionLines4->setEnabled(false);
    comicBookNavigatorSceneDescriptionLines5->setEnabled(false);


    //
    // Компоновка
    //
    comicBookCardLayout->setContentsMargins({});
    comicBookCardLayout->setSpacing(0);
    int itemIndex = 0;
    comicBookCardLayout->addWidget(comicBookTitle, itemIndex++, 0);
    //
    // ... редактор текста
    //
    comicBookCardLayout->addWidget(comicBookEditorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(comicBookEditorDefaultTemplate, 1);
        layout->addWidget(comicBookEditorDefaultTemplateOptions);
        comicBookCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    // ... навигатор текста
    //
    comicBookCardLayout->addWidget(comicBookNavigatorTitle, itemIndex++, 0);
    {
        auto layout = makeLayout();
        layout->addWidget(comicBookNavigatorShowSceneText);
        layout->addWidget(comicBookNavigatorSceneDescriptionLines1);
        layout->addWidget(comicBookNavigatorSceneDescriptionLines2);
        layout->addWidget(comicBookNavigatorSceneDescriptionLines3);
        layout->addWidget(comicBookNavigatorSceneDescriptionLines4);
        layout->addWidget(comicBookNavigatorSceneDescriptionLines5);
        layout->addStretch();
        comicBookCardLayout->addLayout(layout, itemIndex++, 0);
    }
    //
    comicBookCardBottomSpacerIndex = itemIndex;
    comicBookCard->setLayoutReimpl(comicBookCardLayout);
}

void SettingsView::Implementation::initShortcutsCard()
{
    shortcutsForScreenplay->setHeader(new HierarchicalHeaderView(shortcutsForScreenplay));

    shortcutsCardLayout->setContentsMargins({});
    shortcutsCardLayout->setSpacing(0);
    int itemIndex = 0;
    shortcutsCardLayout->addWidget(shortcutsTitle, itemIndex++, 0);
    //
    shortcutsCardLayout->addWidget(shortcutsForScreenplayTitle, itemIndex++, 0);
    shortcutsCardLayout->addWidget(shortcutsForScreenplay, itemIndex++, 0);
    //
    shortcutsCardBottomSpacerIndex = itemIndex;
    shortcutsCard->setLayoutReimpl(shortcutsCardLayout);
}

void SettingsView::Implementation::scrollToTitle(AbstractLabel* title)
{
    const QRect microFocus = title->inputMethodQuery(Qt::ImCursorRectangle).toRect();
    const QRect defaultMicroFocus
        = title->QWidget::inputMethodQuery(Qt::ImCursorRectangle).toRect();
    QRect focusRect = (microFocus != defaultMicroFocus)
        ? QRect(title->mapTo(content->widget(), microFocus.topLeft()), microFocus.size())
        : QRect(title->mapTo(content->widget(), QPoint(0, 0)), title->size());

    focusRect.adjust(-50, -50, 50, 50);

    scrollAnimation.setStartValue(content->verticalScrollBar()->value());
    scrollAnimation.setEndValue(focusRect.top());
    scrollAnimation.start();

    colorAnimation.stop();
    if (colorableTitle != nullptr) {
        colorableTitle->setTextColor(colorAnimation.endValue().value<QColor>());
    }
    colorableTitle = title;
    colorAnimation.setEndValue(colorableTitle->textColor());
    colorableTitle->setTextColor(colorAnimation.startValue().value<QColor>());
    colorAnimation.start();
}

void SettingsView::Implementation::updateTablesGeometry()
{
    auto updateTableGeometry = [](Tree* _table, qreal _firstColumnWidthDelta) {
        if (_table->model() != nullptr) {
            _table->setColumnWidth(0, _table->width() * _firstColumnWidthDelta);
            _table->setFixedHeight((_table->model()->rowCount() + 2)
                                   * qCeil(Ui::DesignSystem::treeOneLineItem().height()));
        }
    };
    updateTableGeometry(shortcutsForScreenplay, 0.25);
}


// ****


SettingsView::SettingsView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setAnimationType(StackWidget::AnimationType::FadeThrough);
    showDefaultPage();

    connect(&d->scrollAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                d->content->verticalScrollBar()->setValue(_value.toInt());
            });
    connect(&d->colorAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                if (d->colorableTitle != nullptr) {
                    d->colorableTitle->setTextColor(_value.value<QColor>());
                }
            });
    //
    // Приложение
    //
    connect(d->useSpellChecker, &CheckBox::checkedChanged, d->spellCheckerLanguage,
            &ComboBox::setEnabled);
    connect(d->saveBackups, &CheckBox::checkedChanged, d->backupsFolderPath,
            &TextField::setEnabled);
    connect(d->backupsFolderPath, &TextField::trailingIconPressed, this, [this] {
        const auto path = QFileDialog::getExistingDirectory(
            this, tr("Choose the folder where backups will be saved"),
            d->backupsFolderPath->text());
        if (!path.isEmpty()) {
            d->backupsFolderPath->setText(path);
        }
    });
    //
    connect(d->changeLanuage, &Button::clicked, this, &SettingsView::applicationLanguagePressed);
    //    connect(d->changeTheme, &Button::clicked, this, &SettingsView::applicationThemePressed);
    for (auto theme : {
             d->lightTheme,
             d->darkAndLightTheme,
             d->darkTheme,
             d->customTheme,
         }) {
        connect(theme, &ThemePreview::themePressed, this, &SettingsView::applicationThemePressed);
    }
    connect(d->scaleFactor, &Slider::valueChanged, this, [this](int _value) {
        emit applicationScaleFactorChanged(0.5 + static_cast<qreal>(_value) / 1000.0);
    });
    connect(d->autoSave, &CheckBox::checkedChanged, this,
            &SettingsView::applicationUseAutoSaveChanged);
    connect(d->saveBackups, &CheckBox::checkedChanged, this,
            &SettingsView::applicationSaveBackupsChanged);
    connect(d->backupsFolderPath, &TextField::textChanged, this,
            [this] { emit applicationBackupsFolderChanged(d->backupsFolderPath->text()); });
    connect(d->showDocumentsPages, &CheckBox::checkedChanged, this,
            &SettingsView::applicationShowDocumentsPagesChanged);
    connect(d->useTypewriterSound, &CheckBox::checkedChanged, this,
            &SettingsView::applicationUseTypewriterSoundChanged);
    connect(d->useSpellChecker, &CheckBox::checkedChanged, this, [this](bool _checked) {
        emit applicationUseSpellCheckerChanged(_checked);
        if (_checked) {
            emit applicationSpellCheckerLanguageChanged(d->spellCheckerLanguage->currentIndex()
                                                            .data(kSpellCheckerLanguageCodeRole)
                                                            .toString());
        }
    });
    connect(d->spellCheckerLanguage, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit applicationSpellCheckerLanguageChanged(
                    _index.data(kSpellCheckerLanguageCodeRole).toString());
            });
    connect(d->highlightCurrentLine, &CheckBox::checkedChanged, this,
            &SettingsView::applicationHighlightCurentLineChanged);
    connect(d->focusCurrentParagraph, &CheckBox::checkedChanged, this,
            &SettingsView::applicationFocusCurrentParagraphChanged);
    connect(d->useTypewriterScrolling, &CheckBox::checkedChanged, this,
            &SettingsView::applicationUseTypewriterScrollingChanged);

    //
    // Компоненты
    //
    // ... Редактор текста
    //
    connect(d->simpleTextEditorDefaultTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit simpleTextEditorDefaultTemplateChanged(
                    _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString());
            });
    //
    // ... навигатор текста
    //
    connect(d->simpleTextNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->simpleTextNavigatorSceneDescriptionLines1, &RadioButton::setEnabled);
    connect(d->simpleTextNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->simpleTextNavigatorSceneDescriptionLines2, &RadioButton::setEnabled);
    connect(d->simpleTextNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->simpleTextNavigatorSceneDescriptionLines3, &RadioButton::setEnabled);
    connect(d->simpleTextNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->simpleTextNavigatorSceneDescriptionLines4, &RadioButton::setEnabled);
    connect(d->simpleTextNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->simpleTextNavigatorSceneDescriptionLines5, &RadioButton::setEnabled);
    //
    auto notifysimpleTextNavigatorShowSceneTextChanged = [this] {
        int sceneTextLines = 1;
        if (d->simpleTextNavigatorSceneDescriptionLines2->isChecked()) {
            sceneTextLines = 2;
        } else if (d->simpleTextNavigatorSceneDescriptionLines3->isChecked()) {
            sceneTextLines = 3;
        } else if (d->simpleTextNavigatorSceneDescriptionLines4->isChecked()) {
            sceneTextLines = 4;
        } else if (d->simpleTextNavigatorSceneDescriptionLines5->isChecked()) {
            sceneTextLines = 5;
        }
        emit simpleTextNavigatorShowSceneTextChanged(
            d->simpleTextNavigatorShowSceneText->isChecked(), sceneTextLines);
    };
    connect(d->simpleTextNavigatorShowSceneText, &CheckBox::checkedChanged, this,
            notifysimpleTextNavigatorShowSceneTextChanged);
    connect(d->simpleTextNavigatorSceneDescriptionLines1, &RadioButton::checkedChanged, this,
            notifysimpleTextNavigatorShowSceneTextChanged);
    connect(d->simpleTextNavigatorSceneDescriptionLines2, &RadioButton::checkedChanged, this,
            notifysimpleTextNavigatorShowSceneTextChanged);
    connect(d->simpleTextNavigatorSceneDescriptionLines3, &RadioButton::checkedChanged, this,
            notifysimpleTextNavigatorShowSceneTextChanged);
    connect(d->simpleTextNavigatorSceneDescriptionLines4, &RadioButton::checkedChanged, this,
            notifysimpleTextNavigatorShowSceneTextChanged);
    connect(d->simpleTextNavigatorSceneDescriptionLines5, &RadioButton::checkedChanged, this,
            notifysimpleTextNavigatorShowSceneTextChanged);
    //
    // ... Редактор сценария
    //
    connect(d->screenplayEditorDefaultTemplateOptions, &IconButton::clicked, this, [this] {
        QVector<QAction*> actions;
        const auto templateIndex = d->screenplayEditorDefaultTemplate->currentIndex();
        const auto templateId
            = templateIndex.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString();
        const auto isDefaultTemplate
            = BusinessLayer::TemplatesFacade::screenplayTemplate(templateId).isDefault();
        if (!isDefaultTemplate) {
            auto editAction = new QAction(tr("Edit"), d->contextMenu);
            connect(editAction, &QAction::triggered, this, [this, templateId] {
                emit editCurrentScreenplayEditorTemplateRequested(templateId);
            });
            actions.append(editAction);
        }
        //
        auto duplicateAction = new QAction(tr("Duplicate"), d->contextMenu);
        connect(duplicateAction, &QAction::triggered, this, [this, templateId] {
            emit duplicateCurrentScreenplayEditorTemplateRequested(templateId);
        });
        actions.append(duplicateAction);
        //
        if (!isDefaultTemplate) {
            auto removeAction = new QAction(tr("Remove"), d->contextMenu);
            connect(removeAction, &QAction::triggered, this, [this, templateId] {
                emit removeCurrentScreenplayEditorTemplateRequested(templateId);
            });
            actions.append(removeAction);
        }
        //        auto exportAction = new QAction(tr("Export"), d->contextMenu);
        //        auto importAction = new QAction(tr("Import"), d->contextMenu);
        //        importAction->setSeparator(true);
        d->contextMenu->setActions(actions);
        d->contextMenu->showContextMenu(QCursor::pos());
    });
    connect(d->screenplayEditorShowSceneNumber, &CheckBox::checkedChanged,
            d->screenplayEditorShowSceneNumberOnLeft, &CheckBox::setEnabled);
    connect(d->screenplayEditorShowSceneNumber, &CheckBox::checkedChanged,
            d->screenplayEditorShowSceneNumberOnRight, &CheckBox::setEnabled);
    auto screenplayEditorCorrectShownSceneNumber = [this] {
        if (!d->screenplayEditorShowSceneNumberOnLeft->isChecked()
            && !d->screenplayEditorShowSceneNumberOnRight->isChecked()) {
            d->screenplayEditorShowSceneNumberOnLeft->setChecked(true);
        }
    };
    connect(d->screenplayEditorShowSceneNumberOnLeft, &CheckBox::checkedChanged, this,
            screenplayEditorCorrectShownSceneNumber);
    connect(d->screenplayEditorShowSceneNumberOnRight, &CheckBox::checkedChanged, this,
            screenplayEditorCorrectShownSceneNumber);
    //
    connect(d->screenplayEditorDefaultTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit screenplayEditorDefaultTemplateChanged(
                    _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString());
            });
    auto notifyScreenplayEditorShowSceneNumbersChanged = [this] {
        emit screenplayEditorShowSceneNumberChanged(
            d->screenplayEditorShowSceneNumber->isChecked(),
            d->screenplayEditorShowSceneNumberOnLeft->isChecked(),
            d->screenplayEditorShowSceneNumberOnRight->isChecked());
    };
    connect(d->screenplayEditorShowSceneNumber, &CheckBox::checkedChanged, this,
            notifyScreenplayEditorShowSceneNumbersChanged);
    connect(d->screenplayEditorShowSceneNumberOnLeft, &CheckBox::checkedChanged, this,
            notifyScreenplayEditorShowSceneNumbersChanged);
    connect(d->screenplayEditorShowSceneNumberOnRight, &CheckBox::checkedChanged, this,
            notifyScreenplayEditorShowSceneNumbersChanged);
    connect(d->screenplayEditorShowDialogueNumber, &CheckBox::checkedChanged, this,
            &SettingsView::screenplayEditorShowDialogueNumberChanged);
    connect(d->screenplayEditorContinueDialogue, &CheckBox::checkedChanged, this,
            &SettingsView::screenplayEditorContinueDialogueChanged);
    connect(d->screenplayEditorUseCharactersFromText, &CheckBox::checkedChanged, this,
            &SettingsView::screenplayEditorUseCharactersFromTextChanged);
    //
    // ... навигатор сценария
    //
    connect(d->screenplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->screenplayNavigatorSceneDescriptionLines1, &RadioButton::setEnabled);
    connect(d->screenplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->screenplayNavigatorSceneDescriptionLines2, &RadioButton::setEnabled);
    connect(d->screenplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->screenplayNavigatorSceneDescriptionLines3, &RadioButton::setEnabled);
    connect(d->screenplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->screenplayNavigatorSceneDescriptionLines4, &RadioButton::setEnabled);
    connect(d->screenplayNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->screenplayNavigatorSceneDescriptionLines5, &RadioButton::setEnabled);
    //
    connect(d->screenplayNavigatorShowSceneNumber, &CheckBox::checkedChanged, this,
            &SettingsView::screenplayNavigatorShowSceneNumberChanged);
    auto notifyScreenplayNavigatorShowSceneTextChanged = [this] {
        int sceneTextLines = 1;
        if (d->screenplayNavigatorSceneDescriptionLines2->isChecked()) {
            sceneTextLines = 2;
        } else if (d->screenplayNavigatorSceneDescriptionLines3->isChecked()) {
            sceneTextLines = 3;
        } else if (d->screenplayNavigatorSceneDescriptionLines4->isChecked()) {
            sceneTextLines = 4;
        } else if (d->screenplayNavigatorSceneDescriptionLines5->isChecked()) {
            sceneTextLines = 5;
        }
        emit screenplayNavigatorShowSceneTextChanged(
            d->screenplayNavigatorShowSceneText->isChecked(), sceneTextLines);
    };
    connect(d->screenplayNavigatorShowSceneText, &CheckBox::checkedChanged, this,
            notifyScreenplayNavigatorShowSceneTextChanged);
    connect(d->screenplayNavigatorSceneDescriptionLines1, &RadioButton::checkedChanged, this,
            notifyScreenplayNavigatorShowSceneTextChanged);
    connect(d->screenplayNavigatorSceneDescriptionLines2, &RadioButton::checkedChanged, this,
            notifyScreenplayNavigatorShowSceneTextChanged);
    connect(d->screenplayNavigatorSceneDescriptionLines3, &RadioButton::checkedChanged, this,
            notifyScreenplayNavigatorShowSceneTextChanged);
    connect(d->screenplayNavigatorSceneDescriptionLines4, &RadioButton::checkedChanged, this,
            notifyScreenplayNavigatorShowSceneTextChanged);
    connect(d->screenplayNavigatorSceneDescriptionLines5, &RadioButton::checkedChanged, this,
            notifyScreenplayNavigatorShowSceneTextChanged);
    //
    // ... хронометраж
    //
    connect(d->screenplayDurationByPage, &RadioButton::checkedChanged,
            d->screenplayDurationByPagePage, &TextField::setEnabled);
    connect(d->screenplayDurationByPage, &RadioButton::checkedChanged,
            d->screenplayDurationByPageDuration, &TextField::setEnabled);
    connect(d->screenplayDurationByCharacters, &RadioButton::checkedChanged,
            d->screenplayDurationByCharactersCharacters, &TextField::setEnabled);
    connect(d->screenplayDurationByCharacters, &RadioButton::checkedChanged,
            d->screenplayDurationByCharactersIncludingSpaces, &RadioButton::setEnabled);
    connect(d->screenplayDurationByCharacters, &RadioButton::checkedChanged,
            d->screenplayDurationByCharactersDuration, &TextField::setEnabled);
    //
    auto notifyScreenplayDurationTypeChanged = [this] {
        using namespace BusinessLayer;
        if (d->screenplayDurationByPage->isChecked()) {
            emit screenplayDurationTypeChanged(static_cast<int>(ChronometerType::Page));
        } else if (d->screenplayDurationByCharacters->isChecked()) {
            emit screenplayDurationTypeChanged(static_cast<int>(ChronometerType::Characters));
        }
    };
    connect(d->screenplayDurationByPage, &RadioButton::checkedChanged, this,
            notifyScreenplayDurationTypeChanged);
    connect(d->screenplayDurationByCharacters, &RadioButton::checkedChanged, this,
            notifyScreenplayDurationTypeChanged);
    connect(d->screenplayDurationByPageDuration, &TextField::textChanged, this, [this] {
        emit screenplayDurationByPageDurationChanged(
            d->screenplayDurationByPageDuration->text().toInt());
    });
    connect(d->screenplayDurationByCharactersCharacters, &TextField::textChanged, this, [this] {
        emit screenplayDurationByCharactersCharactersChanged(
            d->screenplayDurationByCharactersCharacters->text().toInt());
    });
    connect(d->screenplayDurationByCharactersIncludingSpaces, &CheckBox::checkedChanged, this,
            &SettingsView::screenplayDurationByCharactersIncludeSpacesChanged);
    connect(d->screenplayDurationByCharactersDuration, &TextField::textChanged, this, [this] {
        emit screenplayDurationByCharactersDurationChanged(
            d->screenplayDurationByCharactersDuration->text().toInt());
    });
    //
    // ... Редактор комикса
    //
    connect(d->comicBookEditorDefaultTemplate, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit comicBookEditorDefaultTemplateChanged(
                    _index.data(BusinessLayer::TemplatesFacade::kTemplateIdRole).toString());
            });
    //
    // ... навигатор текста
    //
    connect(d->comicBookNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->comicBookNavigatorSceneDescriptionLines1, &RadioButton::setEnabled);
    connect(d->comicBookNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->comicBookNavigatorSceneDescriptionLines2, &RadioButton::setEnabled);
    connect(d->comicBookNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->comicBookNavigatorSceneDescriptionLines3, &RadioButton::setEnabled);
    connect(d->comicBookNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->comicBookNavigatorSceneDescriptionLines4, &RadioButton::setEnabled);
    connect(d->comicBookNavigatorShowSceneText, &CheckBox::checkedChanged,
            d->comicBookNavigatorSceneDescriptionLines5, &RadioButton::setEnabled);
    //
    auto notifycomicBookNavigatorShowSceneTextChanged = [this] {
        int sceneTextLines = 1;
        if (d->comicBookNavigatorSceneDescriptionLines2->isChecked()) {
            sceneTextLines = 2;
        } else if (d->comicBookNavigatorSceneDescriptionLines3->isChecked()) {
            sceneTextLines = 3;
        } else if (d->comicBookNavigatorSceneDescriptionLines4->isChecked()) {
            sceneTextLines = 4;
        } else if (d->comicBookNavigatorSceneDescriptionLines5->isChecked()) {
            sceneTextLines = 5;
        }
        emit comicBookNavigatorShowSceneTextChanged(d->comicBookNavigatorShowSceneText->isChecked(),
                                                    sceneTextLines);
    };
    connect(d->comicBookNavigatorShowSceneText, &CheckBox::checkedChanged, this,
            notifycomicBookNavigatorShowSceneTextChanged);
    connect(d->comicBookNavigatorSceneDescriptionLines1, &RadioButton::checkedChanged, this,
            notifycomicBookNavigatorShowSceneTextChanged);
    connect(d->comicBookNavigatorSceneDescriptionLines2, &RadioButton::checkedChanged, this,
            notifycomicBookNavigatorShowSceneTextChanged);
    connect(d->comicBookNavigatorSceneDescriptionLines3, &RadioButton::checkedChanged, this,
            notifycomicBookNavigatorShowSceneTextChanged);
    connect(d->comicBookNavigatorSceneDescriptionLines4, &RadioButton::checkedChanged, this,
            notifycomicBookNavigatorShowSceneTextChanged);
    connect(d->comicBookNavigatorSceneDescriptionLines5, &RadioButton::checkedChanged, this,
            notifycomicBookNavigatorShowSceneTextChanged);

    //
    // Соединения шорткатов настраиваются в момент установки моделей с данными о них в представление
    //


    designSystemChangeEvent(nullptr);
}

void SettingsView::showDefaultPage()
{
    setCurrentWidget(d->content);
}

void SettingsView::showApplication()
{
    d->scrollToTitle(d->applicationTitle);
}

void SettingsView::showApplicationUserInterface()
{
    d->scrollToTitle(d->applicationUserInterfaceTitle);
}

void SettingsView::showApplicationSaveAndBackups()
{
    d->scrollToTitle(d->applicationSaveAndBackupTitle);
}

void SettingsView::showApplicationTextEditing()
{
    d->scrollToTitle(d->applicationTextEditingTitle);
}

void SettingsView::showComponents()
{
    d->scrollToTitle(d->componentsTitle);
}

void SettingsView::showComponentsSimpleText()
{
    d->scrollToTitle(d->simpleTextTitle);
}

void SettingsView::showComponentsScreenplay()
{
    d->scrollToTitle(d->screenplayTitle);
}

void SettingsView::showComponentsComicBook()
{
    d->scrollToTitle(d->comicBookTitle);
}

void SettingsView::showShortcuts()
{
    d->scrollToTitle(d->shortcutsTitle);
}

void SettingsView::setApplicationLanguage(int _language)
{
    auto languageString = [_language]() -> QString {
        switch (_language) {
        case QLocale::Azerbaijani: {
            return "Azərbaycan";
        }
        case QLocale::Belarusian: {
            return "Беларуский";
        }
        case QLocale::Catalan: {
            return "Català";
        }
        case QLocale::Danish: {
            return "Dansk";
        }
        case QLocale::English: {
            return "English";
        }
        case QLocale::Esperanto: {
            return "Esperanto";
        }
        case QLocale::French: {
            return "Français";
        }
        case QLocale::Galician: {
            return "Galego";
        }
        case QLocale::German: {
            return "Deutsch";
        }
        case QLocale::Hebrew: {
            return "עִבְרִית";
        }
        case QLocale::Hindi: {
            return "हिन्दी";
        }
        case QLocale::Hungarian: {
            return "Magyar";
        }
        case QLocale::Indonesian: {
            return "Indonesian";
        }
        case QLocale::Italian: {
            return "Italiano";
        }
        case QLocale::Persian: {
            return "فارسی";
        }
        case QLocale::Polish: {
            return "Polski";
        }
        case QLocale::Portuguese: {
            return "Português Brasileiro";
        }
        case QLocale::Romanian: {
            return "Română";
        }
        case QLocale::Russian: {
            return "Русский";
        }
        case QLocale::Slovenian: {
            return "Slovenski";
        }
        case QLocale::Spanish: {
            return "Español";
        }
        case QLocale::Turkish: {
            return "Türkçe";
        }
        case QLocale::Ukrainian: {
            return "Українська";
        }
        default: {
            return QLocale::languageToString(static_cast<QLocale::Language>(_language));
        }
        }
    };
    d->changeLanuage->setText(languageString());
}

void SettingsView::setApplicationScaleFactor(qreal _scaleFactor)
{
    d->scaleFactor->setValue((_scaleFactor - 0.5) * 1000.0);
}

void SettingsView::setApplicationUseAutoSave(bool _use)
{
    d->autoSave->setChecked(_use);
}

void SettingsView::setApplicationSaveBackups(bool _save)
{
    d->saveBackups->setChecked(_save);
}

void SettingsView::setApplicationBackupsFolder(const QString& _path)
{
    d->backupsFolderPath->setText(_path);
}

void SettingsView::setApplicationShowDocumentsPages(bool _show)
{
    d->showDocumentsPages->setChecked(_show);
}

void SettingsView::setApplicationUseTypewriterSound(bool _use)
{
    d->useTypewriterSound->setChecked(_use);
}

void SettingsView::setApplicationUseSpellChecker(bool _use)
{
    d->useSpellChecker->setChecked(_use);
}

void SettingsView::setApplicationSpellCheckerLanguage(const QString& _languageCode)
{
    for (int row = 0; row < d->spellCheckerLanguagesModel->rowCount(); ++row) {
        auto item = d->spellCheckerLanguagesModel->item(row);
        if (item->data(kSpellCheckerLanguageCodeRole).toString() != _languageCode) {
            continue;
        }

        d->spellCheckerLanguage->setCurrentIndex(item->index());
        break;
    }
}

void SettingsView::setApplicationHighlightCurrentLine(bool _highlight)
{
    d->highlightCurrentLine->setChecked(_highlight);
}

void SettingsView::setApplicationFocusCurrentParagraph(bool _focus)
{
    d->focusCurrentParagraph->setChecked(_focus);
}

void SettingsView::setApplicationUseTypewriterScrolling(bool _use)
{
    d->useTypewriterScrolling->setChecked(_use);
}

void SettingsView::setSimpleTextEditorDefaultTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::simpleTextTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::simpleTextTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        d->simpleTextEditorDefaultTemplate->setCurrentIndex(item->index());
        break;
    }
}

void SettingsView::setSimpleTextNavigatorShowSceneText(bool _show, int _lines)
{
    d->simpleTextNavigatorShowSceneText->setChecked(_show);
    if (_show) {
        const QHash<int, RadioButton*> buttons
            = { { 1, d->simpleTextNavigatorSceneDescriptionLines1 },
                { 2, d->simpleTextNavigatorSceneDescriptionLines2 },
                { 3, d->simpleTextNavigatorSceneDescriptionLines3 },
                { 4, d->simpleTextNavigatorSceneDescriptionLines4 },
                { 5, d->simpleTextNavigatorSceneDescriptionLines5 } };
        buttons[_lines]->setChecked(true);
    }
}

void SettingsView::setScreenplayEditorDefaultTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::screenplayTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::screenplayTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        d->screenplayEditorDefaultTemplate->setCurrentIndex(item->index());
        break;
    }
}

void SettingsView::setScreenplayEditorShowSceneNumber(bool _show, bool _atLeft, bool _atRight)
{
    QSignalBlocker blocker(d->screenplayEditorShowSceneNumberOnLeft);

    d->screenplayEditorShowSceneNumber->setChecked(_show);
    d->screenplayEditorShowSceneNumberOnLeft->setChecked(_atLeft);
    d->screenplayEditorShowSceneNumberOnRight->setChecked(_atRight);
}

void SettingsView::setScreenplayEditorShowDialogueNumber(bool _show)
{
    d->screenplayEditorShowDialogueNumber->setChecked(_show);
}

void SettingsView::setScreenplayEditorContinueDialogue(bool _continue)
{
    d->screenplayEditorContinueDialogue->setChecked(_continue);
}

void SettingsView::setScreenplayEditorUseCharactersFromText(bool _use)
{
    d->screenplayEditorUseCharactersFromText->setChecked(_use);
}

void SettingsView::setScreenplayNavigatorShowSceneNumber(bool _show)
{
    d->screenplayNavigatorShowSceneNumber->setChecked(_show);
}

void SettingsView::setScreenplayNavigatorShowSceneText(bool _show, int _lines)
{
    d->screenplayNavigatorShowSceneText->setChecked(_show);
    if (_show) {
        const QHash<int, RadioButton*> buttons
            = { { 1, d->screenplayNavigatorSceneDescriptionLines1 },
                { 2, d->screenplayNavigatorSceneDescriptionLines2 },
                { 3, d->screenplayNavigatorSceneDescriptionLines3 },
                { 4, d->screenplayNavigatorSceneDescriptionLines4 },
                { 5, d->screenplayNavigatorSceneDescriptionLines5 } };
        buttons[_lines]->setChecked(true);
    }
}

void SettingsView::setScreenplayDurationType(int _type)
{
    using namespace BusinessLayer;
    switch (static_cast<ChronometerType>(_type)) {
    case ChronometerType::Page: {
        d->screenplayDurationByPage->setChecked(true);
        break;
    }

    case ChronometerType::Characters: {
        d->screenplayDurationByCharacters->setChecked(true);
        break;
    }
    }
}

void SettingsView::setScreenplayDurationByPageDuration(int _duration)
{
    d->screenplayDurationByPageDuration->setText(QString::number(_duration));
}

void SettingsView::setScreenplayDurationByCharactersCharacters(int _characters)
{
    d->screenplayDurationByCharactersCharacters->setText(QString::number(_characters));
}

void SettingsView::setScreenplayDurationByCharactersIncludeSpaces(bool _include)
{
    d->screenplayDurationByCharactersIncludingSpaces->setChecked(_include);
}

void SettingsView::setScreenplayDurationByCharactersDuration(int _duration)
{
    d->screenplayDurationByCharactersDuration->setText(QString::number(_duration));
}

void SettingsView::setComicBookEditorDefaultTemplate(const QString& _templateId)
{
    using namespace BusinessLayer;
    for (int row = 0; row < TemplatesFacade::comicBookTemplates()->rowCount(); ++row) {
        auto item = TemplatesFacade::comicBookTemplates()->item(row);
        if (item->data(TemplatesFacade::kTemplateIdRole).toString() != _templateId) {
            continue;
        }

        d->comicBookEditorDefaultTemplate->setCurrentIndex(item->index());
        break;
    }
}

void SettingsView::setComicBookNavigatorShowSceneText(bool _show, int _lines)
{
    d->comicBookNavigatorShowSceneText->setChecked(_show);
    if (_show) {
        const QHash<int, RadioButton*> buttons
            = { { 1, d->comicBookNavigatorSceneDescriptionLines1 },
                { 2, d->comicBookNavigatorSceneDescriptionLines2 },
                { 3, d->comicBookNavigatorSceneDescriptionLines3 },
                { 4, d->comicBookNavigatorSceneDescriptionLines4 },
                { 5, d->comicBookNavigatorSceneDescriptionLines5 } };
        buttons[_lines]->setChecked(true);
    }
}

void SettingsView::setShortcutsForScreenplayModel(HierarchicalModel* _model)
{
    if (d->shortcutsForScreenplayModel) {
        d->shortcutsForScreenplayModel->disconnect(this);
        d->shortcutsForScreenplayModel->deleteLater();
    }

    d->shortcutsForScreenplayModel = _model;

    d->shortcutsForScreenplay->setModel(d->shortcutsForScreenplayModel);
    d->shortcutsForScreenplay->setItemDelegateForColumn(
        1, new KeySequenceDelegate(d->shortcutsForScreenplay));
    d->shortcutsForScreenplay->setItemDelegateForColumn(2, d->screenplayParagraphAddTypeDelegate);
    d->shortcutsForScreenplay->setItemDelegateForColumn(3, d->screenplayParagraphAddTypeDelegate);
    d->shortcutsForScreenplay->setItemDelegateForColumn(4,
                                                        d->screenplayParagraphChangeTypeDelegate);
    d->shortcutsForScreenplay->setItemDelegateForColumn(5,
                                                        d->screenplayParagraphChangeTypeDelegate);
    d->updateTablesGeometry();

    connect(
        d->shortcutsForScreenplayModel, &HierarchicalModel::dataChanged, this,
        [this](const QModelIndex& _index) {
            if (!_index.isValid()) {
                return;
            }

            const auto blockType
                = _index.sibling(_index.row(), TextEditorShortcuts::Type).data().toString();
            const auto shortcut
                = _index.sibling(_index.row(), TextEditorShortcuts::Shortcut).data().toString();
            const auto jumpByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByTab).data().toString();
            const auto jumpByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::JumpByEnter).data().toString();
            const auto changeByTab
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByTab).data().toString();
            const auto changeByEnter
                = _index.sibling(_index.row(), TextEditorShortcuts::ChangeByEnter)
                      .data()
                      .toString();

            emit shortcutsForScreenplayEditorChanged(blockType, shortcut, jumpByTab, jumpByEnter,
                                                     changeByTab, changeByEnter);
        });
}

void SettingsView::resizeEvent(QResizeEvent* _event)
{
    StackWidget::resizeEvent(_event);

    d->updateTablesGeometry();
}

void SettingsView::updateTranslations()
{
    d->applicationTitle->setText(tr("Application settings"));
    d->language->setText(tr("Language"));
    d->scaleFactorTitle->setText(tr("Size of the user interface elements:"));
    d->scaleFactorSmallInfo->setText(tr("small"));
    d->scaleFactorBigInfo->setText(tr("big"));
    d->applicationSaveAndBackupTitle->setText(tr("Save changes & backups"));
    d->autoSave->setText(tr("Automatically save changes as soon as possible"));
    d->autoSave->setToolTip(
        tr("Autosave works very accurately.\n"
           "It saves the project every 3 seconds if you do not use your mouse or keyboard.\n"
           "If you work with no interruptions it saves the project every 3 minutes."));
    d->saveBackups->setText(tr("Save backups"));
    d->backupsFolderPath->setLabel(tr("Backups folder path"));
    d->applicationTextEditingTitle->setText(tr("Text editing"));
    d->showDocumentsPages->setText(tr("Show documents pages"));
    d->useTypewriterSound->setText(tr("Use typewriter sound for keys pressing"));
    d->useSpellChecker->setText(tr("Spell check"));
    d->spellCheckerLanguage->setLabel(tr("Spelling dictionary"));
    {
        int index = 0;
        for (const auto& language : { tr("Afrikaans"),
                                      tr("Aragonese"),
                                      tr("Arabic"),
                                      tr("Azerbaijani"),
                                      tr("Belarusian"),
                                      tr("Bulgarian"),
                                      tr("Bengali"),
                                      tr("Tibetan"),
                                      tr("Breton"),
                                      tr("Bosnian"),
                                      tr("Catalan (Valencian)"),
                                      tr("Catalan"),
                                      tr("Czech"),
                                      tr("Welsh"),
                                      tr("Danish"),
                                      tr("German (Austria)"),
                                      tr("German (Switzerland)"),
                                      tr("German"),
                                      tr("Greek (Polytonic)"),
                                      tr("Greek"),
                                      tr("English (Australia)"),
                                      tr("English (Canada)"),
                                      tr("English (United Kingdom)"),
                                      tr("English (New Zealand)"),
                                      tr("English (South Africa)"),
                                      tr("English (United States)"),
                                      tr("Esperanto"),
                                      tr("Spanish (Argentina)"),
                                      tr("Spanish (Bolivia)"),
                                      tr("Spanish (Chile)"),
                                      tr("Spanish (Colombia)"),
                                      tr("Spanish (Costa Rica)"),
                                      tr("Spanish (Cuba)"),
                                      tr("Spanish (Dominican Republic)"),
                                      tr("Spanish (Ecuador)"),
                                      tr("Spanish (Guatemala)"),
                                      tr("Spanish (Honduras)"),
                                      tr("Spanish (Mexico)"),
                                      tr("Spanish (Nicaragua)"),
                                      tr("Spanish (Panama)"),
                                      tr("Spanish (Peru)"),
                                      tr("Spanish (Philippines)"),
                                      tr("Spanish (Puerto Rico)"),
                                      tr("Spanish (Paraguay)"),
                                      tr("Spanish (El Salvador)"),
                                      tr("Spanish (United States)"),
                                      tr("Spanish (Uruguay)"),
                                      tr("Spanish (Venezuela)"),
                                      tr("Spanish"),
                                      tr("Estonian"),
                                      tr("Basque"),
                                      tr("Persian"),
                                      tr("Faroese"),
                                      tr("French"),
                                      tr("Friulian"),
                                      tr("Western Frisian"),
                                      tr("Irish"),
                                      tr("Gaelic"),
                                      tr("Galician"),
                                      tr("Gujarati"),
                                      tr("Guarani"),
                                      tr("Hebrew"),
                                      tr("Hindi"),
                                      tr("Croatian"),
                                      tr("Hungarian"),
                                      tr("Armenian"),
                                      tr("Armenian (Western)"),
                                      tr("Interlingua"),
                                      tr("Indonesian"),
                                      tr("Icelandic"),
                                      tr("Italian"),
                                      tr("Georgian"),
                                      tr("Kazakh"),
                                      tr("Kurdish"),
                                      tr("Korean"),
                                      tr("Latin"),
                                      tr("Luxembourgish"),
                                      tr("Lao"),
                                      tr("Lithuanian"),
                                      tr("Latgalian"),
                                      tr("Latvian"),
                                      tr("Macedonian"),
                                      tr("Mongolian"),
                                      tr("Maltese"),
                                      tr("Norwegian"),
                                      tr("Low German"),
                                      tr("Nepali"),
                                      tr("Dutch"),
                                      tr("Norwegian"),
                                      tr("Occitan"),
                                      tr("Polish"),
                                      tr("Portuguese (Brazilian)"),
                                      tr("Portuguese"),
                                      tr("Quechua"),
                                      tr("Romanian"),
                                      tr("Russian (with Yo)"),
                                      tr("Russian"),
                                      tr("Kinyarwanda"),
                                      tr("Sinhala"),
                                      tr("Slovak"),
                                      tr("Slovenian"),
                                      tr("Albanian"),
                                      tr("Serbian (Latin)"),
                                      tr("Serbian"),
                                      tr("Swedish (Finland)"),
                                      tr("Swedish"),
                                      tr("Swahili"),
                                      tr("Tamil"),
                                      tr("Telugu"),
                                      tr("Thai"),
                                      tr("Turkmen"),
                                      tr("Klingon (Latin)"),
                                      tr("Klingon"),
                                      tr("Turkish"),
                                      tr("Ukrainian"),
                                      tr("Vietnamese") }) {
            d->spellCheckerLanguagesModel->item(index++)->setText(language);
        }
        d->spellCheckerLanguagesModel->sort(0);
    }
    d->applicationUserInterfaceTitle->setText(tr("User interface"));
    d->spellCheckerUserDictionary->setToolTip(tr("Manage user dictionary"));
    d->highlightCurrentLine->setText(tr("Highlight current line"));
    d->focusCurrentParagraph->setText(tr("Focus current paragraph"));
    d->useTypewriterScrolling->setText(
        tr("Use typewriter scrolling (keeps line with the cursor on the screen center)"));

    d->componentsTitle->setText(tr("Components"));
    //
    BusinessLayer::TemplatesFacade::updateTranslations();
    //
    d->simpleTextTitle->setText(tr("Simple text module"));
    d->simpleTextEditorTitle->setText(tr("Text editor"));
    d->simpleTextEditorDefaultTemplate->setLabel(tr("Default template"));
    d->simpleTextEditorDefaultTemplateOptions->setToolTip(
        tr("Available actions for the selected template"));
    d->simpleTextNavigatorTitle->setText(tr("Navigator"));
    d->simpleTextNavigatorShowSceneText->setText(tr("Show chapter text, lines"));
    d->simpleTextNavigatorSceneDescriptionLines1->setText("1");
    d->simpleTextNavigatorSceneDescriptionLines2->setText("2");
    d->simpleTextNavigatorSceneDescriptionLines3->setText("3");
    d->simpleTextNavigatorSceneDescriptionLines4->setText("4");
    d->simpleTextNavigatorSceneDescriptionLines5->setText("5");
    //
    d->screenplayTitle->setText(tr("Screenplay module"));
    d->screenplayEditorTitle->setText(tr("Text editor"));
    d->screenplayEditorDefaultTemplate->setLabel(tr("Default template"));
    d->screenplayEditorDefaultTemplateOptions->setToolTip(
        tr("Available actions for the selected template"));
    d->screenplayEditorShowSceneNumber->setText(tr("Show scene number"));
    d->screenplayEditorShowSceneNumberOnLeft->setText(tr("on the left"));
    d->screenplayEditorShowSceneNumberOnRight->setText(tr("on the right"));
    d->screenplayEditorShowDialogueNumber->setText(tr("Show dialogue number"));
    d->screenplayEditorContinueDialogue->setText(
        tr("Automatically continue same speaker's dialogue"));
    d->screenplayEditorUseCharactersFromText->setText(
        tr("Include to the hints only valuable characters and characters from current screenplay"));
    d->screenplayNavigatorTitle->setText(tr("Navigator"));
    d->screenplayNavigatorShowSceneNumber->setText(tr("Show scene number"));
    d->screenplayNavigatorShowSceneText->setText(tr("Show scene text, lines"));
    d->screenplayNavigatorSceneDescriptionLines1->setText("1");
    d->screenplayNavigatorSceneDescriptionLines2->setText("2");
    d->screenplayNavigatorSceneDescriptionLines3->setText("3");
    d->screenplayNavigatorSceneDescriptionLines4->setText("4");
    d->screenplayNavigatorSceneDescriptionLines5->setText("5");
    d->screenplayDurationTitle->setText(tr("Duration"));
    d->screenplayDurationByPage->setText(tr("Calculate duration based on the count of pages"));
    d->screenplayDurationByPagePage->setLabel(tr("at the rate of"));
    d->screenplayDurationByPagePage->setSuffix(tr("pages"));
    d->screenplayDurationByPageDuration->setLabel(tr("has duration"));
    d->screenplayDurationByPageDuration->setSuffix(tr("seconds"));
    d->screenplayDurationByCharacters->setText(
        tr("Calculate duration based on the count of letters"));
    d->screenplayDurationByCharactersCharacters->setLabel(tr("at the rate of"));
    d->screenplayDurationByCharactersCharacters->setSuffix(tr("letters"));
    d->screenplayDurationByCharactersIncludingSpaces->setText(tr("including spaces"));
    d->screenplayDurationByCharactersDuration->setLabel(tr("has duration"));
    d->screenplayDurationByCharactersDuration->setSuffix(tr("seconds"));
    //
    d->comicBookTitle->setText(tr("Comic book module"));
    d->comicBookEditorTitle->setText(tr("Text editor"));
    d->comicBookEditorDefaultTemplate->setLabel(tr("Default template"));
    d->comicBookEditorDefaultTemplateOptions->setToolTip(
        tr("Available actions for the selected template"));
    d->comicBookNavigatorTitle->setText(tr("Navigator"));
    d->comicBookNavigatorShowSceneText->setText(tr("Show panel text, lines"));
    d->comicBookNavigatorSceneDescriptionLines1->setText("1");
    d->comicBookNavigatorSceneDescriptionLines2->setText("2");
    d->comicBookNavigatorSceneDescriptionLines3->setText("3");
    d->comicBookNavigatorSceneDescriptionLines4->setText("4");
    d->comicBookNavigatorSceneDescriptionLines5->setText("5");

    d->shortcutsTitle->setText(tr("Shortcuts"));
    d->shortcutsForScreenplayTitle->setText(tr("Screenplay editor"));
    {
        auto model = d->shortcutsForScreenplayModel->headerModel();
        model->setData(model->index(0, 0), tr("Block name"), Qt::DisplayRole);
        model->setData(model->index(0, 1), tr("Shortcut"), Qt::DisplayRole);
        model->setData(model->index(0, 2), tr("If you press in paragraphs end"), Qt::DisplayRole);
        model->setData(model->index(0, 3), tr("If you press in empty paragraphs"), Qt::DisplayRole);
    }
    buildScreenplayParagraphTypesModel(d->shortcutsForScreenplay, d->screenplayParagraphTypesModel);
    d->screenplayParagraphAddTypeDelegate->setLabel(tr("Add paragraph"));
    d->screenplayParagraphChangeTypeDelegate->setLabel(tr("Change to"));
}

SettingsView::~SettingsView() = default;

void SettingsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().surface());
    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());
    d->contextMenu->setBackgroundColor(Ui::DesignSystem::color().background());
    d->contextMenu->setTextColor(Ui::DesignSystem::color().onBackground());

    d->colorAnimation.setStartValue(Ui::DesignSystem::color().secondary());

    for (auto card : { d->applicationCard, d->simpleTextCard, d->screenplayCard, d->comicBookCard,
                       d->shortcutsCard }) {
        card->setBackgroundColor(DesignSystem::color().background());
    }

    auto titleColor = DesignSystem::color().onBackground();
    titleColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    auto titleMargins = Ui::DesignSystem::label().margins().toMargins();
    titleMargins.setBottom(Ui::DesignSystem::layout().px12());
    for (auto cardTitle : std::vector<Widget*>{
             d->applicationTitle,
             d->applicationUserInterfaceTitle,
             d->applicationSaveAndBackupTitle,
             d->applicationTextEditingTitle,
             d->simpleTextTitle,
             d->simpleTextEditorTitle,
             d->simpleTextNavigatorTitle,
             d->screenplayTitle,
             d->screenplayEditorTitle,
             d->screenplayNavigatorTitle,
             d->screenplayDurationTitle,
             d->comicBookTitle,
             d->comicBookEditorTitle,
             d->comicBookNavigatorTitle,
             d->shortcutsTitle,
             d->shortcutsForScreenplayTitle,
         }) {
        cardTitle->setBackgroundColor(DesignSystem::color().background());
        cardTitle->setTextColor(titleColor);
        cardTitle->setContentsMargins(titleMargins);
    }
    titleMargins.setBottom(0);
    for (auto title : std::vector<Widget*>{ d->componentsTitle }) {
        title->setBackgroundColor(DesignSystem::color().surface());
        title->setTextColor(titleColor);
        title->setContentsMargins(titleMargins);
    }

    auto labelMargins = Ui::DesignSystem::label().margins().toMargins();
    labelMargins.setTop(static_cast<int>(Ui::DesignSystem::button().shadowMargins().top()));
    labelMargins.setBottom(static_cast<int>(Ui::DesignSystem::button().shadowMargins().bottom()));
    for (auto label : std::vector<Widget*>{
             d->language,
             d->scaleFactorTitle,
             d->scaleFactorSmallInfo,
             d->scaleFactorBigInfo,
         }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(DesignSystem::color().onBackground());
        label->setContentsMargins(labelMargins);
    }

    auto iconLabelMargins = labelMargins;
    iconLabelMargins.setLeft(0);
    for (auto iconLabel : std::vector<Widget*>{ d->spellCheckerUserDictionary,
                                                d->simpleTextEditorDefaultTemplateOptions,
                                                d->screenplayEditorDefaultTemplateOptions,
                                                d->comicBookEditorDefaultTemplateOptions }) {
        iconLabel->setBackgroundColor(DesignSystem::color().background());
        iconLabel->setTextColor(DesignSystem::color().onBackground());
        iconLabel->setContentsMargins(iconLabelMargins);
    }

    for (auto checkBox : {
             d->autoSave,
             d->saveBackups,
             //
             d->showDocumentsPages,
             d->useTypewriterSound,
             d->useSpellChecker,
             d->highlightCurrentLine,
             d->focusCurrentParagraph,
             d->useTypewriterScrolling,
             //
             d->simpleTextNavigatorShowSceneText,
             //
             d->screenplayEditorShowSceneNumber,
             d->screenplayEditorShowSceneNumberOnLeft,
             d->screenplayEditorShowSceneNumberOnRight,
             d->screenplayEditorShowDialogueNumber,
             d->screenplayEditorContinueDialogue,
             d->screenplayEditorUseCharactersFromText,
             d->screenplayNavigatorShowSceneNumber,
             d->screenplayNavigatorShowSceneText,
             d->screenplayDurationByCharactersIncludingSpaces,
             //
             d->comicBookNavigatorShowSceneText,
         }) {
        checkBox->setBackgroundColor(DesignSystem::color().background());
        checkBox->setTextColor(DesignSystem::color().onBackground());
    }

    for (auto radioButton : {
             d->simpleTextNavigatorSceneDescriptionLines1,
             d->simpleTextNavigatorSceneDescriptionLines2,
             d->simpleTextNavigatorSceneDescriptionLines3,
             d->simpleTextNavigatorSceneDescriptionLines4,
             d->simpleTextNavigatorSceneDescriptionLines5,
             d->screenplayNavigatorSceneDescriptionLines1,
             d->screenplayNavigatorSceneDescriptionLines2,
             d->screenplayNavigatorSceneDescriptionLines3,
             d->screenplayNavigatorSceneDescriptionLines4,
             d->screenplayNavigatorSceneDescriptionLines5,
             d->screenplayDurationByPage,
             d->screenplayDurationByCharacters,
             d->comicBookNavigatorSceneDescriptionLines1,
             d->comicBookNavigatorSceneDescriptionLines2,
             d->comicBookNavigatorSceneDescriptionLines3,
             d->comicBookNavigatorSceneDescriptionLines4,
             d->comicBookNavigatorSceneDescriptionLines5,
         }) {
        radioButton->setBackgroundColor(DesignSystem::color().background());
        radioButton->setTextColor(DesignSystem::color().onBackground());
    }

    for (auto textField : QVector<TextField*>{
             d->backupsFolderPath, d->spellCheckerLanguage, d->simpleTextEditorDefaultTemplate,
             d->screenplayEditorDefaultTemplate, d->screenplayDurationByPagePage,
             d->screenplayDurationByPageDuration, d->screenplayDurationByCharactersCharacters,
             d->screenplayDurationByCharactersDuration, d->comicBookEditorDefaultTemplate }) {
        textField->setBackgroundColor(DesignSystem::color().onBackground());
        textField->setTextColor(DesignSystem::color().onBackground());
    }
    for (auto textField :
         { /*d->simpleTextEditorDefaultTemplate,*/ d->screenplayEditorDefaultTemplate,
           /*d->comicBookEditorDefaultTemplate*/ }) {
        textField->setCustomMargins({ isLeftToRight() ? Ui::DesignSystem::layout().px24() : 0, 0,
                                      isLeftToRight() ? 0 : Ui::DesignSystem::layout().px24(), 0 });
    }
    for (auto icon :
         { d->simpleTextEditorDefaultTemplateOptions, d->screenplayEditorDefaultTemplateOptions,
           d->comicBookEditorDefaultTemplateOptions }) {
        icon->setContentsMargins(isLeftToRight() ? 0 : Ui::DesignSystem::layout().px16(), 0,
                                 isLeftToRight() ? Ui::DesignSystem::layout().px16() : 0, 0);
    }

    for (auto button : { d->changeLanuage }) {
        button->setBackgroundColor(DesignSystem::color().secondary());
        button->setTextColor(DesignSystem::color().secondary());
    }

    for (auto theme : {
             d->lightTheme,
             d->darkAndLightTheme,
             d->darkTheme,
             d->customTheme,
         }) {
        theme->setBackgroundColor(Ui::DesignSystem::color().background());
        theme->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    d->scaleFactor->setBackgroundColor(DesignSystem::color().background());
    d->scaleFactor->setContentsMargins({ static_cast<int>(Ui::DesignSystem::layout().px24()), 0,
                                         static_cast<int>(Ui::DesignSystem::layout().px24()), 0 });

    d->applicationCardLayout->setRowMinimumHeight(
        d->applicationCardBottomSpacerIndex, static_cast<int>(Ui::DesignSystem::layout().px24()));
    d->applicationThemesLayout->setSpacing(Ui::DesignSystem::layout().px24());
    d->applicationThemesLayout->setContentsMargins(
        QMargins(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px12(),
                 Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px16()));
    //
    d->simpleTextCardLayout->setRowMinimumHeight(
        d->simpleTextCardBottomSpacerIndex, static_cast<int>(Ui::DesignSystem::layout().px24()));
    //
    d->screenplayCardLayout->setRowMinimumHeight(
        d->screenplayCardBottomSpacerIndex, static_cast<int>(Ui::DesignSystem::layout().px24()));
    const auto screenplayDurationByCharactersRow
        = d->screenplayCardLayout->indexOf(d->screenplayDurationByCharacters);
    d->screenplayCardLayout->setRowMinimumHeight(
        screenplayDurationByCharactersRow, static_cast<int>(Ui::DesignSystem::layout().px62()));
    //
    d->comicBookCardLayout->setRowMinimumHeight(
        d->comicBookCardBottomSpacerIndex, static_cast<int>(Ui::DesignSystem::layout().px24()));
    //
    d->shortcutsCardLayout->setRowMinimumHeight(
        d->shortcutsCardBottomSpacerIndex, static_cast<int>(Ui::DesignSystem::layout().px24()));

    for (auto table : {
             d->shortcutsForScreenplay,
         }) {
        table->setBackgroundColor(Ui::DesignSystem::color().background());
        table->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    d->updateTablesGeometry();
}

} // namespace Ui
