#include "settings_manager.h"

#include "screenplay_template_manager.h"

#include <3rd_party/webloader/src/NetworkRequest.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/settings/language_dialog.h>
#include <ui/settings/settings_navigator.h>
#include <ui/settings/settings_tool_bar.h>
#include <ui/settings/settings_view.h>
#include <ui/settings/theme_setup_view.h>
#include <ui/widgets/task_bar/task_bar.h>
#include <ui/widgets/tree/tree_header_view.h>
#include <utils/helpers/dialog_helper.h>
#include <utils/helpers/extension_helper.h>
#include <utils/helpers/shortcuts_helper.h>

#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTimer>


namespace ManagementLayer {

namespace {

/**
 * @brief Сформировать идентификатор задачи для загрузки языка проверки орфографии
 */
QString spellCheckerLoadingTaskId(const QString& _languageId)
{
    return QLatin1String("spell_checker_loading_task_id_") + _languageId;
}

} // namespace

class SettingsManager::Implementation
{
public:
    explicit Implementation(QObject* _parent, QWidget* _parentWidget,
                            const PluginsBuilder& _pluginsBuilder);

    /**
     * @brief Загрузить параметры приложения
     */
    void loadApplicationSettings();

    /**
     * @brief Загрузить настройки компонентов
     */
    void loadComponentsSettings();
    void loadSimpleTextSettings();
    void loadScreenplaySettings();
    void loadComicBookSettings();
    void loadShortcutsSettings();


    Ui::SettingsToolBar* toolBar = nullptr;
    Ui::SettingsNavigator* navigator = nullptr;
    Ui::SettingsView* view = nullptr;
    Ui::ThemeSetupView* themeSetupView = nullptr;

    ScreenplayTemplateManager* screenplayTemplateManager = nullptr;
};

SettingsManager::Implementation::Implementation(QObject* _parent, QWidget* _parentWidget,
                                                const PluginsBuilder& _pluginsBuilder)
    : toolBar(new Ui::SettingsToolBar(_parentWidget))
    , navigator(new Ui::SettingsNavigator(_parentWidget))
    , view(new Ui::SettingsView(_parentWidget))
    , screenplayTemplateManager(
          new ScreenplayTemplateManager(_parent, _parentWidget, _pluginsBuilder))
{
    toolBar->hide();
    navigator->hide();
    view->hide();
}

void SettingsManager::Implementation::loadApplicationSettings()
{
    view->setApplicationLanguage(settingsValue(DataStorageLayer::kApplicationLanguagedKey).toInt());
    view->setApplicationScaleFactor(
        settingsValue(DataStorageLayer::kApplicationScaleFactorKey).toReal());
    view->setApplicationUseAutoSave(
        settingsValue(DataStorageLayer::kApplicationUseAutoSaveKey).toBool());
    view->setApplicationSaveBackups(
        settingsValue(DataStorageLayer::kApplicationSaveBackupsKey).toBool());
    view->setApplicationBackupsFolder(
        settingsValue(DataStorageLayer::kApplicationBackupsFolderKey).toString());
    view->setApplicationShowDocumentsPages(
        settingsValue(DataStorageLayer::kApplicationShowDocumentsPagesKey).toBool());
    view->setApplicationUseTypewriterSound(
        settingsValue(DataStorageLayer::kApplicationUseTypewriterSoundKey).toBool());
    view->setApplicationUseSpellChecker(
        settingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey).toBool());
    view->setApplicationSpellCheckerLanguage(
        settingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey).toString());
    view->setApplicationHighlightCurrentLine(
        settingsValue(DataStorageLayer::kApplicationHighlightCurrentLineKey).toBool());
    view->setApplicationFocusCurrentParagraph(
        settingsValue(DataStorageLayer::kApplicationFocusCurrentParagraphKey).toBool());
    view->setApplicationUseTypewriterScrolling(
        settingsValue(DataStorageLayer::kApplicationUseTypewriterScrollingKey).toBool());
}

void SettingsManager::Implementation::loadComponentsSettings()
{
    loadSimpleTextSettings();
    loadScreenplaySettings();
    loadComicBookSettings();
}

void SettingsManager::Implementation::loadSimpleTextSettings()
{
    const auto defaultTemplate
        = settingsValue(DataStorageLayer::kComponentsSimpleTextEditorDefaultTemplateKey).toString();
    view->setSimpleTextEditorDefaultTemplate(defaultTemplate);
    BusinessLayer::TemplatesFacade::setDefaultSimpleTextTemplate(defaultTemplate);
    //
    view->setSimpleTextNavigatorShowSceneText(
        settingsValue(DataStorageLayer::kComponentsSimpleTextNavigatorShowSceneTextKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsSimpleTextNavigatorSceneTextLinesKey).toInt());
}

