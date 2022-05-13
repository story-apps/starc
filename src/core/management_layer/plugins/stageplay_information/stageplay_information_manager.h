#pragma once

#include "../manager_plugin_global.h"

#include <interfaces/management_layer/i_document_manager.h>

#include <QObject>


namespace ManagementLayer {

/**
 * @brief Менеджер информации о пьесе
 */
class MANAGER_PLUGIN_EXPORT StageplayInformationManager : public QObject, public IDocumentManager
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "app.starc.ManagementLayer.IDocumentManager")
    Q_INTERFACES(ManagementLayer::IDocumentManager)

public:
    explicit StageplayInformationManager(QObject* _parent = nullptr);
    ~StageplayInformationManager() override;

    /**
     * @brief Реализуем интерфейс менеджера документа
     */
    /** @{ */
    void setModel(BusinessLayer::AbstractModel* _model) override;
    Ui::IDocumentView* view() override;
    Ui::IDocumentView* createView() override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
