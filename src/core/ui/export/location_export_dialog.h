#pragma once

#include "document_export_dialog.h"

#include <business_layer/export/locations/location_export_options.h>

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта локации
 */
class LocationExportDialog : public DocumentExportDialog
{
    Q_OBJECT

public:
    explicit LocationExportDialog(const QString& _uuidKey, QWidget* _parent = nullptr);
    ~LocationExportDialog() override;

    /**
     * @brief Получить опции экспорта
     */
    BusinessLayer::LocationExportOptions& exportOptions() const override;

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
