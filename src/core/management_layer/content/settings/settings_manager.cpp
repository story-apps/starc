#include "settings_manager.h"

#include <include/custom_events.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <ui/settings/language_dialog.h>
#include <ui/settings/settings_navigator.h>
#include <ui/settings/settings_tool_bar.h>
#include <ui/settings/settings_view.h>
#include <ui/settings/theme_dialog.h>

#include <QEvent>


namespace ManagementLayer
{

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
    view->setApplicationUseSpellChecker(settingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey).toBool());
    view->setApplicationSpellCheckerLanguage(settingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey).toString());
    view->setApplicationTheme(settingsValue(DataStorageLayer::kApplicationThemeKey).toInt());
    view->setApplicationScaleFactor(settingsValue(DataStorageLayer::kApplicationScaleFactorKey).toReal());
    view->setApplicationUseAutoSave(settingsValue(DataStorageLayer::kApplicationUseAutoSaveKey).toBool());
    view->setApplicationSaveBackups(settingsValue(DataStorageLayer::kApplicationSaveBackupsKey).toBool());
    view->setApplicationBackupsFolder(settingsValue(DataStorageLayer::kApplicationBackupsFolderKey).toString());
}


// ****


SettingsManager::SettingsManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{
    d->loadApplicationSettings();
    d->view->installEventFilter(this);

    connect(d->toolBar, &Ui::SettingsToolBar::backPressed, this, &SettingsManager::closeSettingsRequested);

    connect(d->navigator, &Ui::SettingsNavigator::applicationPressed, d->view, &Ui::SettingsView::showApplication);
    connect(d->navigator, &Ui::SettingsNavigator::applicationUserInterfacePressed, d->view, &Ui::SettingsView::showApplicationUserInterface);
    connect(d->navigator, &Ui::SettingsNavigator::applicationSaveAndBackupsPressed, d->view, &Ui::SettingsView::showApplicationSaveAndBackups);
    connect(d->navigator, &Ui::SettingsNavigator::componentsPressed, d->view, &Ui::SettingsView::showComponents);
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
    connect(d->view, &Ui::SettingsView::applicationUseSpellCheckerChanged, this, &SettingsManager::setApplicationUseSpellChecker);
    connect(d->view, &Ui::SettingsView::applicationSpellCheckerLanguageChanged, this, &SettingsManager::setApplicationSpellCheckerLanguage);
    connect(d->view, &Ui::SettingsView::applicationScaleFactorChanged, this, &SettingsManager::setApplicationScaleFactor);
    connect(d->view, &Ui::SettingsView::applicationUseAutoSaveChanged, this, &SettingsManager::setApplicationUseAutoSave);
    connect(d->view, &Ui::SettingsView::applicationSaveBackupsChanged, this, &SettingsManager::setApplicationSaveBackups);
    connect(d->view, &Ui::SettingsView::applicationBackupsFolderChanged, this, &SettingsManager::setApplicationBackupsFolder);

    //
    // Нотификации об изменении параметров
    //
    connect(d->view, &Ui::SettingsView::applicationScaleFactorChanged, this, &SettingsManager::applicationScaleFactorChanged);
    connect(d->view, &Ui::SettingsView::applicationUseAutoSaveChanged, this, &SettingsManager::applicationUseAutoSaveChanged);
    connect(d->view, &Ui::SettingsView::applicationSaveBackupsChanged, this, &SettingsManager::applicationSaveBackupsChanged);
    connect(d->view, &Ui::SettingsView::applicationBackupsFolderChanged, this, &SettingsManager::applicationBackupsFolderChanged);
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

void SettingsManager::setApplicationUseSpellChecker(bool _use)
{
    d->setSettingsValue(DataStorageLayer::kApplicationUseSpellCheckerKey, _use);
}

void SettingsManager::setApplicationSpellCheckerLanguage(const QString& _languageCode)
{
    d->setSettingsValue(DataStorageLayer::kApplicationSpellCheckerLanguageKey, _languageCode);
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

}
