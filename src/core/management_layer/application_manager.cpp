#include "application_manager.h"

#include "content/account/account_manager.h"
#include "content/export/export_manager.h"
#include "content/import/import_manager.h"
#include "content/onboarding/onboarding_manager.h"
#include "content/project/project_manager.h"
#include "content/projects/project.h"
#include "content/projects/projects_manager.h"
#include "content/settings/settings_manager.h"

#ifdef CLOUD_SERVICE_MANAGER
#include <cloud/cloud_service_manager.h>
#endif

#include <business_layer/model/abstract_model.h>

#include <data_layer/database.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <include/custom_events.h>

#include <ui/application_style.h>
#include <ui/application_view.h>
#include <ui/menu_view.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/dialog/standard_dialog.h>

#include <utils/3rd_party/WAF/Animation/Animation.h>
#include <utils/helpers/dialog_helper.h>
#include <utils/tools/backup_builder.h>
#include <utils/tools/run_once.h>

#include <NetworkRequest.h>

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QLocale>
#include <QShortcut>
#include <QSoundEffect>
#include <QStyleFactory>
#include <QtConcurrentRun>
#include <QTimer>
#include <QTranslator>
#include <QUuid>
#include <QVariant>
#include <QWidget>

namespace ManagementLayer {

namespace  {
    /**
     * @brief Состояние приложения
     */
    enum class ApplicationState {
        Initializing,
        ProjectLoading,
        ProjectClosing,
        Working,
        Importing
    };
}

class ApplicationManager::Implementation
{
public:
    explicit Implementation(ApplicationManager* _q);
    ~Implementation();

    /**
     * @brief Получить значение параметра настроек
     */
    QVariant settingsValue(const QString& _key) const;
    QVariantMap settingsValues(const QString& _key) const;

    /**
     * @brief Проверить новую версию
     */
    void checkNewVersion();

    /**
     * @brief Настроить параметры автосохранения
     */
    void configureAutoSave();

    /**
     * @brief Показать контент приложения
     */
    void showContent();

    /**
     * @brief Показать меню приложения
     */
    void showMenu();

    /**
     * @brief Показать личный кабинет
     */
    void showAccount();

    /**
     * @brief Показать страницу проектов
     */
    void showProjects();

    /**
     * @brief Показать страницу проекта
     */
    void showProject();

    /**
     * @brief Показать страницу настроек
     */
    void showSettings();

    /**
     * @brief Показать предыдущий контент
     */
    void showLastContent();

    /**
     * @brief Установить перевод
     */
    void setTranslation(QLocale::Language _language);

    /**
     * @brief Установить тему
     */
    void setTheme(Ui::ApplicationTheme _theme);

    /**
     * @brief Установить цвета кастомной темы
     */
    void setCustomThemeColors(const Ui::DesignSystem::Color& _color);

    /**
     * @brief Установить коэффициент масштабирования
     */
    void setScaleFactor(qreal _scaleFactor);

    //
    // Работа с проектом
    //

    /**
     * @brief Обновить заголовок приложения
     */
    void updateWindowTitle();

    /**
     * @brief Настроить состояние приложения сохранены ли все изменения или нет
     */
    void markChangesSaved(bool _saved);

    /**
     * @brief Сохранить изменения проекта
     */
    void saveChanges();

    /**
     * @brief Если проект был изменён, но не сохранён предложить пользователю сохранить его
     * @param _callback - метод, который будет вызван, если пользователь хочет (Да),
     *        или не хочет (Нет) сохранять и не будет вызван, если пользователь передумал (Отмена)
     */
    void saveIfNeeded(std::function<void()> _callback);

    /**
     * @brief Сохранить проект как другой файл
     */
    void saveAs();

    /**
     * @brief Создать проект
     */
    void createProject();
    void createLocalProject(const QString& _projectName, const QString& _projectPath, const QString& _importFilePath);

    /**
     * @brief Открыть проект по заданному пути
     */
    void openProject();
    void openProject(const QString& _path);

    /**
      * @brief Перейти к редактированию текущего проекта
      */
    void goToEditCurrentProject(const QString& _importFilePath = {});

    /**
     * @brief Закрыть текущий проект
     */
    void closeCurrentProject();

    /**
     * @brief Импортировать проект
     */
    void importProject();

    /**
     * @brief Экспортировать текущий документ
     */
    void exportCurrentDocument();

    //

    /**
     * @brief Если необходимо имитировать звуки печатной машинки в зависимости от нажатой клавиши
     */
    void imitateTypewriterSound(QKeyEvent* _event) const;

    /**
     * @brief Выйти из приложения
     */
    void exit();


    //
    // Данные
    //

    ApplicationManager* q = nullptr;

    /**
     * @brief Интерфейс приложения
     */
    Ui::ApplicationView* applicationView = nullptr;
    Ui::MenuView* menuView = nullptr;
    struct LastContent {
        QWidget* toolBar = nullptr;
        QWidget* navigator = nullptr;
        QWidget* view = nullptr;
    } lastContent;

    /**
     * @brief Менеджеры управляющие конкретными частями приложения
     */
    QScopedPointer<AccountManager> accountManager;
    QScopedPointer<OnboardingManager> onboardingManager;
    QScopedPointer<ProjectsManager> projectsManager;
    QScopedPointer<ProjectManager> projectManager;
    QScopedPointer<ImportManager> importManager;
    QScopedPointer<ExportManager> exportManager;
    QScopedPointer<SettingsManager> settingsManager;
#ifdef CLOUD_SERVICE_MANAGER
    QScopedPointer<CloudServiceManager> cloudServiceManager;
#endif

    /**
     * @brief Таймер автосохранения проекта
     */
    QTimer autosaveTimer;

    /**
     * @brief Состояние приложения в данный момент
     */
    ApplicationState state = ApplicationState::Initializing;

private:
    template<typename Manager>
    void showContent(Manager* _manager);