void SettingsManager::Implementation::loadScreenplaySettings()
{
    const auto defaultTemplate
        = settingsValue(DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey).toString();
    view->setScreenplayEditorDefaultTemplate(defaultTemplate);
    BusinessLayer::TemplatesFacade::setDefaultScreenplayTemplate(defaultTemplate);
    view->setScreenplayEditorShowSceneNumber(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey)
            .toBool(),
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey)
            .toBool());
    view->setScreenplayEditorShowDialogueNumber(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey)
            .toBool());
    view->setScreenplayEditorContinueDialogue(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorContinueDialogueKey).toBool());
    view->setScreenplayEditorUseCharactersFromText(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorUseCharactersFromTextKey)
            .toBool());
    //
    view->setScreenplayNavigatorShowSceneNumber(
        settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneNumberKey).toBool());
    view->setScreenplayNavigatorShowSceneText(
        settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneTextKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorSceneTextLinesKey).toInt());
    //
    view->setScreenplayDurationType(
        settingsValue(DataStorageLayer::kComponentsScreenplayDurationTypeKey).toInt());
    view->setScreenplayDurationByPageDuration(
        settingsValue(DataStorageLayer::kComponentsScreenplayDurationByPageDurationKey).toInt());
    view->setScreenplayDurationByCharactersCharacters(
        settingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersCharactersKey)
            .toInt());
    view->setScreenplayDurationByCharactersIncludeSpaces(
        settingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersIncludeSpacesKey)
            .toInt());
    view->setScreenplayDurationByCharactersDuration(
        settingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersDurationKey)
            .toInt());
}

void SettingsManager::Implementation::loadComicBookSettings()
{
    const auto defaultTemplate
        = settingsValue(DataStorageLayer::kComponentsComicBookEditorDefaultTemplateKey).toString();
    view->setComicBookEditorDefaultTemplate(defaultTemplate);
    BusinessLayer::TemplatesFacade::setDefaultComicBookTemplate(defaultTemplate);
    //
    view->setComicBookNavigatorShowSceneText(
        settingsValue(DataStorageLayer::kComponentsComicBookNavigatorShowSceneTextKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsComicBookNavigatorSceneTextLinesKey).toInt());
}

void SettingsManager::Implementation::loadShortcutsSettings()
{
    using namespace BusinessLayer;

    //
    // Сформировать строку таблицы переходов между блоками для заданного типа
    //
    auto blocksJumpsModelRow = [](TextParagraphType _type) {
        const auto shortcut = ShortcutsHelper::screenplayShortcut(_type);
        const auto jumpForTab = ShortcutsHelper::screenplayJumpByTab(_type);
        const auto jumpForEnter = ShortcutsHelper::screenplayJumpByEnter(_type);
        const auto changeForTab = ShortcutsHelper::screenplayChangeByTab(_type);
        const auto changeForEnter = ShortcutsHelper::screenplayChangeByEnter(_type);

        QList<QStandardItem*> result;
        result << new QStandardItem(toDisplayString(_type)) << new QStandardItem(shortcut)
               << new QStandardItem(toDisplayString(textParagraphTypeFromString(jumpForTab)))
               << new QStandardItem(toDisplayString(textParagraphTypeFromString(jumpForEnter)))
               << new QStandardItem(toDisplayString(textParagraphTypeFromString(changeForTab)))
               << new QStandardItem(toDisplayString(textParagraphTypeFromString(changeForEnter)));
        result.first()->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return result;
    };
    //
    // ... таблика переходов между разными блоками
    //
    const int rows = 0;
    const int columns = 6;
    QStandardItemModel* blocksJumpsModel = new QStandardItemModel(rows, columns, view);
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::SceneHeading));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::SceneCharacters));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Action));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Character));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Parenthetical));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Dialogue));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Lyrics));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Transition));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::Shot));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::InlineNote));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::FolderHeader));
    blocksJumpsModel->appendRow(blocksJumpsModelRow(TextParagraphType::UnformattedText));
    //
    // ... модель переходов между блоками, её заголовок и делегат
    //
    QStandardItemModel* blockJumpsHeaderModel = new QStandardItemModel(view);
    {
        auto columnAfterText = new QStandardItem();
        {
            auto tab = new QStandardItem("Tab");
            tab->setData(u8"\U000F0312", Qt::DecorationRole);
            columnAfterText->appendColumn({ tab });

            auto enter = new QStandardItem("Enter");
            enter->setData(u8"\U000F0311", Qt::DecorationRole);
            columnAfterText->appendColumn({ enter });
        }
        auto columnEmptyText = new QStandardItem();
        {
            auto tab = new QStandardItem("Tab");
            tab->setData(u8"\U000F0312", Qt::DecorationRole);
            columnEmptyText->appendColumn({ tab });

            auto enter = new QStandardItem("Enter");
            enter->setData(u8"\U000F0311", Qt::DecorationRole);
            columnEmptyText->appendColumn({ enter });
        }

        blockJumpsHeaderModel->setItem(0, 0, new QStandardItem());
        blockJumpsHeaderModel->setItem(0, 1, new QStandardItem());
        blockJumpsHeaderModel->setItem(0, 2, columnAfterText);
        blockJumpsHeaderModel->setItem(0, 3, columnEmptyText);
    }

    auto model = new HierarchicalModel(view);
    model->setSourceModel(blocksJumpsModel);
    model->setHeaderModel(blockJumpsHeaderModel);
    view->setShortcutsForScreenplayModel(model);
}


