#pragma once

#include <core_global.h>

#include <interfaces/management_layer/i_application_manager.h>

#include <QObject>


namespace ManagementLayer
{

/**
 * @brief Менеджер приложения
 */
class CORE_PLUGIN_EXPORT ApplicationManager : public QObject, IApplicationManager
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "app.starc.ManagementLayer.IApplicationManager")
    Q_INTERFACES(ManagementLayer::IApplicationManager)

public:
    explicit ApplicationManager(QObject* _parent = nullptr);
    ~ApplicationManager() override;

    /**
     * @brief Запуск приложения
     */
    void exec(const QString& _fileToOpenPath) override final;

    /**
     * @brief Открытие файла
     */
    void openProject(const QString& _path) override final;

protected:
    /**
     * @brief Переопределяем для обработки кастомных событий
     */
    bool event(QEvent* _event) override;

private:
    /**
     * @brief Настроить соединнеия
     */
    void initConnections();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
