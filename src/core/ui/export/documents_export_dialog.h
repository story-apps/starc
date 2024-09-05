#pragma once

#include "abstract_export_dialog.h"

#include <business_layer/export/export_options.h>

namespace BusinessLayer {
class AbstractModel;
} // namespace BusinessLayer

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта документов
 */
class DocumentsExportDialog : public AbstractExportDialog
{
    Q_OBJECT

public:
    explicit DocumentsExportDialog(const QVector<BusinessLayer::ExportFileFormat>& _formats,
                                   const QString& _uuidKey, QWidget* _parent = nullptr);
    ~DocumentsExportDialog() override;

    /**
     * @brief Задать модель документов для отображения списка
     */
    void setModel(BusinessLayer::AbstractModel* _model) const;

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
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

    /**
     * @brief Обновить видимость параметров
     */
    void updateParametersVisibility() const override;

    /**
     * @brief Должна ли быть активна кнопка "Экспортировать"
     */
    bool isExportEnabled() const override;

    /**
     * @brief Получить список документов вместе с состояниями их чекбоксов
     */
    QVariantMap checkedDocuments() const;

    /**
     * @brief Получить ключ для сохранения списка выбранных документов
     */
    virtual QString documentsKey() const = 0;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