    template<typename Manager>
    void saveLastContent(Manager* _manager);
};

ApplicationManager::Implementation::Implementation(ApplicationManager* _q)
    : q(_q),
      applicationView(new Ui::ApplicationView),
      menuView(new Ui::MenuView(applicationView)),
      accountManager(new AccountManager(nullptr, applicationView)),
      onboardingManager(new OnboardingManager(nullptr, applicationView)),
      projectsManager(new ProjectsManager(nullptr, applicationView)),
      projectManager(new ProjectManager(nullptr, applicationView)),
      importManager(new ImportManager(nullptr, applicationView)),
      exportManager(new ExportManager(nullptr, applicationView)),
      settingsManager(new SettingsManager(nullptr, applicationView))
#ifdef CLOUD_SERVICE_MANAGER
    , cloudServiceManager(new CloudServiceManager)
#endif
{
    applicationView->setAccountBar(accountManager->accountBar());
}

ApplicationManager::Implementation::~Implementation()
{
    applicationView->disconnect();
    menuView->disconnect();
    accountManager->disconnect();
    onboardingManager->disconnect();
    projectsManager->disconnect();
    projectManager->disconnect();
#ifdef CLOUD_SERVICE_MANAGER
    cloudServiceManager->disconnect();
#endif
}

QVariant ApplicationManager::Implementation::settingsValue(const QString& _key) const
{
    return DataStorageLayer::StorageFacade::settingsStorage()->value(
                _key, DataStorageLayer::SettingsStorage::SettingsPlace::Application);
}

QVariantMap ApplicationManager::Implementation::settingsValues(const QString& _key) const
{
    return DataStorageLayer::StorageFacade::settingsStorage()->values(
                _key, DataStorageLayer::SettingsStorage::SettingsPlace::Application);
}

void ApplicationManager::Implementation::checkNewVersion()
{
    //
    // Сформируем uuid для приложения, по которому будем идентифицировать данного пользователя
    //
    auto applicationUuidValue = settingsValue(DataStorageLayer::kApplicationUuidKey).toUuid();
    if (applicationUuidValue.isNull()) {
        applicationUuidValue = QUuid::createUuid();
        DataStorageLayer::StorageFacade::settingsStorage()->setValue(
                    DataStorageLayer::kApplicationUuidKey,
                    applicationUuidValue,
                    DataStorageLayer::SettingsStorage::SettingsPlace::Application);
    }

    //
    // Построим ссылку, чтобы учитывать запрос на проверку обновлений
    //
    auto loader = new NetworkRequest;
    QObject::connect(loader, &NetworkRequest::finished, loader, &NetworkRequest::deleteLater);
    //
    loader->setRequestMethod(NetworkRequestMethod::Post);
    QJsonObject data;
    data["device_uuid"] = applicationUuidValue.toString();
    data["application_name"] = QApplication::applicationName();
    data["application_version"] = QApplication::applicationVersion();
    data["application_language"] = QLocale::languageToString(QLocale().language());
    data["system_type"] =
#ifdef Q_OS_WIN
            "windows"
#elif defined Q_OS_LINUX
            "linux"
#elif defined Q_OS_MAC
            "mac"
#else
            QSysInfo::kernelType()
#endif
            ;
    data["system_name"] = QSysInfo::prettyProductName();
    data["system_language"] = QLocale::languageToString(QLocale::system().language());
    data["action_name"] = "startup";
    data["action_content"] = QString();
    loader->setRawRequestData(QJsonDocument(data).toJson(), "application/json");
    loader->loadAsync("https://demo.storyapps.dev/telemetry/");
}

void ApplicationManager::Implementation::configureAutoSave()
{
    autosaveTimer.stop();
    autosaveTimer.disconnect();

    if (settingsValue(DataStorageLayer::kApplicationUseAutoSaveKey).toBool()) {
        QObject::connect(&autosaveTimer, &QTimer::timeout, [this] { saveChanges(); });
        autosaveTimer.start(std::chrono::minutes{3});
    }
}

void ApplicationManager::Implementation::showContent()
{
    //
    // Если это первый запуск приложения, то покажем онбординг
    //
    if (settingsValue(DataStorageLayer::kApplicationConfiguredKey).toBool() == false) {
        showContent(onboardingManager.data());
    }
    //
    // В противном случае показываем недавние проекты
    //
    else {
        //
        // Сперва проекты нужно загрузить
        //
        projectsManager->loadProjects();
        //
        // ... а затем уже отобразить
        //
        showProjects();

#ifdef CLOUD_SERVICE_MANAGER
        //
        // Если менеджер облака доступен, отобржаем панель для работы с личным кабинетом
        //
        accountManager->accountBar()->show();
#endif
    }
}

void ApplicationManager::Implementation::showMenu()
{
    menuView->setFocus();
    menuView->setFixedWidth(std::max(menuView->sizeHint().width(),
                                     static_cast<int>(Ui::DesignSystem::drawer().width())));
    WAF::Animation::sideSlideIn(menuView);
}

void ApplicationManager::Implementation::showAccount()
{
    showContent(accountManager.data());
    accountManager->accountBar()->hide();
}

void ApplicationManager::Implementation::showProjects()
{
    menuView->checkProjects();
    showContent(projectsManager.data());
    saveLastContent(projectsManager.data());
}

void ApplicationManager::Implementation::showProject()
{
    menuView->checkProject();
    showContent(projectManager.data());
    saveLastContent(projectManager.data());
}

void ApplicationManager::Implementation::showSettings()
{
    showContent(settingsManager.data());
}

void ApplicationManager::Implementation::showLastContent()
{
    if (lastContent.toolBar == nullptr
        || lastContent.navigator == nullptr
        || lastContent.view == nullptr) {
        return;
    }

    applicationView->showContent(lastContent.toolBar, lastContent.navigator, lastContent.view);
}

void ApplicationManager::Implementation::setTranslation(QLocale::Language _language)
{
    //
    // Определим файл перевода
    //
    const QLocale::Language currentLanguage = _language != QLocale::AnyLanguage
                                        ? _language
                                        : QLocale::system().language();
    QString translation;
    switch (currentLanguage) {
        default:
        case QLocale::English: {
            translation = "en";
            break;
        }

        case QLocale::Azerbaijani: {
            translation = "az";
            break;
        }

        case QLocale::Belarusian: {
            translation = "be";
            break;
        }

        case QLocale::Danish: {
            translation = "da_DK";
            break;
        }

        case QLocale::French: {
            translation = "fr";
            break;
        }

        case QLocale::Galician: {
            translation = "gl_ES";
            break;
        }

        case QLocale::German: {
            translation = "de";
            break;
        }

        case QLocale::Hebrew: {
            translation = "he";
            break;
        }

        case QLocale::Hindi: {
            translation = "hi";
            break;
        }

        case QLocale::Hungarian: {
            translation = "hu";
            break;
        }

        case QLocale::Indonesian: {
            translation = "id";
            break;
        }

        case QLocale::Italian: {
            translation = "it";
            break;
        }

        case QLocale::Persian: {
            translation = "fa";
            break;
        }

        case QLocale::Polish: {
            translation = "pl";
            break;
        }

        case QLocale::Portuguese: {
            translation = "pt_BR";
            break;
        }

        case QLocale::Romanian: {
            translation = "ro_RO";
            break;
        }

        case QLocale::Russian: {
            translation = "ru";
            break;
        }

        case QLocale::Slovenian: {
            translation = "sl";
            break;
        }

        case QLocale::Spanish: {
            translation = "es";
            break;
        }

        case QLocale::Turkish: {
            translation = "tr";
            break;
        }

        case QLocale::Ukrainian: {
            translation = "uk";
            break;
        }
    }

    QLocale::setDefault(QLocale(currentLanguage));

    //
    // Подключим файл переводов программы
    //
    static QTranslator* appTranslator = [] {
        //
        // ... небольшой workaround для того, чтобы при запуске приложения кинуть событие о смене языка
        //
        QTranslator* translator = new QTranslator;
        QApplication::installTranslator(translator);
        return translator;
    } ();
    //
    QApplication::removeTranslator(appTranslator);
    if (!translation.isEmpty()) {
        appTranslator->load(":/translations/translation_" + translation + ".qm");
        QApplication::installTranslator(appTranslator);
    }

    //
    // Для языков, которые пишутся справа-налево настроим соответствующее выравнивание интерфейса
    //
    if (currentLanguage == QLocale::Persian
        || currentLanguage == QLocale::Hebrew) {
        QApplication::setLayoutDirection(Qt::RightToLeft);
        QApplication::postEvent(q, new DesignSystemChangeEvent);
    } else {
        QApplication::setLayoutDirection(Qt::LeftToRight);
        QApplication::postEvent(q, new DesignSystemChangeEvent);
    }

    //
    // Настроим дизайн систему так, чтобы она использовала шрифт подходящий для используемого языка
    //
    Ui::DesignSystem::updateLanguage();
}

void ApplicationManager::Implementation::setTheme(Ui::ApplicationTheme _theme)
{
    if (state == ApplicationState::Working) {
        WAF::Animation::circleTransparentIn(applicationView, QCursor::pos(), applicationView->grab());
    }
    Ui::DesignSystem::setTheme(_theme);
    QApplication::postEvent(q, new DesignSystemChangeEvent);
}

void ApplicationManager::Implementation::setCustomThemeColors(const Ui::DesignSystem::Color& _color)
{
    if (Ui::DesignSystem::theme() != Ui::ApplicationTheme::Custom) {
        return;
    }

    if (state == ApplicationState::Working) {
        WAF::Animation::circleTransparentIn(applicationView, QCursor::pos(), applicationView->grab());
    }
    Ui::DesignSystem::setColor(_color);
    QApplication::postEvent(q, new DesignSystemChangeEvent);
}

void ApplicationManager::Implementation::setScaleFactor(qreal _scaleFactor)
{
    Ui::DesignSystem::setScaleFactor(_scaleFactor);
    QApplication::postEvent(q, new DesignSystemChangeEvent);
}

void ApplicationManager::Implementation::updateWindowTitle()
{
    if (!projectsManager->currentProject().isValid()) {
        applicationView->setWindowTitle("Story Architect");
        return;
    }

    applicationView->setWindowTitle(
                QString("[*]%1 - Story Architect")
                .arg(projectsManager->currentProject().name()));

    if (applicationView->isWindowModified()) {
        markChangesSaved(false);
    }
}

void ApplicationManager::Implementation::markChangesSaved(bool _saved)
{
    const QString suffix = QApplication::translate("ManagementLayer::ApplicationManager", " - changed");
    if (_saved
        && applicationView->windowTitle().endsWith(suffix)) {
        applicationView->setWindowTitle(applicationView->windowTitle().remove(suffix));
    } else if (!_saved
               && !applicationView->windowTitle().endsWith(suffix)) {
        applicationView->setWindowTitle(applicationView->windowTitle() + suffix);
    }

    applicationView->setWindowModified(!_saved);
    menuView->markChangesSaved(_saved);
}

void ApplicationManager::Implementation::saveChanges()
{
    //
    // Избегаем рекурсии
    //
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    //
    // Сохраняем только, если приложение находится в рабочем состоянии
    //
    if (state != ApplicationState::Working) {
        return;
    }

    //
    // Сохраняем только, если есть какие-либо изменения
    //
    if (!applicationView->isWindowModified()) {
        return;
    }

    //
    // Управляющие должны сохранить все изменения
    //
    DatabaseLayer::Database::transaction();
    projectsManager->saveChanges();
    projectManager->saveChanges();
    DatabaseLayer::Database::commit();

#ifdef CLOUD_SERVICE_MANAGER
    //
    // Для проекта из облака синхронизируем данные
    //
    if (projectsManager->currentProject().isRemote()) {
        cloudServiceManager->saveChanges();
    }
#endif

    //
    // Если произошла ошибка сохранения, то делаем дополнительные проверки и работаем с пользователем
    //
    if (DatabaseLayer::Database::hasError()) {
        //
        // Если файл, в который мы пробуем сохранять изменения существует
        //
        if (QFile::exists(DatabaseLayer::Database::currentFile())) {
            //
            // ... то у нас случилась какая-то внутренняя ошибка базы данных
            //
            StandardDialog::information(
                        applicationView, tr("Saving error"),
                        tr("Changes can't be written. There is an internal database error: \"%1\" "
                           "Please check, if your file exists and if you have permission to write.")
                        .arg(DatabaseLayer::Database::lastError()));

            //
            // TODO: пока хер знает, как реагировать на данную проблему...
            //       нужны реальные кейсы и пробовать что-то предпринимать
            //
        }
        //
        // Файла с базой данных не найдено
        //
        else {
            //
            // ... возможно файл был на флешке, а она отошла, или файл был переименован во время работы программы
            //
            StandardDialog::information(
                        applicationView, tr("Saving error"),
                        tr("Changes can't be written because the story located at \"%1\" doesn't exist. "
                           "Please move the file back and retry saving.")
                        .arg(DatabaseLayer::Database::currentFile()));
        }
        return;
    }


    //
    // Если изменения сохранились без ошибок, то изменим статус окна на сохранение изменений
    //
    markChangesSaved(true);

    //
    // И, если необходимо создадим резервную копию закрываемого файла
    //
    if (settingsValue(DataStorageLayer::kApplicationSaveBackupsKey).toBool()) {
        QString baseBackupName;
        const auto& currentProject = projectsManager->currentProject();
        if (currentProject.isRemote()) {
            //
            // Для удаленных проектов имя бекапа - имя проекта + id проекта
            // В случае, если имя удаленного проекта изменилось, то бэкапы со старым именем останутся навсегда
            //
            baseBackupName = QString("%1 [%2]").arg(currentProject.name()).arg(currentProject.id());
        }
        QtConcurrent::run(&BackupBuilder::save,
                          projectsManager->currentProject().path(),
                          settingsValue(DataStorageLayer::kApplicationBackupsFolderKey).toString(),
                          baseBackupName);
    }
}

void ApplicationManager::Implementation::saveIfNeeded(std::function<void()> _callback)
{
    if (!applicationView->isWindowModified()) {
        _callback();
        return;
    }

    const int kCancelButtonId = 0;
    const int kNoButtonId = 1;
    const int kYesButtonId = 2;
    auto dialog = new Dialog(applicationView);
    dialog->showDialog({},
                       tr("Project was modified. Save changes?"),
                       {{ kCancelButtonId, tr("Cancel"), Dialog::RejectButton },
                        { kNoButtonId, tr("Don't save"), Dialog::NormalButton },
                        { kYesButtonId, tr("Save"), Dialog::AcceptButton }});
    QObject::connect(dialog, &Dialog::finished,
                     [this, _callback, kCancelButtonId, kNoButtonId, dialog] (const Dialog::ButtonInfo& _buttonInfo)
    {
        dialog->hideDialog();

        //
        // Пользователь передумал сохранять
        //
        if (_buttonInfo.id == kCancelButtonId) {
            return;
        }

        //
        // Пользователь не хочет сохранять изменения
        //
        if (_buttonInfo.id == kNoButtonId) {
            markChangesSaved(true);
        }
        //
        // ... пользователь хочет сохранить изменения перед следующим действием
        //
        else {
            saveChanges();
        }

        _callback();
    });
    QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);

