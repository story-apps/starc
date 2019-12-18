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
    void loadCurrentProject();

    /**
     * @brief Сохранить изменения проекта
     */
    void saveChanges();

signals:
    /**
     * @brief Запрос на отображение меню
     */
    void menuRequested();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

}
