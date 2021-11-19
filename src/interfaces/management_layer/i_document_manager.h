#pragma once

#include <QtPlugin>

namespace BusinessLayer {
class AbstractModel;
}

namespace Ui {
class IDocumentView;
}

class QObject;
class QStringList;
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
     * @brief Перенастроить редактор, которым управляет менеджер
     */
    virtual void reconfigure(const QStringList& _changedSettingsKeys)
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
};

} // namespace ManagementLayer

Q_DECLARE_INTERFACE(ManagementLayer::IDocumentManager, "app.starc.ManagementLayer.IDocumentManager")