    QApplication::alert(applicationView);
}

void ApplicationManager::Implementation::saveAs()
{
    //
    // Изначально высвечивается текущее имя проекта
    //
    const auto& currentProject = projectsManager->currentProject();
    QString projectPath = currentProject.path();
    if (currentProject.isRemote()) {
        //
        // Для удаленных проектов используем имя проекта + id проекта
        // и сохраняем в папку вновь создаваемых проектов
        //
        const auto projectsFolderPath
                = DataStorageLayer::StorageFacade::settingsStorage()->value(
                      DataStorageLayer::kProjectSaveFolderKey,
                      DataStorageLayer::SettingsStorage::SettingsPlace::Application)
                  .toString();
        projectPath = projectsFolderPath + QDir::separator()
                      + QString("%1 [%2]%3").arg(currentProject.name())
                                            .arg(currentProject.id())
                                            .arg(Project::extension());
    }

    //
    // Получим имя файла для сохранения
    //
    QString saveAsProjectFilePath
            = QFileDialog::getSaveFileName(applicationView,
                                           tr("Choose file to save story"),
                                           projectPath,
                                           DialogHelper::starcProjectFilter());
    if (saveAsProjectFilePath.isEmpty()) {
        return;
    }

    //
    // Если файл выбран
    //

    //
    // Установим расширение, если не задано
    //
    if (!saveAsProjectFilePath.endsWith(Project::extension())) {
        saveAsProjectFilePath.append(Project::extension());
    }

    //
    // Если пользователь указал тот же путь, ничего не делаем
    //
    if (saveAsProjectFilePath == currentProject.path()) {
        return;
    }

    //
    // Cохраняем в новый файл
    //
    // ... если файл существовал, удалим его для удаления данных в нём
    //
    if (QFile::exists(saveAsProjectFilePath)) {
        QFile::remove(saveAsProjectFilePath);
    }
    //
    // ... скопируем текущую базу в указанный файл
    //
    const auto isCopied = QFile::copy(currentProject.path(), saveAsProjectFilePath);
    if (!isCopied) {
        StandardDialog::information(applicationView, tr("Saving error"),
            tr("Can't save the story to the file %1. Please check permissions and retry.")
                                    .arg(saveAsProjectFilePath));
        return;
    }

    //
    // Откроем копию текущего проекта
    //
    openProject(saveAsProjectFilePath);
}

