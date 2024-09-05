#pragma once

#include <business_layer/export/export_options.h>
#include <ui/export/script_export_dialog.h>

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта простого текстового документа
 */
class SimpleTextExportDialog : public AbstractExportDialog
{
    Q_OBJECT

public:
    explicit SimpleTextExportDialog(const QString& _uuidKey, QWidget* _parent = nullptr);
    ~SimpleTextExportDialog() override;

    /**
     * @brief Получить опции экспорта
     */
    BusinessLayer::ExportOptions& exportOptions() const override;

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
