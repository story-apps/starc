#pragma once

#include <QScopedPointer>
#include <QVector>

namespace BusinessLayer {
class AbstractModel;
}

class QWidget;


namespace ManagementLayer
{

/**
 * @brief Билдер управляющая созданием плагинов редакторов
 */
class ProjectPluginsBuilder final
{
public:
    ProjectPluginsBuilder();
    ~ProjectPluginsBuilder();

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
     * @brief Получить майм-тип навигатора для заданного редактора
     * @note Если такого не существует, возвращается пустая строка
     */
    QString navigatorMimeTypeFor(const QString& _editorMimeType) const;

    /**
     * @brief Активировать представление заданного типа заданной моделью
     */
    QWidget* activateView(const QString& _viewMimeType, BusinessLayer::AbstractModel* _model);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