void ApplicationManager::Implementation::createProject()
{
    auto callback = [this] { projectsManager->createProject(); };
    saveIfNeeded(callback);
}

void ApplicationManager::Implementation::createLocalProject(const QString& _projectName,
    const QString& _projectPath, const QString& _importFilePath)
{
    if (_projectPath.isEmpty()) {
        return;
    }

    //
    // Закроем текущий проект
    //
    closeCurrentProject();

    //
    // Папки, в которую пользователь хочет писать может и не быть,
    // создадим на всякий случай, чтобы его не мучать
    //
    QDir::root().mkpath(QFileInfo(_projectPath).absolutePath());

    //
    // Проверяем, можем ли мы писать в выбранный файл
    //
    QFile file(_projectPath);
    const bool canWrite = file.open(QIODevice::WriteOnly);
    file.close();

    //
    // Если невозможно записать в файл, предупреждаем пользователя и отваливаемся
    //
    if (!canWrite) {
        const QFileInfo fileInfo(_projectPath);
        QString errorMessage;
        if (!fileInfo.dir().exists()) {
            errorMessage =
                tr("You tried to create a project in nonexistent folder %1. "
                   "Please, choose another location for the new project.")
                .arg(fileInfo.dir().absolutePath());
        } else if (fileInfo.exists()) {
            errorMessage =
                tr("The file can't be written. Looks like it is opened by another application. "
                   "Please close it and retry to create a new project.");
        } else {
            errorMessage =
                tr("The file can't be written. Please, check and give permissions to the app "
                   "to write into the selected folder, or choose another folder for saving a new project.");
        }
        StandardDialog::information(applicationView, tr("Create project error"), errorMessage);
        return;
    }

    //
    // Если возможна запись в файл
    //
    // ... создаём новую базу данных в файле и делаем её текущим проектом
    //
    projectsManager->setCurrentProject(_projectPath);
    projectsManager->setCurrentProjectName(_projectName);
    //
    // ... сохраняем новый проект в списке недавних
    //
    projectsManager->saveProjects();
    //
    // ... перейдём к редактированию
    //
    goToEditCurrentProject(_importFilePath);
}

