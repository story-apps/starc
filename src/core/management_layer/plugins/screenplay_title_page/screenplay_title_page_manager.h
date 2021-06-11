#pragma once

#include "../manager_plugin_global.h"

#include <interfaces/management_layer/i_document_manager.h>

#include <QObject>


namespace ManagementLayer {

/**
 * @brief Менеджер информации о сценарии
 */
class MANAGER_PLUGIN_EXPORT ScreenplayTitlePageManager : public QObject, public IDocumentManager
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "app.starc.ManagementLayer.IDocumentManager")
    Q_INTERFACES(ManagementLayer::IDocumentManager)

public:
    explicit ScreenplayTitlePageManager(QObject* _parent = nullptr);
    ~ScreenplayTitlePageManager() override;

    /**
     * @brief Реализуем интерфейс менеджера документа
     */
    /** @{ */
    void setModel(BusinessLayer::AbstractModel* _model) override;
    QWidget* view() override;
    QWidget* createView() override;
    void reconfigure(const QStringList& _changedSettingsKeys) override;
    void saveSettings() override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
