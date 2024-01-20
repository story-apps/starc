#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>

namespace Domain {
struct TeamInfo;
}

namespace Ui {

/**
 * @brief Диалог создания нового проекта
 */
class CreateProjectDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit CreateProjectDialog(QWidget* _parent = nullptr);
    ~CreateProjectDialog() override;

    /**
     * @brief Тип проекта
     */
    int projectType() const;
    void setProjectType(int _type);

    /**
     * @brief Название нового проекта
     */
    QString projectName() const;
    void setProjectName(const QString& _name);

    /**
     * @brief Папка, куда будет сохраняться новый проект
     */
    QString projectFolder() const;
    void setProjectFolder(const QString& _path);

    /**
     * @brief Папка, из которой будут выбираться проекты для импорта
     */
    QString importFilePath() const;
    void setImportFolder(const QString& _path);

    /**
     * @brief Настроить возможность создавать проекты в облаке
     */
    void configureCloudProjectCreationAbility(bool _isConnected, bool _isLogged,
                                              bool _isSubscriptionActive,
                                              const QVector<Domain::TeamInfo>& _teams);

    /**
     * @brief Где пользователь хочет разместить проект true - локально, false - в облаке
     */
    bool isLocal() const;

    /**
     * @brief Айди команды в которую пользователь хочет поместить проект
     */
    int teamId() const;

    /**
     * @brief Показать проблему создания проекта в заданной папке
     */
    void showProjectFolderError();

signals:
    /**
     * @brief Пользователь нажал ссылку для авторизации
     */
    void loginPressed();

    /**
     * @brief Пользователь нажал ссылку для продления подписки
     */
    void renewSubscriptionPressed();

    /**
     * @brief Пользователь нажал на кнопку создания проекта
     */
    void createProjectPressed();

protected:
    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

    /**
     * @brief Опеределим последний фокусируемый виджет в диалоге
     */
    QWidget* lastFocusableWidget() const override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