void ApplicationManager::Implementation::openProject()
{
    auto callback = [this] { projectsManager->openProject(); };
    saveIfNeeded(callback);
}

void ApplicationManager::Implementation::openProject(const QString& _path)
{
    if (_path.isEmpty()) {
        return;
    }

    if (!QFileInfo::exists(_path)) {
        projectsManager->hideProject(_path);
        return;
    }

    if (projectsManager->currentProject().path() == _path) {
        showProject();
        return;
    }

    //
    // ... закроем текущий проект
    //
    closeCurrentProject();

    //
    // ... переключаемся на работу с выбранным файлом
    //
    projectsManager->setCurrentProject(_path);

    //
    // ... перейдём к редактированию
    //
    goToEditCurrentProject();
}

void ApplicationManager::Implementation::goToEditCurrentProject(const QString& _importFilePath)
{
    state = ApplicationState::ProjectLoading;

    //
    // Установим заголовок
    //
    updateWindowTitle();

    //
    // Настроим меню
    //
    menuView->setProjectTitle(projectsManager->currentProject().name());
    menuView->setProjectActionsVisible(true);

    //
    // Загрузим данные текущего проекта
    //
    projectManager->loadCurrentProject(projectsManager->currentProject().name(),
                                       projectsManager->currentProject().path());

    //
    // При необходимости импортируем данные из заданного файла
    //
    if (!_importFilePath.isEmpty()) {
        importManager->import(_importFilePath);
    }

    //
    // Отобразить страницу самого проекта
    //
    showProject();

    state = ApplicationState::Working;
}

void ApplicationManager::Implementation::closeCurrentProject()
{
    if (!projectsManager->currentProject().isValid()) {
        return;
    }

    state = ApplicationState::ProjectClosing;

    //
    // Порядок важен
    //
    projectManager->closeCurrentProject(projectsManager->currentProject().path());
    projectsManager->closeCurrentProject();

    state = ApplicationState::Working;
}

void ApplicationManager::Implementation::importProject()
{
    importManager->import();
}

void ApplicationManager::Implementation::exportCurrentDocument()
{
    exportManager->exportDocument(projectManager->currentModel());
}

void ApplicationManager::Implementation::imitateTypewriterSound(QKeyEvent* _event) const
{
    //
    // Обрабатываем событие только в случае, если в виджет в фокусе можно вводить текст
    //
    if (QApplication::focusWidget()
        && !QApplication::focusWidget()->testAttribute(Qt::WA_InputMethodEnabled)) {
        return;
    }
    //
    // ... и если опция озвучивания печати включена
    //
    const auto keyboardSoundEnabled
            = settingsValue(DataStorageLayer::kApplicationUseTypewriterSoundKey).toBool();
    if (!keyboardSoundEnabled) {
        return;
    }

    auto makeSound = [this] (const QString& path) {
        QSoundEffect* sound = new QSoundEffect(applicationView);
        sound->setSource(QUrl::fromLocalFile(path));
        return sound;
    };
    static auto s_returnSound = makeSound(":/audio/return");
    static auto s_spaceSound = makeSound(":/audio/space");
    static auto s_deleteSound = makeSound(":/audio/backspace");
    static QVector<QSoundEffect*> s_keySounds = { makeSound(":/audio/key-01"),
                                                  makeSound(":/audio/key-02"),
                                                  makeSound(":/audio/key-03"),
                                                  makeSound(":/audio/key-04") };
    switch (_event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter: {
            s_returnSound->play();
            break;
        }

        case Qt::Key_Space: {
            s_spaceSound->play();
            break;
        }

        case Qt::Key_Backspace:
        case Qt::Key_Delete: {
            s_deleteSound->play();
            break;
        }

        default: {
            if (_event->text().isEmpty()) {
                break;
            }

            const int firstSoundId = 0;
            const int maxSoundId = 3;
            static int lastSoundId = firstSoundId;
            if (lastSoundId > maxSoundId) {
                lastSoundId = firstSoundId;
            }
            s_keySounds[lastSoundId++]->play();
            break;
        }
    }
}

void ApplicationManager::Implementation::exit()
{
    //
    // Закрываем текущий открытый проект
    //
    closeCurrentProject();

    //
    // Сохраняем состояние приложения
    //
    DataStorageLayer::StorageFacade::settingsStorage()->setValues(
                DataStorageLayer::kApplicationViewStateKey,
                applicationView->saveState(),
                DataStorageLayer::SettingsStorage::SettingsPlace::Application);

    //
    // Сохраним расположение проектов
    //
    projectsManager->saveProjects();

    //
    // Выходим
    //
    QApplication::processEvents();
    QApplication::quit();
}

template<typename Manager>
void ApplicationManager::Implementation::showContent(Manager* _manager)
{
    applicationView->showContent(_manager->toolBar(),
                                 _manager->navigator(),
                                 _manager->view());
}

template<typename Manager>
void ApplicationManager::Implementation::saveLastContent(Manager* _manager)
{
    lastContent.toolBar = _manager->toolBar();
    lastContent.navigator = _manager->navigator();
    lastContent.view = _manager->view();
}


// ****


