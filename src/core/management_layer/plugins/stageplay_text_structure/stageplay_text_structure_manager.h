#pragma once

#include "../manager_plugin_global.h"

#include <interfaces/management_layer/i_document_manager.h>

#include <QObject>


namespace ManagementLayer {

/**
 * @brief Менеджер структуры аудиопостановки
 */
class MANAGER_PLUGIN_EXPORT StageplayTextStructureManager : public QObject, public IDocumentManager
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "app.starc.ManagementLayer.IDocumentManager")
    Q_INTERFACES(ManagementLayer::IDocumentManager)

public:
    explicit StageplayTextStructureManager(QObject* _parent = nullptr);
    ~StageplayTextStructureManager() override;

    /**
     * @brief Реализуем интерфейс менеджера документа
     */
    /** @{ */
    QObject* asQObject() override;
    void setModel(BusinessLayer::AbstractModel* _model) override;
    Ui::IDocumentView* view() override;
    Ui::IDocumentView* createView() override;
    void reconfigure(const QStringList& _changedSettingsKeys) override;
    void bind(IDocumentManager* _manager) override;
    /** @} */

signals:
    /**
     * @brief Пользователь выбрал элемент в навигаторе с заданным индексом в модели сценария
     */
    void currentModelIndexChanged(const QModelIndex& _index);

private:
    /**
     * @brief Выбрать в навигаторе элемент соответствующий заданному индексу в модели
     */
    Q_SLOT void setCurrentModelIndex(const QModelIndex& _index);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
