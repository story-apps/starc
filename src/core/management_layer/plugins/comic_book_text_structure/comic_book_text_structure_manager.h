#pragma once

#include "../manager_plugin_global.h"

#include <interfaces/management_layer/i_document_manager.h>

#include <QObject>


namespace ManagementLayer {

/**
 * @brief Менеджер навигатора комикса
 */
class MANAGER_PLUGIN_EXPORT ComicBookTextStructureManager : public QObject, public IDocumentManager
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "app.starc.ManagementLayer.IDocumentManager")
    Q_INTERFACES(ManagementLayer::IDocumentManager)

public:
    explicit ComicBookTextStructureManager(QObject* _parent = nullptr);
    ~ComicBookTextStructureManager() override;

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
    void reconfigure(const QStringList& _changedSettingsKeys) override;
    void bind(IDocumentManager* _manager) override;
    void setEditingMode(DocumentEditingMode _mode) override;
    /** @} */

signals:
    /**
     * @brief Пользователь выбрал элемент в навигаторе с заданным индексом в модели
     */
    void currentModelIndexChanged(const QModelIndex& _index);

private:
    /**
     * @brief Выбрать в навигаторе элемент соответствующий заданному индексу в модели
     */
    Q_SLOT void setCurrentModelIndex(const QModelIndex& _index);

    /**
     * @brief Установить модель
     */
    void setModel(BusinessLayer::AbstractModel* _model);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