// ****


SettingsManager::SettingsManager(QObject* _parent, QWidget* _parentWidget,
                                 const PluginsBuilder& _pluginsBuilder)
    : QObject(_parent)
    , d(new Implementation(this, _parentWidget, _pluginsBuilder))
{
    d->loadApplicationSettings();
    d->loadComponentsSettings();
    d->loadShortcutsSettings();
    d->view->installEventFilter(this);

    connect(d->toolBar, &Ui::SettingsToolBar::backPressed, this,
            &SettingsManager::closeSettingsRequested);

    connect(d->navigator, &Ui::SettingsNavigator::applicationPressed, d->view,
            &Ui::SettingsView::showApplication);
    connect(d->navigator, &Ui::SettingsNavigator::applicationUserInterfacePressed, d->view,
            &Ui::SettingsView::showApplicationUserInterface);
    connect(d->navigator, &Ui::SettingsNavigator::applicationSaveAndBackupsPressed, d->view,
            &Ui::SettingsView::showApplicationSaveAndBackups);
    connect(d->navigator, &Ui::SettingsNavigator::applicationTextEditingPressed, d->view,
            &Ui::SettingsView::showApplicationTextEditing);
    connect(d->navigator, &Ui::SettingsNavigator::componentsPressed, d->view,
            &Ui::SettingsView::showComponents);
    connect(d->navigator, &Ui::SettingsNavigator::componentsSimpleTextPressed, d->view,
            &Ui::SettingsView::showComponentsSimpleText);
    connect(d->navigator, &Ui::SettingsNavigator::componentsScreenplayPressed, d->view,
            &Ui::SettingsView::showComponentsScreenplay);
    connect(d->navigator, &Ui::SettingsNavigator::componentsComicBookPressed, d->view,
            &Ui::SettingsView::showComponentsComicBook);
    connect(d->navigator, &Ui::SettingsNavigator::shortcutsPressed, d->view,
            &Ui::SettingsView::showShortcuts);

    connect(d->view, &Ui::SettingsView::applicationLanguagePressed, this, [this, _parentWidget] {
        auto dialog = new Ui::LanguageDialog(_parentWidget);
        dialog->setCurrentLanguage(QLocale().language());
        dialog->showDialog();
        connect(dialog, &Ui::LanguageDialog::languageChanged, this,
                &SettingsManager::applicationLanguageChanged);
        connect(dialog, &Ui::LanguageDialog::languageChanged, this,
                &SettingsManager::setApplicationLanguage);
        connect(dialog, &Ui::LanguageDialog::disappeared, dialog, &Ui::LanguageDialog::deleteLater);
    });
    connect(
        d->view, &Ui::SettingsView::applicationThemePressed, this,
        [this](Ui::ApplicationTheme _theme) {
            //
            // Настроим видимость панели настройки темы приложения
            //
            if (_theme == Ui::ApplicationTheme::Custom) {
                //
                // Для кастомной темы сохраним текущее состояние, чтобы пользователь
                // мог отменить свои изменения в ней
                //
                d->themeSetupView->setSourceThemeHash(
                    settingsValue(DataStorageLayer::kApplicationCustomThemeColorsKey).toString());
                //
                // Показываем панель параметров темы отложенно, чтобы гармонично сочеталось с
                // анимацией смены темы, в случае если она будет активирована
                //
                QTimer::singleShot(200, this, [this] { d->themeSetupView->showView(); });
            } else {
                d->themeSetupView->hideView();
            }

            if (_theme == Ui::DesignSystem::theme()) {
                return;
            }

            //
            // Уведомляем об изменении темы
            //
            setApplicationTheme(_theme);
            emit applicationThemeChanged(_theme);
        });
    connect(d->view, &Ui::SettingsView::applicationScaleFactorChanged, this,
            &SettingsManager::setApplicationScaleFactor);
    connect(d->view, &Ui::SettingsView::applicationUseAutoSaveChanged, this,
            &SettingsManager::setApplicationUseAutoSave);
    connect(d->view, &Ui::SettingsView::applicationSaveBackupsChanged, this,
            &SettingsManager::setApplicationSaveBackups);
    connect(d->view, &Ui::SettingsView::applicationBackupsFolderChanged, this,
            &SettingsManager::setApplicationBackupsFolder);
    connect(d->view, &Ui::SettingsView::applicationShowDocumentsPagesChanged, this,
            &SettingsManager::setApplicationShowDocumentsPages);
    connect(d->view, &Ui::SettingsView::applicationUseTypewriterSoundChanged, this,
            &SettingsManager::setApplicationUseTypeWriterSound);
    connect(d->view, &Ui::SettingsView::applicationUseSpellCheckerChanged, this,
            &SettingsManager::setApplicationUseSpellChecker);
    connect(d->view, &Ui::SettingsView::applicationSpellCheckerLanguageChanged, this,
            &SettingsManager::setApplicationSpellCheckerLanguage);
    connect(d->view, &Ui::SettingsView::applicationHighlightCurentLineChanged, this,
            &SettingsManager::setApplicationHighlightCurrentLine);
    connect(d->view, &Ui::SettingsView::applicationFocusCurrentParagraphChanged, this,
            &SettingsManager::setApplicationFocusCurrentParagraph);
    connect(d->view, &Ui::SettingsView::applicationUseTypewriterScrollingChanged, this,
            &SettingsManager::setApplicationUseTypewriterScrolling);
    //
    //
    connect(d->view, &Ui::SettingsView::simpleTextEditorDefaultTemplateChanged, this,
            &SettingsManager::setSimpleTextEditorDefaultTemplate);
    //
    connect(d->view, &Ui::SettingsView::simpleTextNavigatorShowSceneTextChanged, this,
            &SettingsManager::setSimpleTextNavigatorShowSceneText);
    //
    //
    connect(d->view, &Ui::SettingsView::screenplayEditorDefaultTemplateChanged, this,
            &SettingsManager::setScreenplayEditorDefaultTemplate);
    connect(d->view, &Ui::SettingsView::screenplayEditorShowSceneNumberChanged, this,
            &SettingsManager::setScreenplayEditorShowSceneNumber);
    connect(d->view, &Ui::SettingsView::screenplayEditorShowDialogueNumberChanged, this,
            &SettingsManager::setScreenplayEditorShowDialogueNumber);
    connect(d->view, &Ui::SettingsView::screenplayEditorContinueDialogueChanged, this,
            &SettingsManager::setScreenplayEditorContinueDialogue);
    connect(d->view, &Ui::SettingsView::screenplayEditorUseCharactersFromTextChanged, this,
            &SettingsManager::setScreenplayEditorUseCharactersFromText);
    //
    connect(d->view, &Ui::SettingsView::screenplayNavigatorShowSceneNumberChanged, this,
            &SettingsManager::setScreenplayNavigatorShowSceneNumber);
    connect(d->view, &Ui::SettingsView::screenplayNavigatorShowSceneTextChanged, this,
            &SettingsManager::setScreenplayNavigatorShowSceneText);
    //
    connect(d->view, &Ui::SettingsView::screenplayDurationTypeChanged, this,
            &SettingsManager::setScreenplayDurationType);
    connect(d->view, &Ui::SettingsView::screenplayDurationByPageDurationChanged, this,
            &SettingsManager::setScreenplayDurationByPageDuration);
    connect(d->view, &Ui::SettingsView::screenplayDurationByCharactersCharactersChanged, this,
            &SettingsManager::setScreenplayDurationByCharactersCharacters);
    connect(d->view, &Ui::SettingsView::screenplayDurationByCharactersIncludeSpacesChanged, this,
            &SettingsManager::setScreenplayDurationByCharactersIncludeSpaces);
    connect(d->view, &Ui::SettingsView::screenplayDurationByCharactersDurationChanged, this,
            &SettingsManager::setScreenplayDurationByCharactersDuration);
    //
    //
    connect(d->view, &Ui::SettingsView::comicBookEditorDefaultTemplateChanged, this,
            &SettingsManager::setComicBookEditorDefaultTemplate);
    //
    connect(d->view, &Ui::SettingsView::comicBookNavigatorShowSceneTextChanged, this,
            &SettingsManager::setComicBookNavigatorShowSceneText);

    //
    // Работа с библиотекой шаблонов сценария
    //
    auto showScreenplayTemplateEditor = [this] {
        d->toolBar->setCurrentWidget(d->screenplayTemplateManager->toolBar());
        d->navigator->setCurrentWidget(d->screenplayTemplateManager->navigator());
        d->view->setCurrentWidget(d->screenplayTemplateManager->view());
        d->screenplayTemplateManager->viewToolBar()->setParent(d->view);
        d->screenplayTemplateManager->viewToolBar()->show();
    };
    connect(d->view, &Ui::SettingsView::editCurrentScreenplayEditorTemplateRequested, this,
            [this, showScreenplayTemplateEditor](const QString& _templateId) {
                d->screenplayTemplateManager->editTemplate(_templateId);
                showScreenplayTemplateEditor();
            });
    connect(d->view, &Ui::SettingsView::duplicateCurrentScreenplayEditorTemplateRequested, this,
            [this, showScreenplayTemplateEditor](const QString& _templateId) {
                d->screenplayTemplateManager->duplicateTemplate(_templateId);
                showScreenplayTemplateEditor();
            });
    connect(d->view, &Ui::SettingsView::saveToFileCurrentScreenplayEditorTemplateRequested, this,
            [this](const QString& _templateId) {
                auto saveToFilePath = QFileDialog::getSaveFileName(
                    d->view->topLevelWidget(), tr("Choose the file to save template"),
                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                    DialogHelper::starcTemplateFilter());
                if (saveToFilePath.isEmpty()) {
                    return;
                }

                if (!saveToFilePath.endsWith(ExtensionHelper::starct())) {
                    saveToFilePath.append(QString(".%1").arg(ExtensionHelper::starct()));
                }
                const auto screenplayTemplate
                    = BusinessLayer::TemplatesFacade::screenplayTemplate(_templateId);
                screenplayTemplate.saveToFile(saveToFilePath);
            });
    connect(d->view, &Ui::SettingsView::removeCurrentScreenplayEditorTemplateRequested, this,
            [](const QString& _templateId) {
                BusinessLayer::TemplatesFacade::removeScreenplayTemplate(_templateId);
            });
    connect(d->view, &Ui::SettingsView::loadFromFileScreenplayEditorTemplateRequested, this,
            [this] {
                const auto templateFilePath = QFileDialog::getOpenFileName(
                    d->view->topLevelWidget(), tr("Choose the file with template to load"),
                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                    DialogHelper::starcTemplateFilter());
                if (templateFilePath.isEmpty()) {
                    return;
                }

                const BusinessLayer::ScreenplayTemplate screenplayTemplate(templateFilePath);
                BusinessLayer::TemplatesFacade::saveScreenplayTemplate(screenplayTemplate);
            });
    connect(d->screenplayTemplateManager, &ScreenplayTemplateManager::closeRequested, this, [this] {
        d->toolBar->showDefaultPage();
        d->navigator->showDefaultPage();
        d->view->showDefaultPage();
        d->screenplayTemplateManager->viewToolBar()->hide();

        //
        // После закрытия уведомляем клиентов о том, что текущий шаблон обновился
        //
        emit screenplayEditorChanged(
            { DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey });
    });
    connect(d->screenplayTemplateManager, &ScreenplayTemplateManager::showViewRequested, this,
            [this](QWidget* _view) {
                d->view->setCurrentWidget(_view);
                d->screenplayTemplateManager->viewToolBar()->raise();
            });

    //
    // Горячие клавиши
    //
    connect(d->view, &Ui::SettingsView::shortcutsForScreenplayEditorChanged, this,
            &SettingsManager::setShortcutsForScreenplayEdit);
}

