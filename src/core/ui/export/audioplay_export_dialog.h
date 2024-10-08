#pragma once

#include <business_layer/export/audioplay/audioplay_export_options.h>
#include <ui/export/script_export_dialog.h>

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта аудиопьесы
 */
class AudioplayExportDialog : public ScriptExportDialog
{
    Q_OBJECT

public:
    explicit AudioplayExportDialog(const QString& _uuidKey, QWidget* _parent = nullptr);
    ~AudioplayExportDialog() override;

    /**
     * @brief Получить опции экспорта
     */
    BusinessLayer::AudioplayExportOptions& exportOptions() const override;

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
