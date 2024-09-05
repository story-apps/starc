#pragma once

#include "document_export_dialog.h"

#include <business_layer/export/characters/character_export_options.h>

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта персонажа
 */
class CharacterExportDialog : public DocumentExportDialog
{
    Q_OBJECT

public:
    explicit CharacterExportDialog(const QString& _uuidKey, QWidget* _parent = nullptr);
    ~CharacterExportDialog() override;

    /**
     * @brief Получить опции экспорта
     */
    BusinessLayer::CharacterExportOptions& exportOptions() const override;

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