SettingsManager::~SettingsManager() = default;

QWidget* SettingsManager::toolBar() const
{
    return d->toolBar;
}

QWidget* SettingsManager::navigator() const
{
    return d->navigator;
}

QWidget* SettingsManager::view() const
{
    return d->view;
}

void SettingsManager::setThemeSetupView(Ui::ThemeSetupView* _view)
{
    d->themeSetupView = _view;
    connect(d->themeSetupView, &Ui::ThemeSetupView::customThemeColorsChanged, this,
            &SettingsManager::applicationCustomThemeColorsChanged);
    connect(d->themeSetupView, &Ui::ThemeSetupView::customThemeColorsChanged, this,
            &SettingsManager::setApplicationCustomThemeColors);
}

void SettingsManager::updateScaleFactor()
{
    d->view->setApplicationScaleFactor(
        settingsValue(DataStorageLayer::kApplicationScaleFactorKey).toReal());
}

bool SettingsManager::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_event->type() == QEvent::LanguageChange && _watched == d->view) {
        d->view->setApplicationLanguage(
            settingsValue(DataStorageLayer::kApplicationLanguagedKey).toInt());
        d->loadShortcutsSettings();
    }

    return QObject::eventFilter(_watched, _event);
}

void SettingsManager::setApplicationLanguage(int _language)
{
    setSettingsValue(DataStorageLayer::kApplicationLanguagedKey, _language);
}

