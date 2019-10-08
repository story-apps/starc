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
    void checkStories();

    /**
     * @brief Установить видимость пунктов меню, относящихся к проекту
     */
    void setStoryActionsVisible(bool _visible);

    /**
     * @brief Пометить пункт меню текущего проекта выделенным
     */
    void checkStory();

    /**
     * @brief Установить название текущего проекта
     */
    void setStoryTitle(const QString& _title);

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
    void storiesPressed();

    /**
     * @brief Нажат пункт меню "Создать проект"
     */
    void createStoryPressed();

    /**
     * @brief Нажат пункт меню "Открыть проект"
     */
    void openStoryPressed();

    /**
     * @brief Нажат пункт меню "Проект"
     */
    void storyPressed();

    /**
     * @brief Нажат пункт меню "Сохранить изменения"
     */
    void saveChangesPressed();

    /**
     * @brief Нажат пункт меню "Экспортировать"
     */
    void exportPressed();

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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
