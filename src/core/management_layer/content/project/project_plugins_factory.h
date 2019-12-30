#pragma once

#include <QScopedPointer>
#include <QVector>

class QWidget;


namespace ManagementLayer
{

/**
 * @brief Фабрика управляющая созданием плагинов редакторов
 */
class ProjectPluginsFactory final
{
public:
    ProjectPluginsFactory();
    ~ProjectPluginsFactory();

    /**
     * @brief Получить майм-тип навигатора для заданного редактора
     * @note Если такого не существует, возвращается пустая строка
     */
    QString navigatorFor(const QString& _editorMimeType) const;

    /**
     * @brief Получить навигатор заданного типа
     */
    QWidget* navigator(const QString& _mimeType) const;

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
    QVector<EditorInfo> viewsFor(const QString& _documentMimeType) const;

    /**
     * @brief Получить редактор заданного типа
     */
    QWidget* view(const QString& _mimeType) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
