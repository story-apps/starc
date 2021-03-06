#include "export_manager.h"

#include <business_layer/model/abstract_model.h>

#include <domain/document_object.h>

#include <ui/export/export_dialog.h>
#include <ui/widgets/dialog/standard_dialog.h>


namespace ManagementLayer
{

class ExportManager::Implementation
{
public:
    explicit Implementation(ExportManager* _parent, QWidget* _topLevelWidget);

    /**
     * @brief Экспортировать сценарий
     */
    void exportScreenplay(BusinessLayer::AbstractModel* _model);

    //
    // Данные
    //

    ExportManager* q = nullptr;

    QWidget* topLevelWidget = nullptr;

    Ui::ExportDialog* exportDialog = nullptr;
};

ExportManager::Implementation::Implementation(ExportManager* _parent, QWidget* _topLevelWidget)
    : q(_parent),
      topLevelWidget(_topLevelWidget)
{

}

void ExportManager::Implementation::exportScreenplay(BusinessLayer::AbstractModel* _model)
{
    if (exportDialog == nullptr) {
        exportDialog = new Ui::ExportDialog(topLevelWidget);
        connect(exportDialog, &Ui::ExportDialog::exportRequested, exportDialog, [this] {
            exportDialog->hideDialog();
//            export(exportDialog->exportOptions());
        });
        connect(exportDialog, &Ui::ExportDialog::canceled, exportDialog, &Ui::ExportDialog::hideDialog);
        connect(exportDialog, &Ui::ExportDialog::disappeared, exportDialog, [this] {
            exportDialog->deleteLater();
            exportDialog = nullptr;
        });
    }

    exportDialog->showDialog();
}


// ****


ExportManager::ExportManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(this, _parentWidget))
{

}

ExportManager::~ExportManager() = default;

void ExportManager::exportDocument(BusinessLayer::AbstractModel* _model)
{
    if (_model == nullptr
        || _model->document() == nullptr) {
        return;
    }

    switch (_model->document()->type())
    {
        case Domain::DocumentObjectType::ScreenplayText: {
            d->exportScreenplay(_model);
            break;
        }

        default: break;
    }
}

} // namespace ManagementLayer
