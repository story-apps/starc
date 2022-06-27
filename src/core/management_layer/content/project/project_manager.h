#pragma once

#include <QObject>


namespace BusinessLayer {
class AbstractModel;
}

namespace Domain {
enum class DocumentObjectType;
}

namespace ManagementLayer {

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

    /**
     * @brief Обновить информацию о том, может ли пользователь использовать платные редакторы
     */
    void checkAvailabilityToEdit();

    /**
     * @brief Загрузить данные текущего проекта
     */
    void loadCurrentProject(const QString& _name, const QString& _path);

    /**
     * @brief Закрыть текущий проект
     */
    void closeCurrentProject(const QString& _path);

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

signals:
    /**
     * @brief Запрос на отображение меню
     */
    void menuRequested();

    /**
     * @brief Запрос на апгрейд аккаунта из одного из плагинов
     */
    void upgradeRequested();

    /**
     * @brief Изменились данные
     */
    void contentsChanged();

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
     * @brief Сменилась текущая модель документа
     */
    void currentModelChanged(BusinessLayer::AbstractModel* _model);

protected:
    /**
     * @brief Переопределяем для обработки события простоя пользователя
     */
    bool event(QEvent* _event) override;

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
    void showView(const QModelIndex& _itemIndex, const QString& _viewMimeType);

    /**
     * @brief Отобразить навигатор для заданного индекса
     * @note Если индекс невалидный, то отображается навигатор по проекту
     */
    void showNavigator(const QModelIndex& _itemIndex, const QString& _viewMimeType = {});

private:
    /**
     * @brief Обновить значение текущей модели и её представления
     */
    void updateCurrentDocument(BusinessLayer::AbstractModel* _model, const QString& _viewMimeType);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
