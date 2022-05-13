#pragma once

#include <QScopedPointer>
#include <QVector>

namespace BusinessLayer {
class AbstractModel;
}

namespace Ui {
class IDocumentView;
}

namespace ManagementLayer {
class IDocumentManager;

/**
 * @brief Билдер управляющая созданием плагинов редакторов
 */
class PluginsBuilder final
{
public:
    PluginsBuilder();
    ~PluginsBuilder();

    /**
     * @brief Получить менеджер документа заданного типа
     */
    IDocumentManager* plugin(const QString& _mimeType) const;

    /**
     * @brief Вспомогательная структура с информацией о плагине редактора
     */
    struct EditorInfo {
        QString mimeType;
        QString icon;
    };

    /**
     * @brief Получить списком инфорацию о доступных редакторах для заданного типа документа
     */
    QVector<EditorInfo> editorsInfoFor(const QString& _documentMimeType) const;

    /**
     * @brief Получить описание редактора по заданному майм типу
     */
    QString editorDescription(const QString& _documentMimeType,
                              const QString& _editorMimeType) const;

    /**
     * @brief Получить майм-тип навигатора для заданного редактора
     * @note Если такого не существует, возвращается пустая строка
     */
    QString navigatorMimeTypeFor(const QString& _editorMimeType) const;

    /**
     * @brief Активировать представление заданного типа заданной моделью
     */
    Ui::IDocumentView* activateView(const QString& _viewMimeType,
                                    BusinessLayer::AbstractModel* _model) const;

    /**
     * @brief Связать два менеджера
     * @note Обычно используется для связки навигатора и редактора
     */
    void bind(const QString& _viewMimeType, const QString& _navigatorMimeType) const;

    /**
     * @brief Активировать полноэкранный режим
     */
    void toggleFullScreen(bool _isFullScreen, const QString& _viewMimeType) const;

    /**
     * @brief Перенастроить плагины
     */
    void reconfigureAll() const;
    void reconfigurePlugin(const QString& _mimeType, const QStringList& _changedSettingsKeys) const;
    void reconfigureSimpleTextEditor(const QStringList& _changedSettingsKeys) const;
    void reconfigureSimpleTextNavigator() const;
    void reconfigureScreenplayEditor(const QStringList& _changedSettingsKeys) const;
    void reconfigureScreenplayNavigator() const;
    void reconfigureComicBookEditor(const QStringList& _changedSettingsKeys) const;
    void reconfigureComicBookNavigator() const;
    void reconfigureAudioplayEditor(const QStringList& _changedSettingsKeys) const;
    void reconfigureAudioplayNavigator() const;
    void reconfigureStageplayEditor(const QStringList& _changedSettingsKeys) const;
    void reconfigureStageplayNavigator() const;

    /**
     * @brief Обновить информацию о том, может ли пользователь использовать платные редакторы
     */
    void checkAvailabilityToEdit() const;

    /**
     * @brief Сбросить модели для всех плагинов
     */
    void resetModels() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