void SettingsManager::setApplicationTheme(Ui::ApplicationTheme _theme)
{
    setSettingsValue(DataStorageLayer::kApplicationThemeKey, static_cast<int>(_theme));
}

void SettingsManager::setApplicationCustomThemeColors(const Ui::DesignSystem::Color& _color)
{
    setSettingsValue(DataStorageLayer::kApplicationCustomThemeColorsKey, _color.toString());
}

void SettingsManager::setApplicationScaleFactor(qreal _scaleFactor)
{
    setSettingsValue(DataStorageLayer::kApplicationScaleFactorKey, _scaleFactor);
    emit applicationScaleFactorChanged(_scaleFactor);
}

void SettingsManager::setApplicationUseAutoSave(bool _use)
{
    setSettingsValue(DataStorageLayer::kApplicationUseAutoSaveKey, _use);
    emit applicationUseAutoSaveChanged(_use);
}

void SettingsManager::setApplicationSaveBackups(bool _save)
{
    setSettingsValue(DataStorageLayer::kApplicationSaveBackupsKey, _save);
    emit applicationSaveBackupsChanged(_save);
}

void SettingsManager::setApplicationBackupsFolder(const QString& _path)
{
    setSettingsValue(DataStorageLayer::kApplicationBackupsFolderKey, _path);
    emit applicationBackupsFolderChanged(_path);
}

