#include "settings_manager.h"

#include <include/custom_events.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <ui/settings/language_dialog.h>
#include <ui/settings/settings_navigator.h>
#include <ui/settings/settings_tool_bar.h>
#include <ui/settings/settings_view.h>
#include <ui/settings/theme_dialog.h>
#include <ui/widgets/task_bar/task_bar.h>

#include <3rd_party/webloader/src/NetworkRequest.h>

#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QStandardPaths>


namespace ManagementLayer
{

namespace {
const QString kSpellCheckerLoadingTaskId = "spell_checker_loading_task_id";
}

class SettingsManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить значение параметра из настроек по ключу
     */
    QVariant settingsValue(const QString& _key) const;

    /**
     * @brief Сохранить в настройках значение параметр по ключу
     */
    void setSettingsValue(const QString& _key, const QVariant& _value);

    /**
     * @brief Загрузить параметры приложения
     */
    void loadApplicationSettings();

    /**
     * @brief Загрузить настройки компонентов
     */
    void loadComponentsSettings();
    void loadScreenplaySettings();


    Ui::SettingsToolBar* toolBar = nullptr;
    Ui::SettingsNavigator* navigator = nullptr;
    Ui::SettingsView* view = nullptr;
};

SettingsManager::Implementation::Implementation(QWidget* _parent)
    : toolBar(new Ui::SettingsToolBar(_parent)),
      navigator(new Ui::SettingsNavigator(_parent)),
      view(new Ui::SettingsView(_parent))
{
    toolBar->hide();
    navigator->hide();
    view->hide();
}

QVariant SettingsManager::Implementation::settingsValue(const QString& _key) const
{
    return DataStorageLayer::StorageFacade::settingsStorage()->value(
                _key, DataStorageLayer::SettingsStorage::SettingsPlace::Application);
}

void SettingsManager::Implementation::setSettingsValue(const QString& _key, const QVariant& _value)
{
    DataStorageLayer::StorageFacade::settingsStorage()->setValue(
                _key, _value, DataStorageLayer::SettingsStorage::SettingsPlace::Application);
}

void SettingsManager::Implementation::loadApplicationSettings()
{
    view->setApplicationLanguage(settingsValue(DataStorageLayer::kApplicationLanguagedKey).toInt());
    view->setApplicationUseTypewriterSound(settingsValue(DataStorageLayer::kApplicationUseTypewriterSoundKey).toBool());
    view->setApplicationUseSpellChecker(settingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey).toBool());
    view->setApplicationSpellCheckerLanguage(settingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey).toString());
    view->setApplicationTheme(settingsValue(DataStorageLayer::kApplicationThemeKey).toInt());
    view->setApplicationScaleFactor(settingsValue(DataStorageLayer::kApplicationScaleFactorKey).toReal());
    view->setApplicationUseAutoSave(settingsValue(DataStorageLayer::kApplicationUseAutoSaveKey).toBool());
    view->setApplicationSaveBackups(settingsValue(DataStorageLayer::kApplicationSaveBackupsKey).toBool());
    view->setApplicationBackupsFolder(settingsValue(DataStorageLayer::kApplicationBackupsFolderKey).toString());
}

void SettingsManager::Implementation::loadComponentsSettings()
{
    loadScreenplaySettings();
}

void SettingsManager::Implementation::loadScreenplaySettings()
{
    view->setScreenplayEditorDefaultTemplate(settingsValue(DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey).toString());
    view->setScreenplayEditorShowSceneNumber(
                settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey).toBool(),
                settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey).toBool(),
                settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumberOnLeftKey).toBool());
    view->setScreenplayEditorShowDialogueNumber(settingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumberKey).toBool());
    view->setScreenplayEditorHighlightCurrentLine(settingsValue(DataStorageLayer::kComponentsScreenplayEditorHighlightCurrentLineKey).toBool());
    //
    view->setScreenplayNavigatorShowSceneNumber(settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneNumberKey).toBool());
    view->setScreenplayNavigatorShowSceneText(
                settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneTextKey).toBool(),
                settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorSceneTextLinesKey).toInt());
    //
    view->setScreenplayDurationType(settingsValue(DataStorageLayer::kComponentsScreenplayDurationTypeKey).toInt());
    view->setScreenplayDurationByPageDuration(settingsValue(DataStorageLayer::kComponentsScreenplayDurationByPageDurationKey).toInt());
    view->setScreenplayDurationByCharactersCharacters(settingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersCharactersKey).toInt());
    view->setScreenplayDurationByCharactersIncludeSpaces(settingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersIncludeSpacesKey).toInt());
    view->setScreenplayDurationByCharactersDuration(settingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersDurationKey).toInt());
}


