#pragma once

#include "../manager_plugin_global.h"

#include <interfaces/management_layer/i_document_manager.h>

#include <QObject>


namespace ManagementLayer {

/**
 * @brief Менеджер структуры
 */
class MANAGER_PLUGIN_EXPORT WorldInformationStructureManager : public QObject,
                                                               public IDocumentManager
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "app.starc.ManagementLayer.IDocumentManager")
    Q_INTERFACES(ManagementLayer::IDocumentManager)

public:
    explicit WorldInformationStructureManager(QObject* _parent = nullptr);
    ~WorldInformationStructureManager() override;

    /**
     * @brief Реализуем интерфейс менеджера документа
     */
    /** @{ */
    QObject* asQObject() override;
    Ui::IDocumentView* view() override;
    Ui::IDocumentView* view(BusinessLayer::AbstractModel* _model) override;
    Ui::IDocumentView* secondaryView() override;
    Ui::IDocumentView* secondaryView(BusinessLayer::AbstractModel* _model) override;
    Ui::IDocumentView* createView(BusinessLayer::AbstractModel* _model) override;
    void resetModels() override;
    /** @} */

signals:
    /**
     * @brief Пользователь выбрал категорию параметров с заданным индексом
     */
    void traitsCategoryIndexChanged(const QModelIndex& _index);

private:
    /**
     * @brief Установить модель
     */
    void setModel(BusinessLayer::AbstractModel* _model);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
