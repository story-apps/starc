#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

namespace Domain {
struct Notification;
}


namespace Ui {

/**
 * @brief Меню приложения
 */
class MenuView : public StackWidget
{
    Q_OBJECT

public:
    explicit MenuView(QWidget* _parent = nullptr);
    ~MenuView() override;

    /**
     * @brief Установить необходимость использования панели аккаунта
     */
    void setAccountVisible(bool _use);

    /**
     * @brief Параметры панели аккаунта
     */
    void setAvatar(const QPixmap& _avatar);
    void setAccountName(const QString& _name);
    void setAccountEmail(const QString& _email);

    /**
     * @brief Задать видимость пункта меню "Авторизоваться"
     */
    void setSignInVisible(bool _visible);

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
     * @brief Задать горячие клавиши импорта
     */
    void setImportShortcut(const QKeySequence& _key);

    /**
     * @brief Задать доступность импорта
     */
    void setImportAvailable(bool _available);

    /**
     * @brief Задать горячие клавиши экспорта текущего документа
     */
    void setCurrentDocumentExportShortcut(const QKeySequence& _key);

    /**
     * @brief Установить возможность экспортирования текущего документа
     */
    void setCurrentDocumentExportAvailable(bool _available);

    /**
     * @brief Показать/скрыть наличие непрочитанных комментариев
     */
    void setHasUnreadNotifications(bool _hasUnreadNotifications);

    /**
     * @brief Задать необходимость отображать дев-версии
     */
    void setShowDevVersions(bool _show);

    /**
     * @brief Задать список уведомлений
     */
    void setNotifications(const QVector<Domain::Notification>& _notifications);

    /**
     * @brief Открыть меню
     */
    void openMenu();

    /**
     * @brief Закрытие меню
     */
    void closeMenu();

    /**
     * @brief В качестве идеального размера используем размер самого меню
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Пользователь кликнул на аккаунте
     */
    void accountPressed();

    /**
     * @brief Нажат пункт меню "Авторизоваться"
     */
    void signInPressed();

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
    void saveProjectChangesPressed();

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
     * @brief Нажат пункт меню "Перейти в/выйти из полноэкранного режима"
     */
    void fullscreenPressed();

    /**
     * @brief Нажат пункт меню "Настройки"
     */
    void settingsPressed();

    /**
     * @brief Нажат пункт меню "Справка"
     */
    void helpPressed();

    /**
     * @brief Нажата кнопка отображения статистики по сессиям работы с программой
     */
    void writingStatisticsPressed();

    /**
     * @brief Нажата кнопка отображения таймера писательского спринта
     */
    void writingSprintPressed();

    /**
     * @brief Нажата кнопка отображения списка уведомлений
     */
    void notificationsPressed();

    /**
     * @brief Пользователь включил/отключил отображение дев-версий
     */
    void showDevVersionsChanged(bool _show);

    /**
     * @brief Пользователь нажал кнопку продления PRO версии
     */
    void renewProPressed();

    /**
     * @brief Пользователь нажал кнопку продления CLOUD версии
     */
    void renewTeamPressed();

protected:
    /**
     * @brief Обработка нажатия клавиатуры (Закрытие при нажатии esc)
     */
    void keyPressEvent(QKeyEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Настроить внешний вид
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

    /**
     * @brief Добавить MenuBar
     */
    void createMenuBar();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
