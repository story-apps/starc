#pragma once

#include <QtPlugin>

namespace BusinessLayer {
class AbstractModel;
}

class QObject;
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
     * @brief Вернуть указатель на себя как QObject, если менеджер является наследником  QObject
     */
    virtual QObject* asQObject() { return nullptr; }

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

    /**
     * @brief Перенастроить редактор, которым управляет менеджер
     */
    virtual void reconfigure() {}

    /**
     * @brief Связать с другим менеджером
     */
    virtual void bind(IDocumentManager*) {}
};

} // namespace ManagementLayer

Q_DECLARE_INTERFACE(ManagementLayer::IDocumentManager, "app.starc.ManagementLayer.IDocumentManager")