// ****


SettingsManager::SettingsManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{
    d->loadApplicationSettings();
    d->loadComponentsSettings();
    d->view->installEventFilter(this);

    connect(d->toolBar, &Ui::SettingsToolBar::backPressed, this, &SettingsManager::closeSettingsRequested);

    connect(d->navigator, &Ui::SettingsNavigator::applicationPressed, d->view, &Ui::SettingsView::showApplication);
    connect(d->navigator, &Ui::SettingsNavigator::applicationUserInterfacePressed, d->view, &Ui::SettingsView::showApplicationUserInterface);
    connect(d->navigator, &Ui::SettingsNavigator::applicationSaveAndBackupsPressed, d->view, &Ui::SettingsView::showApplicationSaveAndBackups);
    connect(d->navigator, &Ui::SettingsNavigator::componentsPressed, d->view, &Ui::SettingsView::showComponents);
    connect(d->navigator, &Ui::SettingsNavigator::componentsScreenplayPressed, d->view, &Ui::SettingsView::showComponentsScreenplay);
    connect(d->navigator, &Ui::SettingsNavigator::shortcutsPressed, d->view, &Ui::SettingsView::showShortcuts);

    connect(d->view, &Ui::SettingsView::applicationLanguagePressed, this, [this, _parentWidget] {
        auto dialog = new Ui::LanguageDialog(_parentWidget);
        dialog->setCurrentLanguage(QLocale().language());
        dialog->showDialog();
        connect(dialog, &Ui::LanguageDialog::languageChanged, this, &SettingsManager::applicationLanguageChanged);
        connect(dialog, &Ui::LanguageDialog::languageChanged, this, &SettingsManager::setApplicationLanguage);
        connect(dialog, &Ui::LanguageDialog::disappeared, dialog, &Ui::LanguageDialog::deleteLater);
    });
    connect(d->view, &Ui::SettingsView::applicationThemePressed, this, [this, _parentWidget] {
        auto dialog = new Ui::ThemeDialog(_parentWidget);
        dialog->setCurrentTheme(Ui::DesignSystem::theme());
        dialog->showDialog();
        connect(dialog, &Ui::ThemeDialog::themeChanged, this, &SettingsManager::applicationThemeChanged);
        connect(dialog, &Ui::ThemeDialog::themeChanged, this, &SettingsManager::setApplicationTheme);
        connect(dialog, &Ui::ThemeDialog::customThemeColorsChanged, this, &SettingsManager::applicationCustomThemeColorsChanged);
        connect(dialog, &Ui::ThemeDialog::customThemeColorsChanged, this, &SettingsManager::setApplicationCustomThemeColors);
        connect(dialog, &Ui::ThemeDialog::disappeared, dialog, &Ui::ThemeDialog::deleteLater);
    });
    connect(d->view, &Ui::SettingsView::applicationUseTypewriterSoundChanged, this, &SettingsManager::setApplicationUseTypeWriterSound);
    connect(d->view, &Ui::SettingsView::applicationUseSpellCheckerChanged, this, &SettingsManager::setApplicationUseSpellChecker);
    connect(d->view, &Ui::SettingsView::applicationSpellCheckerLanguageChanged, this, &SettingsManager::setApplicationSpellCheckerLanguage);
    connect(d->view, &Ui::SettingsView::applicationScaleFactorChanged, this, &SettingsManager::setApplicationScaleFactor);
    connect(d->view, &Ui::SettingsView::applicationUseAutoSaveChanged, this, &SettingsManager::setApplicationUseAutoSave);
    connect(d->view, &Ui::SettingsView::applicationSaveBackupsChanged, this, &SettingsManager::setApplicationSaveBackups);
    connect(d->view, &Ui::SettingsView::applicationBackupsFolderChanged, this, &SettingsManager::setApplicationBackupsFolder);
    //
    //
    connect(d->view, &Ui::SettingsView::screenplayEditorDefaultTemplateChanged, this, &SettingsManager::setScreenplayEditorDefaultTemplate);
    connect(d->view, &Ui::SettingsView::screenplayEditorShowSceneNumberChanged, this, &SettingsManager::setScreenplayEditorShowSceneNumber);
    connect(d->view, &Ui::SettingsView::screenplayEditorShowDialogueNumberChanged, this, &SettingsManager::setScreenplayEditorShowDialogueNumber);
    connect(d->view, &Ui::SettingsView::screenplayEditorHighlightCurrentLineChanged, this, &SettingsManager::setScreenplayEditorHighlightCurrentLine);
    //
    connect(d->view, &Ui::SettingsView::screenplayNavigatorShowSceneNumberChanged, this, &SettingsManager::setScreenplayNavigatorShowSceneNumber);
    connect(d->view, &Ui::SettingsView::screenplayNavigatorShowSceneTextChanged, this, &SettingsManager::setScreenplayNavigatorShowSceneText);
    //
    connect(d->view, &Ui::SettingsView::screenplayDurationTypeChanged, this, &SettingsManager::setScreenplayDurationType);
    connect(d->view, &Ui::SettingsView::screenplayDurationByPageDurationChanged, this, &SettingsManager::setScreenplayDurationByPageDuration);
    connect(d->view, &Ui::SettingsView::screenplayDurationByCharactersCharactersChanged, this, &SettingsManager::setScreenplayDurationByCharactersCharacters);
    connect(d->view, &Ui::SettingsView::screenplayDurationByCharactersIncludeSpacesChanged, this, &SettingsManager::setScreenplayDurationByCharactersIncludeSpaces);
    connect(d->view, &Ui::SettingsView::screenplayDurationByCharactersDurationChanged, this, &SettingsManager::setScreenplayDurationByCharactersDuration);

    //
    // Нотификации об изменении параметров
    //
    connect(d->view, &Ui::SettingsView::applicationScaleFactorChanged, this, &SettingsManager::applicationScaleFactorChanged);
    connect(d->view, &Ui::SettingsView::applicationUseSpellCheckerChanged, this, &SettingsManager::applicationUseSpellCheckerChanged);
    connect(d->view, &Ui::SettingsView::applicationSpellCheckerLanguageChanged, this, &SettingsManager::applicationSpellCheckerLanguageChanged);
    connect(d->view, &Ui::SettingsView::applicationUseAutoSaveChanged, this, &SettingsManager::applicationUseAutoSaveChanged);
    connect(d->view, &Ui::SettingsView::applicationSaveBackupsChanged, this, &SettingsManager::applicationSaveBackupsChanged);
    connect(d->view, &Ui::SettingsView::applicationBackupsFolderChanged, this, &SettingsManager::applicationBackupsFolderChanged);
    //
    //
    connect(d->view, &Ui::SettingsView::screenplayNavigatorShowSceneNumberChanged, this, &SettingsManager::screenplayNavigatorChanged);
    connect(d->view, &Ui::SettingsView::screenplayNavigatorShowSceneTextChanged, this, &SettingsManager::screenplayNavigatorChanged);
    //
    connect(d->view, &Ui::SettingsView::screenplayDurationTypeChanged, this, &SettingsManager::screenplayDurationChanged);
    connect(d->view, &Ui::SettingsView::screenplayDurationByPageDurationChanged, this, &SettingsManager::screenplayDurationChanged);
    connect(d->view, &Ui::SettingsView::screenplayDurationByCharactersCharactersChanged, this, &SettingsManager::screenplayDurationChanged);
    connect(d->view, &Ui::SettingsView::screenplayDurationByCharactersIncludeSpacesChanged, this, &SettingsManager::screenplayDurationChanged);
    connect(d->view, &Ui::SettingsView::screenplayDurationByCharactersDurationChanged, this, &SettingsManager::screenplayDurationChanged);
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

void SettingsManager::updateScaleFactor()
{
    d->view->setApplicationScaleFactor(d->settingsValue(DataStorageLayer::kApplicationScaleFactorKey).toReal());
}

bool SettingsManager::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_event->type() == QEvent::LanguageChange
        && _watched == d->view) {
        d->view->setApplicationLanguage(d->settingsValue(DataStorageLayer::kApplicationLanguagedKey).toInt());
        d->view->setApplicationTheme(d->settingsValue(DataStorageLayer::kApplicationThemeKey).toInt());
    } else if (static_cast<EventType>(_event->type()) == EventType::DesignSystemChangeEvent) {
        d->view->setApplicationTheme(d->settingsValue(DataStorageLayer::kApplicationThemeKey).toInt());
    }

    return QObject::eventFilter(_watched, _event);
}

