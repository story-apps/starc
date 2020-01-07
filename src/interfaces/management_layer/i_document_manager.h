#pragma once

#include <QtPlugin>

namespace BusinessLayer {
class AbstractModel;
}

class QWidget;

namespace ManagementLayer
{

/**
 * @brief Интерфейс менеджера документа
 */
class IDocumentManager
{
public:
    virtual ~IDocumentManager() = default;

    /**
     * @brief Задать модель документа
     */
    virtual void setModel(BusinessLayer::AbstractModel* _model) = 0;

    /**
     * @brief Представление документа для основного окна приложения
     */
    virtual QWidget* view() = 0;

    /**
     * @brief Представление документа для отдельного окна
     */
    virtual QWidget* createView() = 0;
};

} // namespace ManagementLayer

Q_DECLARE_INTERFACE(ManagementLayer::IDocumentManager, "app.starc.ManagementLayer.IDocumentManager")
