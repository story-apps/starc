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
     * @brief Получить путь к файлу с логами
     */
    virtual QString logFilePath() const = 0;

    /**
     * @brief Запустить приложение
     */
    virtual void exec(const QString& _fileToOpenPath) = 0;

    /**
     * @brief Открыть проект по заданному пути
     */
    virtual bool openProject(const QString& _path) = 0;
};

} // namespace ManagementLayer

Q_DECLARE_INTERFACE(ManagementLayer::IApplicationManager,
                    "app.starc.ManagementLayer.IApplicationManager")