ApplicationManager::ApplicationManager(QObject* _parent)
    : QObject(_parent),
      IApplicationManager()
{
    QApplication::setApplicationVersion("0.0.7");

    QApplication::setStyle(new ApplicationStyle(QStyleFactory::create("Fusion")));

    //
    // Загрузим шрифты в базу шрифтов программы, если их там ещё нет
    //
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/fonts/materialdesignicons");
    fontDatabase.addApplicationFont(":/fonts/roboto-black");
    fontDatabase.addApplicationFont(":/fonts/roboto-bold");
    fontDatabase.addApplicationFont(":/fonts/roboto-medium");
    fontDatabase.addApplicationFont(":/fonts/roboto-regular");
    fontDatabase.addApplicationFont(":/fonts/roboto-thin");
    fontDatabase.addApplicationFont(":/fonts/noto-sans");
    fontDatabase.addApplicationFont(":/fonts/noto-sans-bold");
    fontDatabase.addApplicationFont(":/fonts/noto-sans-bold-italic");
    fontDatabase.addApplicationFont(":/fonts/noto-sans-italic");
    fontDatabase.addApplicationFont(":/fonts/courier-new");
    fontDatabase.addApplicationFont(":/fonts/courier-new-bold");
    fontDatabase.addApplicationFont(":/fonts/courier-new-italic");
    fontDatabase.addApplicationFont(":/fonts/courier-new-bold-italic");
    fontDatabase.addApplicationFont(":/fonts/courier-prime");
    fontDatabase.addApplicationFont(":/fonts/courier-prime-bold");
    fontDatabase.addApplicationFont(":/fonts/courier-prime-italic");
    fontDatabase.addApplicationFont(":/fonts/courier-prime-bold-italic");
    fontDatabase.addApplicationFont(":/fonts/muktamalar-bold");
    fontDatabase.addApplicationFont(":/fonts/muktamalar-regular");

    //
    // Инициилизируем данные после подгрузки шрифтов, чтобы они сразу подхватились системой
    //
    d.reset(new Implementation(this));

    initConnections();
}

ApplicationManager::~ApplicationManager() = default;

void ApplicationManager::exec(const QString& _fileToOpenPath)
{
    //
    // Самое главное - настроить заголовок!
    //
    d->updateWindowTitle();

    //
    // Установим размер экрана по-умолчанию, на случай, если это первый запуск
    //
    d->applicationView->resize(1024, 640);
    //
    // ... затем пробуем загрузить геометрию и состояние приложения
    //
    d->setTranslation(d->settingsValue(DataStorageLayer::kApplicationLanguagedKey).value<QLocale::Language>());
    d->setTheme(static_cast<Ui::ApplicationTheme>(d->settingsValue(DataStorageLayer::kApplicationThemeKey).toInt()));
    d->setCustomThemeColors(Ui::DesignSystem::Color(d->settingsValue(DataStorageLayer::kApplicationCustomThemeColorsKey).toString()));
    d->setScaleFactor(d->settingsValue(DataStorageLayer::kApplicationScaleFactorKey).toReal());
    d->applicationView->restoreState(d->settingsValues(DataStorageLayer::kApplicationViewStateKey));

    //
    // Покажем интерфейс
    //
    d->applicationView->show();

    //
    // Осуществляем остальную настройку и показываем содержимое, после того, как на экране
    // отобразится приложение, чтобы у пользователя возник эффект моментального запуска
    //
    QTimer::singleShot(0, this, [this, _fileToOpenPath] {
        //
        // Настройка
        //
        d->configureAutoSave();

        //
        // Отображение
        //
        d->showContent();

#ifdef CLOUD_SERVICE_MANAGER
        //
        // Запуск облачного сервиса
        //
        d->cloudServiceManager->start();
#endif

        //
        // Открыть заданный проект
        //
        openProject(_fileToOpenPath);

        //
        // Отправим запрос в статистику
        //
        d->checkNewVersion();

        //
        // Переводим состояние приложение в рабочий режим
        //
        d->state = ApplicationState::Working;
    });
}

void ApplicationManager::openProject(const QString& _path)
{
    d->openProject(_path);
}

bool ApplicationManager::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
        case static_cast<QEvent::Type>(EventType::IdleEvent): {
            //
            // Сохраняем только если пользователь желает делать это автоматически
            //
            if (d->autosaveTimer.isActive()) {
                d->saveChanges();
            }

            _event->accept();
            return true;
        }

        case static_cast<QEvent::Type>(EventType::DesignSystemChangeEvent): {
            //
            // Уведомляем все виджеты о том, что сменилась дизайн система
            //
            for (auto widget : d->applicationView->findChildren<QWidget*>()) {
                QApplication::sendEvent(widget, _event);
            }
            QApplication::sendEvent(d->applicationView, _event);

            _event->accept();
            return true;
        }

        case QEvent::KeyPress: {
            const auto keyEvent = static_cast<QKeyEvent*>(_event);

            //
            // Музицируем
            //
            d->imitateTypewriterSound(keyEvent);

            return false;
        }

        default: {
            return QObject::event(_event);
        }
    }
}

