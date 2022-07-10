#pragma once

#include "../manager_plugin_global.h"

#include <interfaces/management_layer/i_document_manager.h>

#include <QObject>


namespace ManagementLayer {

/**
 * @brief Менеджер титульной страницы
 */
class MANAGER_PLUGIN_EXPORT TitlePageManager : public QObject, public IDocumentManager
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "app.starc.ManagementLayer.IDocumentManager")
    Q_INTERFACES(ManagementLayer::IDocumentManager)

public:
    explicit TitlePageManager(QObject* _parent = nullptr);
    ~TitlePageManager() override;

    /**
     * @brief Реализуем интерфейс менеджера документа
     */
    /** @{ */
    Ui::IDocumentView* view() override;
    Ui::IDocumentView* view(BusinessLayer::AbstractModel* _model) override;
    Ui::IDocumentView* secondaryView() override;
    Ui::IDocumentView* secondaryView(BusinessLayer::AbstractModel* _model) override;
    Ui::IDocumentView* createView(BusinessLayer::AbstractModel* _model) override;
    void resetModels() override;
    void reconfigure(const QStringList& _changedSettingsKeys) override;
    void saveSettings() override;
    void setEditingMode(DocumentEditingMode _mode) override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
