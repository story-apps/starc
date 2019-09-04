#pragma once

#include "../core_global.h"

#include <interfaces/management_layer/i_application_manager.h>

#include <QObject>


namespace ManagementLayer
{

/**
 * @brief Менеджер приложения
 */
class CORE_LIBRARY_EXPORT ApplicationManager : public QObject, IApplicationManager
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
    void exec() final;

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
