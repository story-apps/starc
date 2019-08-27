#pragma once

#include <QtPlugin>

namespace ManagementLayer
{

/**
 * @brief Интерфейс менеджера приложения
 */
class IApplicationManager
{
public:
    virtual ~IApplicationManager() = default;

    /**
     * @brief Запустить приложение
     */
    virtual void exec() = 0;
};

} // namespace ManagementLayer

Q_DECLARE_INTERFACE(ManagementLayer::IApplicationManager, "app.starc.ManagementLayer.IApplicationManager")
