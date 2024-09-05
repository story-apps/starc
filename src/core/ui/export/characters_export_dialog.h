#pragma once

#include "documents_export_dialog.h"

#include <business_layer/export/characters/characters_export_options.h>

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта персонажей
 */
class CharactersExportDialog : public DocumentsExportDialog
{
    Q_OBJECT

public:
    explicit CharactersExportDialog(const QString& _uuidKey, QWidget* _parent = nullptr);
    ~CharactersExportDialog() override;

    /**
     * @brief Получить опции экспорта
     */
    BusinessLayer::CharactersExportOptions& exportOptions() const override;

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
     * @brief Получить ключ для сохранения списка выбранных документов
     */
    QString documentsKey() const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
