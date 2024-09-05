#pragma once

#include "documents_export_dialog.h"

#include <business_layer/export/export_options.h>

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта локаций
 */
class LocationsExportDialog : public DocumentsExportDialog
{
    Q_OBJECT

public:
    explicit LocationsExportDialog(const QString& _uuidKey, QWidget* _parent = nullptr);
    ~LocationsExportDialog() override;

    /**
     * @brief Получить опции экспорта
     */
    BusinessLayer::DocumentsExportOptions& exportOptions() const override;

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Получить ключ для сохранения списка выбранных документов
     */
    QString documentsKey() const override;
};

} // namespace Ui