void ApplicationManager::initConnections()
{
    //
    // Горячие клавиши
    //
    QShortcut* saveShortcut = new QShortcut(QKeySequence::Save, d->applicationView);
    saveShortcut->setContext(Qt::ApplicationShortcut);
    connect(saveShortcut, &QShortcut::activated, this, [this] { d->saveChanges(); });
    //
    QShortcut* importShortcut = new QShortcut(QKeySequence("Alt+I"), d->applicationView);
    importShortcut->setContext(Qt::ApplicationShortcut);
    connect(importShortcut, &QShortcut::activated, this, [this] { d->importProject(); });
    //
    QShortcut* exportShortcut = new QShortcut(QKeySequence("Alt+E"), d->applicationView);
    exportShortcut->setContext(Qt::ApplicationShortcut);
    connect(exportShortcut, &QShortcut::activated, this, [this] { d->exportCurrentDocument(); });

    //
    // Представление приложения
    //
    connect(d->applicationView, &Ui::ApplicationView::closeRequested, this,
            [this]
    {
        auto callback = [this] { d->exit(); };
        d->saveIfNeeded(callback);
    });

    //
    // Представление меню
    //
    connect(d->menuView, &Ui::MenuView::projectsPressed, this, [this] { d->showProjects(); });
    connect(d->menuView, &Ui::MenuView::createProjectPressed, this, [this] { d->createProject(); });
    connect(d->menuView, &Ui::MenuView::openProjectPressed, this, [this] { d->openProject(); });
    connect(d->menuView, &Ui::MenuView::projectPressed, this, [this] { d->showProject(); });
    connect(d->menuView, &Ui::MenuView::saveProjectChangesPressed, this, [this] { d->saveChanges(); });
    connect(d->menuView, &Ui::MenuView::saveProjectAsPressed, this, [this] {
        auto callback = [this] { d->saveAs(); };
        d->saveIfNeeded(callback);
    });
    connect(d->menuView, &Ui::MenuView::importPressed, this, [this] { d->importProject(); });
    connect(d->menuView, &Ui::MenuView::exportCurrentDocumentPressed, this, [this] { d->exportCurrentDocument(); });
    connect(d->menuView, &Ui::MenuView::settingsPressed, this, [this] { d->showSettings(); });

    //
    // Менеджер посадки
    //
    connect(d->onboardingManager.data(), &OnboardingManager::languageChanged, this,
            [this] (QLocale::Language _language) { d->setTranslation(_language); });
    connect(d->onboardingManager.data(), &OnboardingManager::themeChanged, this,
            [this] (Ui::ApplicationTheme _theme) { d->setTheme(_theme); });
    connect(d->onboardingManager.data(), &OnboardingManager::scaleFactorChanged, this,
            [this] (qreal _scaleFactor)
    {
        d->setScaleFactor(_scaleFactor);
        d->settingsManager->updateScaleFactor();
    });
    connect(d->onboardingManager.data(), &OnboardingManager::finished, this,
            [this]
    {
        auto setSettingsValue = [] (const QString& _key, const QVariant& _value) {
            DataStorageLayer::StorageFacade::settingsStorage()->setValue(
                        _key,
                        _value,
                        DataStorageLayer::SettingsStorage::SettingsPlace::Application);
        };
        setSettingsValue(DataStorageLayer::kApplicationConfiguredKey, true);
        setSettingsValue(DataStorageLayer::kApplicationLanguagedKey, QLocale::system().language());
        setSettingsValue(DataStorageLayer::kApplicationThemeKey, static_cast<int>(Ui::DesignSystem::theme()));
        setSettingsValue(DataStorageLayer::kApplicationScaleFactorKey, Ui::DesignSystem::scaleFactor());
        d->showContent();
    });

    //
    // Менеджер аккаунта
    //
    connect(d->accountManager.data(), &AccountManager::showAccountRequested, this, [this] { d->showAccount(); });
    connect(d->accountManager.data(), &AccountManager::closeAccountRequested, this, [this] {
        d->accountManager->accountBar()->show();
        d->showLastContent();
    });
    connect(d->accountManager.data(), &AccountManager::cloudProjectsCreationAvailabilityChanged,
            d->projectsManager.data(), &ProjectsManager::setProjectsInCloudCanBeCreated);

    //
    // Менеджер проектов
    //
    connect(d->projectsManager.data(), &ProjectsManager::menuRequested, this,
            [this] { d->showMenu(); });
    connect(d->projectsManager.data(), &ProjectsManager::createProjectRequested, this,
            [this] { d->createProject(); });
    connect(d->projectsManager.data(), &ProjectsManager::createLocalProjectRequested, this,
            [this] (const QString& _projectName,const QString& _projectPath, const QString& _importFilePath)
    {
        d->createLocalProject(_projectName, _projectPath, _importFilePath);
    });
    connect(d->projectsManager.data(), &ProjectsManager::openProjectRequested, this,
            [this] { d->openProject(); });
    connect(d->projectsManager.data(), &ProjectsManager::openChoosedProjectRequested, this,
            [this] (const QString& _path)
    {
        if (d->projectsManager->currentProject().path() == _path) {
            d->showProject();
            return;
        }

        auto callback = [this, _path] { openProject(_path); };
        d->saveIfNeeded(callback);
    });

    //
    // Менеджер проекта
    //
    connect(d->projectManager.data(), &ProjectManager::menuRequested, this,
            [this] { d->showMenu(); });
    connect(d->projectManager.data(), &ProjectManager::contentsChanged, this,
            [this] { d->markChangesSaved(false); });
    connect(d->projectManager.data(), &ProjectManager::projectNameChanged, this,
            [this] (const QString& _name)
    {
        d->projectsManager->setCurrentProjectName(_name);
        d->menuView->setProjectTitle(_name);
        d->updateWindowTitle();
    });
    connect(d->projectManager.data(), &ProjectManager::projectLoglineChanged,
            d->projectsManager.data(), &ProjectsManager::setCurrentProjectLogline);
    connect(d->projectManager.data(), &ProjectManager::projectCoverChanged,
            d->projectsManager.data(), &ProjectsManager::setCurrentProjectCover);
    connect(d->projectManager.data(), &ProjectManager::currentModelChanged, this,
            [this] (BusinessLayer::AbstractModel* _model) {
        d->menuView->setCurrentDocumentExportAvailable(d->exportManager->canExportDocument(_model));
    });

    //
    // Менеджер импорта
    //
    connect(d->importManager.data(), &ImportManager::characterImported,
            d->projectManager.data(), &ProjectManager::addCharacter);
    connect(d->importManager.data(), &ImportManager::locationImported,
            d->projectManager.data(), &ProjectManager::addLocation);
    connect(d->importManager.data(), &ImportManager::screenplayImported,
            d->projectManager.data(), &ProjectManager::addScreenplay);

    //
    // Менеджер настроек
    //
    connect(d->settingsManager.data(), &SettingsManager::closeSettingsRequested, this,
            [this] { d->showLastContent(); });
    connect(d->settingsManager.data(), &SettingsManager::applicationLanguageChanged, this,
            [this] (QLocale::Language _language) { d->setTranslation(_language); });
    connect(d->settingsManager.data(), &SettingsManager::applicationUseSpellCheckerChanged, this,
            [this] { d->projectManager->reconfigureAll(); });
    connect(d->settingsManager.data(), &SettingsManager::applicationSpellCheckerLanguageChanged, this,
            [this] { d->projectManager->reconfigureAll(); });
    connect(d->settingsManager.data(), &SettingsManager::applicationThemeChanged, this,
            [this] (Ui::ApplicationTheme _theme)
    {
        d->setTheme(_theme);
        //
        // ... если применяется кастомная тема, то нужно загрузить её цвета
        //
        if (_theme == Ui::ApplicationTheme::Custom) {
            d->setCustomThemeColors(Ui::DesignSystem::Color(d->settingsValue(DataStorageLayer::kApplicationCustomThemeColorsKey).toString()));
        }
    });
    connect(d->settingsManager.data(), &SettingsManager::applicationCustomThemeColorsChanged, this,
            [this] (const Ui::DesignSystem::Color& _color) { d->setCustomThemeColors(_color); });
    connect(d->settingsManager.data(), &SettingsManager::applicationScaleFactorChanged, this,
            [this] (qreal _scaleFactor) { d->setScaleFactor(_scaleFactor); });
    connect(d->settingsManager.data(), &SettingsManager::applicationUseAutoSaveChanged, this,
            [this] { d->configureAutoSave(); });
    //
    connect(d->settingsManager.data(), &SettingsManager::screenplayEditorChanged, this,
            [this] (const QStringList& _changedSettingsKeys) {
        d->projectManager->reconfigureScreenplayEditor(_changedSettingsKeys);
    });
    connect(d->settingsManager.data(), &SettingsManager::screenplayNavigatorChanged, this,
            [this] { d->projectManager->reconfigureScreenplayNavigator(); });
    connect(d->settingsManager.data(), &SettingsManager::screenplayDurationChanged, this,
            [this] { d->projectManager->reconfigureScreenplayDuration(); });

#ifdef CLOUD_SERVICE_MANAGER
    //
    // Менеджер облака
    //
    // ... поймали/потеряли связь
    //
    connect(d->cloudServiceManager.data(), &CloudServiceManager::connected,
            d->accountManager.data(), &AccountManager::notifyConnected);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::disconnected,
            d->accountManager.data(), &AccountManager::notifyDisconnected);
    //
    // ... авторизация/регистрация
    //
    {
        //
        // проверка регистрация или вход
        //
        connect(d->accountManager.data(), &AccountManager::emailEntered,
                d->cloudServiceManager.data(), &CloudServiceManager::canLogin);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::registrationAllowed,
                d->accountManager.data(), &AccountManager::allowRegistration);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::loginAllowed,
                d->accountManager.data(), &AccountManager::allowLogin);

        //
        // регистрация
        //
        connect(d->accountManager.data(), &AccountManager::registrationRequested,
                d->cloudServiceManager.data(), &CloudServiceManager::registerAccount);
        connect(d->accountManager.data(), &AccountManager::registrationConfirmationCodeEntered,
                d->cloudServiceManager.data(), &CloudServiceManager::confirmRegistration);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::registrationConfiramtionCodeSended,
                d->accountManager.data(), &AccountManager::prepareToEnterRegistrationConfirmationCode);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::registrationConfirmationError,
                d->accountManager.data(), &AccountManager::setRegistrationConfirmationError);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::registrationCompleted,
                d->accountManager.data(), &AccountManager::login);

        //
        // восстановление пароля
        //
        connect(d->accountManager.data(), &AccountManager::restorePasswordRequested,
                d->cloudServiceManager.data(), &CloudServiceManager::restorePassword);
        connect(d->accountManager.data(), &AccountManager::passwordRestoringConfirmationCodeEntered,
                d->cloudServiceManager.data(), &CloudServiceManager::confirmPasswordRestoring);
        connect(d->accountManager.data(), &AccountManager::changePasswordRequested,
                d->cloudServiceManager.data(), &CloudServiceManager::changePassword);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::passwordRestoringConfirmationCodeSended,
                d->accountManager.data(), &AccountManager::prepareToEnterRestorePasswordConfirmationCode);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::passwordRestoringConfirmationSucceed,
                d->accountManager.data(), &AccountManager::allowChangePassword);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::registrationConfirmationError,
                d->accountManager.data(), &AccountManager::setRestorePasswordConfirmationError);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::passwordChanged,
                d->accountManager.data(), &AccountManager::login);

        //
        // авторизация
        //
        connect(d->accountManager.data(), &AccountManager::loginRequested,
                d->cloudServiceManager.data(), &CloudServiceManager::login);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::loginPasswordError,
                d->accountManager.data(), &AccountManager::setLoginPasswordError);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::loginCompleted,
                d->accountManager.data(), &AccountManager::completeLogin);

        //
        // Выход из аккаунта
        //
        connect(d->accountManager.data(), &AccountManager::logoutRequested,
                d->cloudServiceManager.data(), &CloudServiceManager::logout);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::logoutCompleted,
                d->accountManager.data(), &AccountManager::completeLogout);
    }

    //
    // Получены параметры об аккаунте
    //
    connect(d->cloudServiceManager.data(), &CloudServiceManager::accountParametersLoaded,
            d->accountManager.data(), &AccountManager::setAccountParameters);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::paymentInfoLoaded,
            d->accountManager.data(), &AccountManager::setPaymentInfo);

    //
    // Оплата услуг
    //
    connect(d->accountManager.data(), &AccountManager::renewSubscriptionRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::renewSubscription);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::subscriptionRenewed,
            d->accountManager.data(), &AccountManager::setSubscriptionEnd);

    //
    // Изменение параметров аккаунта
    //
    connect(d->accountManager.data(), &AccountManager::changeUserNameRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::setUserName);
    connect(d->accountManager.data(), &AccountManager::changeReceiveEmailNotificationsRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::setNeedNotify);
    connect(d->accountManager.data(), &AccountManager::changeAvatarRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::setAvatar);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::userNameChanged,
            d->accountManager.data(), &AccountManager::setUserName);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::receiveEmailNotificationsChanged,
            d->accountManager.data(), &AccountManager::setReceiveEmailNotifications);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::avatarChanged,
            d->accountManager.data(), qOverload<const QByteArray&>(&AccountManager::setAvatar));
#endif
}

} // namespace ManagementLayer
