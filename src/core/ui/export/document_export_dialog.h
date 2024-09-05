#pragma once

#include "abstract_export_dialog.h"

#include <business_layer/export/export_options.h>

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта документа
 */
class DocumentExportDialog : public AbstractExportDialog
{
    Q_OBJECT

public:
    explicit DocumentExportDialog(const QVector<BusinessLayer::ExportFileFormat>& _formats,
                                  const QString& _uuidKey, QWidget* _parent = nullptr);
    ~DocumentExportDialog() override;

    /**
     * @brief Получить опции экспорта
     */
    BusinessLayer::DocumentExportOptions& exportOptions() const override;

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

    /**
     * @brief Обновить видимость параметров
     */
    void updateParametersVisibility() const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
