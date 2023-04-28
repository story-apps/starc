#pragma once

#include <QObject>


namespace BusinessLayer {
class AbstractModel;
class StructureModelItem;
} // namespace BusinessLayer

namespace Domain {
class DocumentChangeObject;
class DocumentObject;
enum class DocumentObjectType;
struct CursorInfo;
struct DocumentInfo;
} // namespace Domain

namespace ManagementLayer {

enum class DocumentEditingMode;
class Project;
class PluginsBuilder;

/**
 * @brief Управляющий открытым проектом
 */
class ProjectManager : public QObject
{
    Q_OBJECT

public:
    ProjectManager(QObject* _parent, QWidget* _parentWidget, const PluginsBuilder& _pluginsBuilder);
    ~ProjectManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

    /**
     * @brief Показать/скрыть наличие непрочитанных комментариев
     */
    void setHasUnreadNotifications(bool _hasUnreadNotifications);

    /**
     * @brief Активировать полноэкранный режим
     */
    void toggleFullScreen(bool _isFullScreen);

    /**
     * @brief Перенастроить плагины
     */
    void reconfigureAll();
    void reconfigureSimpleTextEditor(const QStringList& _changedSettingsKeys);
    void reconfigureSimpleTextNavigator();
    void reconfigureScreenplayEditor(const QStringList& _changedSettingsKeys);
    void reconfigureScreenplayNavigator();
    void reconfigureScreenplayDuration();
    void reconfigureComicBookEditor(const QStringList& _changedSettingsKeys);
    void reconfigureComicBookNavigator();
    void reconfigureAudioplayEditor(const QStringList& _changedSettingsKeys);
    void reconfigureAudioplayNavigator();
    void reconfigureAudioplayDuration();
    void reconfigureStageplayEditor(const QStringList& _changedSettingsKeys);
    void reconfigureStageplayNavigator();
    void reconfigureNovelEditor(const QStringList& _changedSettingsKeys);
    void reconfigureNovelNavigator();

    /**
     * @brief Обновить информацию о том, может ли пользователь использовать платные редакторы
     */
    void checkAvailabilityToEdit();

    /**
     * @brief Загрузить данные текущего проекта
     */
    void loadCurrentProject(const Project& _project);
    void updateCurrentProject(const Project& _project);
    void restoreCurrentProjectState(const QString& _path);

    /**
     * @brief Закрыть текущий проект
     */
    void closeCurrentProject(const QString& _path);

    /**
     * @brief Очистить историю изменений
     */
    void clearChangesHistory();

    /**
     * @brief Сохранить изменения проекта
     */
    void saveChanges();

    /**
     * @brief Добавить персонажа
     */
    void addCharacter(const QString& _name, const QString& _content);

    /**
     * @brief Добавить локацию
     */
    void addLocation(const QString& _name, const QString& _content);

    /**
     * @brief Добавить данные по сценарию
     */
    void addScreenplay(const QString& _name, const QString& _titlePage, const QString& _synopsis,
                       const QString& _treatment, const QString& _text);

    /**
     * @brief Модель документа для экспорта
     * @note Маппим модели для случаев, когда у пользователя выбрана титульная страница, чтобы
     *       экспортировался сам скрипт
     */
    BusinessLayer::AbstractModel* currentModelForExport() const;

    /**
     * @brief Получить первую модель проекта со сценарием
     * @note Используется для экспорта сценария в теневом режиме
     */
    BusinessLayer::AbstractModel* firstScriptModel() const;

    /**
     * @brief Текущий открытый документ
     */
    Domain::DocumentObject* currentDocument() const;

    /**
     * @brief Получить список документов, у которых есть несинхроинизированные изменения
     */
    QVector<Domain::DocumentObject*> unsyncedDocuments() const;

    /**
     * @brief Смержить данные документа полученные с облака с текущими
     */
    void mergeDocumentInfo(const Domain::DocumentInfo& _documentInfo);

    /**
     * @brief Применить данные документа полученные с облака к текущим
     */
    void applyDocumentChanges(const Domain::DocumentInfo& _documentInfo);

    /**
     * @brief Список изменений документа, которые ещё не были синхронизированы
     */
    QVector<Domain::DocumentChangeObject*> unsyncedChanges(const QUuid& _documentUuid) const;

    /**
     * @brief Пометить отправленные изменения сохранёнными
     */
    void markChangesSynced(const QUuid& _documentUuid);

    /**
     * @brief Запланировать полную синхроинзацию документа
     */
    void planDocumentSyncing(const QUuid& _documentUuid);

    /**
     * @brief Получить данные документа для синхронизации
     */
    Domain::DocumentObject* documentToSync(const QUuid& _documentUuid) const;

    /**
     * @brief Получить список документов, связанных с заданным
     */
    QVector<QUuid> connectedDocuments(const QUuid& _documentUuid) const;

    /**
     * @brief Задать курсоры соавторов
     */
    void setCursors(const QUuid& _document, const QVector<Domain::CursorInfo>& _cursors);