void SettingsManager::setApplicationShowDocumentsPages(bool _show)
{
    setSettingsValue(DataStorageLayer::kApplicationShowDocumentsPagesKey, _show);
    emit simpleTextEditorChanged({ DataStorageLayer::kApplicationShowDocumentsPagesKey });
    emit screenplayEditorChanged({ DataStorageLayer::kApplicationShowDocumentsPagesKey });
    emit comicBookEditorChanged({ DataStorageLayer::kApplicationShowDocumentsPagesKey });
}

void SettingsManager::setApplicationUseTypeWriterSound(bool _use)
{
    setSettingsValue(DataStorageLayer::kApplicationUseTypewriterSoundKey, _use);
}

void SettingsManager::setApplicationUseSpellChecker(bool _use)
{
    setSettingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey, _use);
    emit applicationUseSpellCheckerChanged(_use);
}

void SettingsManager::setApplicationSpellCheckerLanguage(const QString& _languageCode)
{
    //
    // Сохраняем значение выбранного языка
    //
    setSettingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey, _languageCode);

    //
    // Проверяем установлен ли выбранный словарь
    //
    const QString appDataFolderPath
        = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString hunspellDictionariesFolderPath = appDataFolderPath + "/hunspell/";
    const QString affFileName = _languageCode + ".aff";
    const QString dicFileName = _languageCode + ".dic";
    //
    // Получим информацию о файлах словаря
    //
    const QFileInfo affFileInfo(hunspellDictionariesFolderPath + affFileName);
    const QFileInfo dicFileInfo(hunspellDictionariesFolderPath + dicFileName);

    //
    // Если словарь установлен, просто будем использовать его
    //
    if (affFileInfo.exists() && dicFileInfo.exists()) {
        emit applicationSpellCheckerLanguageChanged(_languageCode);
        return;
    }

    //
    // Ежели словарь не установлен, будем качать
    //
    loadSpellingDictionary(_languageCode);
}

void SettingsManager::loadSpellingDictionary(const QString& _languageCode)
{
    //
    // Добавляем идентификацию процесса загрузки словарей
    //
    const auto taskId = spellCheckerLoadingTaskId(_languageCode);
    TaskBar::addTask(taskId);
    TaskBar::setTaskTitle(taskId,
                          tr("Spelling dictionary loading") + QString(" (%1)").arg(_languageCode));
    TaskBar::setTaskProgress(taskId, 0.0);

    //
    // Создаём папку для пользовательских файлов
    //
    const QString appDataFolderPath
        = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString hunspellDictionariesFolderPath = appDataFolderPath + "/hunspell/";
    QDir::root().mkpath(hunspellDictionariesFolderPath);

    //
    // Начинаем загрузку файлов словаря
    //
    loadSpellingDictionaryAffFile(_languageCode);
}

