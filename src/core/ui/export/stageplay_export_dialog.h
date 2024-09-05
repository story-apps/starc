#pragma once

#include <business_layer/export/stageplay/stageplay_export_options.h>
#include <ui/export/script_export_dialog.h>

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта пьесы
 */
class StageplayExportDialog : public ScriptExportDialog
{
    Q_OBJECT

public:
    explicit StageplayExportDialog(const QString& _uuidKey, QWidget* _parent = nullptr);
    ~StageplayExportDialog() override;

    /**
     * @brief Получить опции экспорта
     */
    BusinessLayer::StageplayExportOptions& exportOptions() const override;

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