    /**
     * @brief Очистить курсоры соавторов
     */
    void clearCursors();

    /**
     * @brief Задать кол-во кредитов доступных для использования с ИИ инструментами
     */
    void setAvailableCredits(int _credits);

    /**
     * @brief Установить результат генерации текста
     */
    void setRephrasedText(const QString& _text);
    void setExpandedText(const QString& _text);
    void setShortenedText(const QString& _text);
    void setInsertedText(const QString& _text);
    void setSummarizeedText(const QString& _text);
    void setTranslatedText(const QString& _text);
    void setGeneratedText(const QString& _generatedText);

signals:
    /**
     * @brief Запрос на отображение меню
     */
    void menuRequested();

    /**
     * @brief Запрос на апгрейд аккаунта из одного из плагинов
     */
    void upgradeToProRequested();
    void upgradeToTeamRequested();

    /**
     * @brief Запрос на покупку кредитов из одного из плагинов
     */
    void buyCreditsRequested();

    /**
     * @brief Изменились данные
     */
    void contentsChanged(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Изменился гуид проекта
     */
    void projectUuidChanged(const QUuid& _uuid);

    /**
     * @brief Изменилось название проекта
     */
    void projectNameChanged(const QString& _name);

    /**
     * @brief Изменилось короткое описание проекта
     */
    void projectLoglineChanged(const QString& _logline);

    /**
     * @brief Изменилась обложка проекта
     */
    void projectCoverChanged(const QPixmap& _cover);

    /**
     * @brief Пользователь хочет изменить список соавторов в проекте
     */
    void projectCollaboratorInviteRequested(const QString& _email, const QColor& _color, int _role);
    void projectCollaboratorUpdateRequested(const QString& _email, const QColor& _color, int _role);
    void projectCollaboratorRemoveRequested(const QString& _email);

    /**
     * @brief Сменилась модель структуры
     */
    void structureModelChanged(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Запрос на загрузку документа
     */
    void downloadDocumentRequested(const QUuid& _documentUuid);

    /**
     * @brief Сменилась текущая модель документа
     */
    void currentModelChanged(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Необходимо отправить данные документа на сервер
     */
    void uploadDocumentRequested(const QUuid& _documentUuid, bool _isNewDocument);

    /**
     * @brief Документ был удалён
     */
    void documentRemoved(const QUuid& _documentUuid);

    /**
     * @brief Изменилась позиция курсора
     */
    void cursorChanged(const QUuid& _documentUuid, const QByteArray& _cursorData);

    /**
     * @brief Запрос на генерацию текста с заданными настройками
     */
    void rephraseTextRequested(const QString& _sourceText, const QString& _style);
    void expandTextRequested(const QString& _sourceText);
    void shortenTextRequested(const QString& _sourceText);
    void insertTextRequested(const QString& _after, const QString& _before);
    void summarizeTextRequested(const QString& _sourceText);
    void translateTextRequested(const QString& _text, const QString& _languageCode);
    void generateTextRequested(const QString& _prompt, const QString& _promptSuffix);

protected:
    /**
     * @brief Переопределяем для обработки события простоя пользователя
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Реализуем фильтр на событие смены дизайн системы, чтобы обновить значения в
     *        представлениях открытых в отдельных окнах, а также на смену локализации, чтобы
     *        обновить переводы пунктов меню
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

private:
    /**
     * @brief Обработать изменение модели
     */
    void handleModelChange(BusinessLayer::AbstractModel* _model, const QByteArray& _undo,
                           const QByteArray& _redo);

    /**
     * @brief Отменить последнее изменение в модели с заданным индексом
     */
    void undoModelChange(BusinessLayer::AbstractModel* _model, int _undoStep);

    /**
     * @brief Отобразить представление заданного типа для заданного элемента
     */
    void showView(const QModelIndex& _index);
    void showView(const QModelIndex& _itemIndex, const QString& _viewMimeType,
                  const QString& _defaultMimeType = {});
    void showViewForVersion(BusinessLayer::StructureModelItem* _item);

    /**
     * @brief Активировать представление заданным элементом и маймом редактора
     */
    void activateView(const QModelIndex& _itemIndex, const QString& _viewMimeType);

    /**
     * @brief Отобразить навигатор для заданного индекса
     * @note Если индекс невалидный, то отображается навигатор по проекту
     */
    void showNavigator(const QModelIndex& _itemIndex, const QString& _viewMimeType = {});
    void showNavigatorForVersion(BusinessLayer::StructureModelItem* _item);

    /**
     * @brief Уведомить клиентов об обновлении курсора
     */
    Q_SLOT void notifyCursorChanged(const QByteArray& _cursorData);

    /**
     * @brief Пользователь хочет активировать ссылку на за документ с заданным индексом элемента
     */
    Q_SLOT void activateLink(const QUuid& _documentUuid, const QModelIndex& _index);

    /**
     * @brief Обновить значение текущей модели и её представления
     */
    void updateCurrentDocument(BusinessLayer::AbstractModel* _model, const QString& _viewMimeType);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