void SettingsManager::loadSpellingDictionaryAffFile(const QString& _languageCode)
{
    const auto hunspellDictionariesFolderUrl
        = QString("https://starc.app/downloads/hunspell/%1/").arg(_languageCode);
    const QString affFileName = _languageCode + ".aff";

    //
    // Настраиваем загрузчик
    //
    auto dictionaryLoader = new NetworkRequest;
    connect(dictionaryLoader, &NetworkRequest::downloadProgress, this, [_languageCode](int _value) {
        //
        // Aff-файлы считаем за 10 процентов всего словаря
        //
        const qreal progress = _value * 0.1;
        TaskBar::setTaskProgress(spellCheckerLoadingTaskId(_languageCode), progress);
    });
    connect(dictionaryLoader, &NetworkRequest::downloadComplete, this,
            [this, _languageCode, affFileName](const QByteArray& _data) {
                if (_data.isEmpty()) {
                    //
                    // TODO: сообщить об ошибке
                    //
                    return;
                }

                //
                // Сохраняем файл
                //
                QFile affFile(
                    QString("%1/hunspell/%2")
                        .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
                             affFileName));
                affFile.open(QIODevice::WriteOnly);
                affFile.write(_data);
                affFile.close();

                //
                // Загрузим следующий файл
                //
                loadSpellingDictionaryDicFile(_languageCode);
            });
    connect(dictionaryLoader, &NetworkRequest::error, this, [] {
        //
        // TODO: сообщить об ошибке
        //
    });
    connect(dictionaryLoader, &NetworkRequest::finished, dictionaryLoader,
            &NetworkRequest::deleteLater);

    //
    // Запускаем загрузку
    //
    dictionaryLoader->loadAsync(hunspellDictionariesFolderUrl + affFileName);
}

void SettingsManager::loadSpellingDictionaryDicFile(const QString& _languageCode)
{
    const auto hunspellDictionariesFolderUrl
        = QString("https://starc.app/downloads/hunspell/%1/").arg(_languageCode);
    const QString dicFileName = _languageCode + ".dic";

    //
    // Настраиваем загрузчик
    //
    auto dictionaryLoader = new NetworkRequest;
    connect(dictionaryLoader, &NetworkRequest::downloadProgress, this, [_languageCode](int _value) {
        //
        // Dic-файлы считаем за 90 процентов всего словаря
        //
        const qreal progress = 10 + _value * 0.9;
        TaskBar::setTaskProgress(spellCheckerLoadingTaskId(_languageCode), progress);
    });
    connect(dictionaryLoader, &NetworkRequest::downloadComplete, this,
            [this, _languageCode, dicFileName](const QByteArray& _data) {
                if (_data.isEmpty()) {
                    //
                    // TODO: сообщить об ошибке
                    //
                    return;
                }

                //
                // Сохраняем файл
                //
                QFile affFile(
                    QString("%1/hunspell/%2")
                        .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
                             dicFileName));
                affFile.open(QIODevice::WriteOnly);
                affFile.write(_data);
                affFile.close();

                //
                // Cкрываем прогресс
                //
                TaskBar::finishTask(spellCheckerLoadingTaskId(_languageCode));

                //
                // Уведимим клиентов, что теперь можно использовать данный словарь
                //
                emit applicationSpellCheckerLanguageChanged(_languageCode);
            });
    connect(dictionaryLoader, &NetworkRequest::error, this, [] {
        //
        // TODO: сообщить об ошибке
        //
    });
    connect(dictionaryLoader, &NetworkRequest::finished, dictionaryLoader,
            &NetworkRequest::deleteLater);

    //
    // Запускаем загрузку
    //
    dictionaryLoader->loadAsync(hunspellDictionariesFolderUrl + dicFileName);
}

void SettingsManager::setApplicationHighlightCurrentLine(bool _highlight)
{
    setSettingsValue(DataStorageLayer::kApplicationHighlightCurrentLineKey, _highlight);
    emit simpleTextEditorChanged({ DataStorageLayer::kApplicationHighlightCurrentLineKey });
    emit screenplayEditorChanged({ DataStorageLayer::kApplicationHighlightCurrentLineKey });
    emit comicBookEditorChanged({ DataStorageLayer::kApplicationHighlightCurrentLineKey });
}

void SettingsManager::setApplicationFocusCurrentParagraph(bool _focus)
{
    setSettingsValue(DataStorageLayer::kApplicationFocusCurrentParagraphKey, _focus);
    emit simpleTextEditorChanged({ DataStorageLayer::kApplicationFocusCurrentParagraphKey });
    emit screenplayEditorChanged({ DataStorageLayer::kApplicationFocusCurrentParagraphKey });
    emit comicBookEditorChanged({ DataStorageLayer::kApplicationFocusCurrentParagraphKey });
}

void SettingsManager::setApplicationUseTypewriterScrolling(bool _use)
{
    setSettingsValue(DataStorageLayer::kApplicationUseTypewriterScrollingKey, _use);
    emit simpleTextEditorChanged({ DataStorageLayer::kApplicationUseTypewriterScrollingKey });
    emit screenplayEditorChanged({ DataStorageLayer::kApplicationUseTypewriterScrollingKey });
    emit comicBookEditorChanged({ DataStorageLayer::kApplicationUseTypewriterScrollingKey });
}

