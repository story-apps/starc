#pragma once

#include <QObject>


namespace ManagementLayer
{

/**
 * @brief Управляющий открытым проектом
 */
class ProjectManager : public QObject
{
    Q_OBJECT

public:
    explicit ProjectManager(QObject* _parent, QWidget* _parentWidget);
    ~ProjectManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

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

signals:
    /**
     * @brief Запрос на отображение меню
     */
    void menuRequested();

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

private:
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
    class Implementation;
    QScopedPointer<Implementation> d;
};

}