void SettingsManager::setApplicationLanguage(int _language)
{
    d->setSettingsValue(DataStorageLayer::kApplicationLanguagedKey, _language);
}

void SettingsManager::setApplicationUseTypeWriterSound(bool _use)
{
    d->setSettingsValue(DataStorageLayer::kApplicationUseTypewriterSoundKey, _use);
}

void SettingsManager::setApplicationUseSpellChecker(bool _use)
{
    d->setSettingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey, _use);
}

void SettingsManager::setApplicationSpellCheckerLanguage(const QString& _languageCode)
{
    //
    // Сохраняем значение выбранного языка
    //
    d->setSettingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey, _languageCode);

    //
    // Проверяем установлен ли выбранный словарь
    //
    const QString appDataFolderPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
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
    TaskBar::addTask(kSpellCheckerLoadingTaskId);
    TaskBar::setTaskTitle(kSpellCheckerLoadingTaskId, tr("Spelling dictionary loading"));
    TaskBar::setTaskProgress(kSpellCheckerLoadingTaskId, 0.0);

    //
    // Создаём папку для пользовательских файлов
    //
    const QString appDataFolderPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
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
    connect(dictionaryLoader, &NetworkRequest::downloadProgress, this, [] (int _value) {
        //
        // Aff-файлы считаем за 10 процентов всего словаря
        //
        const qreal progress = _value * 0.1;
        TaskBar::setTaskProgress(kSpellCheckerLoadingTaskId, progress);
    });
    connect(dictionaryLoader, &NetworkRequest::downloadComplete, this,
            [this, _languageCode, affFileName] (const QByteArray& _data)
    {
        if (_data.isEmpty()) {
            //
            // TODO: сообщить об ошибке
            //
            return;
        }

        //
        // Сохраняем файл
        //
        QFile affFile(QString("%1/hunspell/%2")
                      .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation), affFileName));
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
    connect(dictionaryLoader, &NetworkRequest::finished, dictionaryLoader, &NetworkRequest::deleteLater);

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
    connect(dictionaryLoader, &NetworkRequest::downloadProgress, this, [] (int _value) {
        //
        // Dic-файлы считаем за 90 процентов всего словаря
        //
        const qreal progress = 10 + _value * 0.9;
        TaskBar::setTaskProgress(kSpellCheckerLoadingTaskId, progress);
    });
    connect(dictionaryLoader, &NetworkRequest::downloadComplete, this,
            [this, _languageCode, dicFileName] (const QByteArray& _data)
    {
        if (_data.isEmpty()) {
            //
            // TODO: сообщить об ошибке
            //
            return;
        }

        //
        // Сохраняем файл
        //
        QFile affFile(QString("%1/hunspell/%2")
                      .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation), dicFileName));
        affFile.open(QIODevice::WriteOnly);
        affFile.write(_data);
        affFile.close();

        //
        // Cкрываем прогресс
        //
        TaskBar::finishTask(kSpellCheckerLoadingTaskId);

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
    connect(dictionaryLoader, &NetworkRequest::finished, dictionaryLoader, &NetworkRequest::deleteLater);

    //
    // Запускаем загрузку
    //
    dictionaryLoader->loadAsync(hunspellDictionariesFolderUrl + dicFileName);
}