void SettingsManager::setSimpleTextEditorDefaultTemplate(const QString& _templateId)
{
    setSettingsValue(DataStorageLayer::kComponentsSimpleTextEditorDefaultTemplateKey, _templateId);
    BusinessLayer::TemplatesFacade::setDefaultSimpleTextTemplate(_templateId);
    emit simpleTextEditorChanged(
        { DataStorageLayer::kComponentsSimpleTextEditorDefaultTemplateKey });
}

void SettingsManager::setSimpleTextNavigatorShowSceneText(bool _show, int _lines)
{
    setSettingsValue(DataStorageLayer::kComponentsSimpleTextNavigatorShowSceneTextKey, _show);
    setSettingsValue(DataStorageLayer::kComponentsSimpleTextNavigatorSceneTextLinesKey, _lines);
    emit simpleTextNavigatorChanged();
}

void SettingsManager::setScreenplayEditorDefaultTemplate(const QString& _templateId)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey, _templateId);
    BusinessLayer::TemplatesFacade::setDefaultScreenplayTemplate(_templateId);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey });
}

void SettingsManager::setScreenplayEditorShowSceneNumber(bool _show, bool _atLeft, bool _atRight)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey, _show);
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey,
                     _atLeft);
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey,
                     _atRight);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey,
          DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnLeftKey,
          DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey });
}

void SettingsManager::setScreenplayEditorShowDialogueNumber(bool _show)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey, _show);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumbersKey });
}

void SettingsManager::setScreenplayEditorContinueDialogue(bool _continue)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorContinueDialogueKey, _continue);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsScreenplayEditorContinueDialogueKey });
}

void SettingsManager::setScreenplayEditorUseCharactersFromText(bool _use)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorUseCharactersFromTextKey, _use);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsScreenplayEditorUseCharactersFromTextKey });
}

void SettingsManager::setScreenplayNavigatorShowSceneNumber(bool _show)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneNumberKey, _show);
    emit screenplayNavigatorChanged();
}

void SettingsManager::setScreenplayNavigatorShowSceneText(bool _show, int _lines)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneTextKey, _show);
    setSettingsValue(DataStorageLayer::kComponentsScreenplayNavigatorSceneTextLinesKey, _lines);
    emit screenplayNavigatorChanged();
}

void SettingsManager::setScreenplayDurationType(int _type)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationTypeKey, _type);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationByPageDuration(int _duration)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationByPageDurationKey, _duration);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationByCharactersCharacters(int _characters)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersCharactersKey,
                     _characters);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationByCharactersIncludeSpaces(bool _include)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersIncludeSpacesKey,
                     _include);
    emit screenplayDurationChanged();
}

void SettingsManager::setScreenplayDurationByCharactersDuration(int _duration)
{
    setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersDurationKey,
                     _duration);
    emit screenplayDurationChanged();
}

void SettingsManager::setComicBookEditorDefaultTemplate(const QString& _templateId)
{
    setSettingsValue(DataStorageLayer::kComponentsComicBookEditorDefaultTemplateKey, _templateId);
    BusinessLayer::TemplatesFacade::setDefaultComicBookTemplate(_templateId);
    emit screenplayEditorChanged(
        { DataStorageLayer::kComponentsComicBookEditorDefaultTemplateKey });
}

void SettingsManager::setComicBookNavigatorShowSceneText(bool _show, int _lines)
{
    setSettingsValue(DataStorageLayer::kComponentsComicBookNavigatorShowSceneTextKey, _show);
    setSettingsValue(DataStorageLayer::kComponentsComicBookNavigatorSceneTextLinesKey, _lines);
    emit comicBookNavigatorChanged();
}

void SettingsManager::setShortcutsForScreenplayEdit(
    const QString& _blockType, const QString& _shortcut, const QString& _jumpByTab,
    const QString& _jumpByEnter, const QString& _changeByTab, const QString& _changeByEnter)
{
    const auto blockType = BusinessLayer::textParagraphTypeFromDisplayString(_blockType);
    ShortcutsHelper::setScreenplayShortcut(blockType, _shortcut);
    ShortcutsHelper::setScreenplayJumpByTab(
        blockType, BusinessLayer::textParagraphTypeFromDisplayString(_jumpByTab));
    ShortcutsHelper::setScreenplayJumpByEnter(
        blockType, BusinessLayer::textParagraphTypeFromDisplayString(_jumpByEnter));
    ShortcutsHelper::setScreenplayChangeByTab(
        blockType, BusinessLayer::textParagraphTypeFromDisplayString(_changeByTab));
    ShortcutsHelper::setScreenplayChangeByEnter(
        blockType, BusinessLayer::textParagraphTypeFromDisplayString(_changeByEnter));
    emit screenplayEditorChanged({ DataStorageLayer::kComponentsScreenplayEditorShortcutsKey });
}

} // namespace ManagementLayer
