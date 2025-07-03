#pragma once

#include "document_export_dialog.h"

#include <business_layer/export/worlds/world_export_options.h>

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта мира
 */
class WorldExportDialog : public DocumentExportDialog
{
    Q_OBJECT

public:
    explicit WorldExportDialog(const QString& _uuidKey, QWidget* _parent = nullptr);
    ~WorldExportDialog() override;

    /**
     * @brief Получить опции экспорта
     */
    BusinessLayer::WorldExportOptions& exportOptions() const override;

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