void SettingsManager::setApplicationTheme(Ui::ApplicationTheme _theme)
{
    d->setSettingsValue(DataStorageLayer::kApplicationThemeKey, static_cast<int>(_theme));
}

void SettingsManager::setApplicationCustomThemeColors(const Ui::DesignSystem::Color& _color)
{
    d->setSettingsValue(DataStorageLayer::kApplicationCustomThemeColorsKey, _color.toString());
}

void SettingsManager::setApplicationScaleFactor(qreal _scaleFactor)
{
    d->setSettingsValue(DataStorageLayer::kApplicationScaleFactorKey, _scaleFactor);
}

void SettingsManager::setApplicationUseAutoSave(bool _use)
{
    d->setSettingsValue(DataStorageLayer::kApplicationUseAutoSaveKey, _use);
}

void SettingsManager::setApplicationSaveBackups(bool _save)
{
    d->setSettingsValue(DataStorageLayer::kApplicationSaveBackupsKey, _save);
}

void SettingsManager::setApplicationBackupsFolder(const QString& _path)
{
    d->setSettingsValue(DataStorageLayer::kApplicationBackupsFolderKey, _path);
}

void SettingsManager::setScreenplayEditorDefaultTemplate(const QString& _templateId)
{
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey, _templateId);
    emit screenplayEditorChanged({ DataStorageLayer::kComponentsScreenplayEditorDefaultTemplateKey });
}

