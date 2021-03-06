#pragma once

#include <ui/widgets/drawer/drawer.h>


namespace Ui
{

/**
 * @brief Меню приложения
 */
class MenuView : public Drawer
{
    Q_OBJECT

public:
    explicit MenuView(QWidget* _parent = nullptr);
    ~MenuView() override;

    /**
     * @brief Пометить пункт меню проектов выделенным
     */
    void checkProjects();

    /**
     * @brief Установить видимость пунктов меню, относящихся к проекту
     */
    void setProjectActionsVisible(bool _visible);

    /**
     * @brief Пометить пункт меню текущего проекта выделенным
     */
    void checkProject();

    /**
     * @brief Установить название текущего проекта
     */
    void setProjectTitle(const QString& _title);

    /**
     * @brief Пометить пункт меню настройки выделенным
     */
    void checkSettings();

    /**
     * @brief Установить состояние сохранённости изменений проекта
     */
    void markChangesSaved(bool _saved);

    /**
     * @brief Установить индикацию отображения куплена ли про версия
     */
    void setProVersion(bool _isPro);

signals:
    /**
     * @brief Нажат пункт меню "Проекты"
     */
    void projectsPressed();

    /**
     * @brief Нажат пункт меню "Создать проект"
     */
    void createProjectPressed();

    /**
     * @brief Нажат пункт меню "Открыть проект"
     */
    void openProjectPressed();

    /**
     * @brief Нажат пункт меню "Проект"
     */
    void projectPressed();

    /**
     * @brief Нажат пункт меню "Сохранить изменения"
     */
    void saveChangesPressed();

    /**
     * @brief Нажат пункт меню "Сохранить проект как"
     */
    void saveProjectAsPressed();

    /**
     * @brief Нажат пункт меню "Экспортировать"
     */
    void exportProjectPressed();
    void exportCurrentDocumentPressed();

    /**
     * @brief Нажат пункт меню "Импортировать"
     */
    void importPressed();

    /**
     * @brief Нажат пункт меню "Настройки"
     */
    void settingsPressed();

    /**
     * @brief Нажат пункт меню "Справка"
     */
    void helpPressed();

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
