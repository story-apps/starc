#pragma once

#include "../manager_plugin_global.h"

#include <interfaces/management_layer/i_document_manager.h>

#include <QObject>


namespace ManagementLayer
{

/**
 * @brief Менеджер текста сценария
 */
class MANAGER_PLUGIN_EXPORT ScreenplayTextManager : public QObject, public IDocumentManager
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "app.starc.ManagementLayer.IDocumentManager")
    Q_INTERFACES(ManagementLayer::IDocumentManager)

public:
    explicit ScreenplayTextManager(QObject* _parent = nullptr);
    ~ScreenplayTextManager() override;

    /**
     * @brief Реализуем интерфейс менеджера документа
     */
    /** @{ */
    void setModel(BusinessLayer::AbstractModel* _model) override;
    QWidget* view() override;
    QWidget* createView() override;
    void reconfigure() /*override*/;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
