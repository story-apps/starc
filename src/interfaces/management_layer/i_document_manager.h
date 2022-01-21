#pragma once

#include <QtContainerFwd>
#include <QtPlugin>

namespace BusinessLayer {
class AbstractModel;
}

namespace Ui {
class IDocumentView;
}

class QObject;
class QWidget;

namespace ManagementLayer {

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
    virtual QObject* asQObject()
    {
        return nullptr;
    }

    /**
     * @brief Задать модель документа
     */
    virtual void setModel(BusinessLayer::AbstractModel* _model) = 0;

    /**
     * @brief Представление документа для основного окна приложения
     */
    virtual Ui::IDocumentView* view() = 0;

    /**
     * @brief Представление документа для отдельного окна
     */
    virtual Ui::IDocumentView* createView() = 0;

    /**
     * @brief Перенастроить редактор, которым управляет менеджер,
     *        в соответствии с заданным списком изменившихся параметров
     */
    virtual void reconfigure(const QStringList&)
    {
    }

    /**
     * @brief Связать с другим менеджером
     */
    virtual void bind(IDocumentManager*)
    {
    }

    /**
     * @brief Сохранить параметры плагина
     */
    virtual void saveSettings()
    {
    }

    /**
     * @brief Перепроверить воможность редактирования
     * @note Актуально для платных плагинов
     */
    virtual void checkAvailabilityToEdit()
    {
    }
};

} // namespace ManagementLayer

Q_DECLARE_INTERFACE(ManagementLayer::IDocumentManager, "app.starc.ManagementLayer.IDocumentManager")
