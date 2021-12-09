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
class ProjectPluginsBuilder final
{
public:
    ProjectPluginsBuilder();
    ~ProjectPluginsBuilder();

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
                                    BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать два менеджера
     * @note Обычно используется для связки навигатора и редактора
     */
    void bind(const QString& _viewMimeType, const QString& _navigatorMimeType);

    /**
     * @brief Активировать полноэкранный режим
     */
    void toggleFullScreen(bool _isFullScreen, const QString& _viewMimeType);

    /**
     * @brief Перенастроить плагины
     */
    void reconfigureAll();
    void reconfigurePlugin(const QString& _mimeType, const QStringList& _changedSettingsKeys);
    void reconfigureSimpleTextEditor(const QStringList& _changedSettingsKeys);
    void reconfigureSimpleTextNavigator();
    void reconfigureScreenplayEditor(const QStringList& _changedSettingsKeys);
    void reconfigureScreenplayNavigator();
    void reconfigureComicBookEditor(const QStringList& _changedSettingsKeys);
    void reconfigureComicBookNavigator();

    /**
     * @brief Сбросить модели для всех плагинов
     */
    void resetModels();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
