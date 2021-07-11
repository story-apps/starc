#pragma once

#include <QtPlugin>

class QString;

namespace ManagementLayer {

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
    virtual void exec(const QString& _fileToOpenPath) = 0;

    /**
     * @brief Открыть проект по заданному пути
     */
    virtual void openProject(const QString& _path) = 0;
};

} // namespace ManagementLayer

Q_DECLARE_INTERFACE(ManagementLayer::IApplicationManager,
                    "app.starc.ManagementLayer.IApplicationManager")