void SettingsManager::setScreenplayEditorShowSceneNumber(bool _show, bool _atLeft, bool _atRight)
{
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey, _show);
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey, _atLeft);
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorShowSceneNumberOnLeftKey, _atRight);
    emit screenplayEditorChanged({ DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersKey,
                                   DataStorageLayer::kComponentsScreenplayEditorShowSceneNumbersOnRightKey,
                                   DataStorageLayer::kComponentsScreenplayEditorShowSceneNumberOnLeftKey });
}

void SettingsManager::setScreenplayEditorShowDialogueNumber(bool _show)
{
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumberKey, _show);
    emit screenplayEditorChanged({ DataStorageLayer::kComponentsScreenplayEditorShowDialogueNumberKey });
}

void SettingsManager::setScreenplayEditorHighlightCurrentLine(bool _highlight)
{
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayEditorHighlightCurrentLineKey, _highlight);
    emit screenplayEditorChanged({ DataStorageLayer::kComponentsScreenplayEditorHighlightCurrentLineKey });
}

void SettingsManager::setScreenplayNavigatorShowSceneNumber(bool _show)
{
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneNumberKey, _show);
}

void SettingsManager::setScreenplayNavigatorShowSceneText(bool _show, int _lines)
{
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneTextKey, _show);
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayNavigatorSceneTextLinesKey, _lines);
}

void SettingsManager::setScreenplayDurationType(int _type)
{
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationTypeKey, _type);
}

void SettingsManager::setScreenplayDurationByPageDuration(int _duration)
{
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationByPageDurationKey, _duration);
}

void SettingsManager::setScreenplayDurationByCharactersCharacters(int _characters)
{
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersCharactersKey, _characters);
}

void SettingsManager::setScreenplayDurationByCharactersIncludeSpaces(bool _include)
{
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersIncludeSpacesKey, _include);
}

void SettingsManager::setScreenplayDurationByCharactersDuration(int _duration)
{
    d->setSettingsValue(DataStorageLayer::kComponentsScreenplayDurationByCharactersDurationKey, _duration);
}

}
