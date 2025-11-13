#pragma once

#include "abstract_export_dialog.h"

namespace BusinessLayer {
enum class ExportFileFormat;
} // namespace BusinessLayer

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта сценариев
 */
class ScriptExportDialog : public AbstractExportDialog
{
    Q_OBJECT

public:
    explicit ScriptExportDialog(const QVector<BusinessLayer::ExportFileFormat>& _formats,
                                const QString& _uuidKey, QWidget* _parent = nullptr);
    ~ScriptExportDialog() override;

    /**
     * @brief Задать список драфтов сценария для экспорта
     */
    void setDrafts(const QVector<QString>& _drafts, int _currentDraftIndex);

    /**
     * @brief Индекс драфта для экспорта
     */
    int selectedDraft() const;

    /**
     * @brief Получить опции экспорта
     */
    BusinessLayer::ExportOptions& exportOptions() const override;

protected:
    /**
     * @brief Обновить видимость параметров
     */
    void updateParametersVisibility() const override;

    /**
     * @brief Должа ли быть активна кнопка "Экспортировать"
     */
    bool isExportEnabled() const override;

    /**
     * @brief Следует ли показывать праметры справа
     */
    virtual bool isRightLayoutVisible() const;

    /**
     * @brief Установить параметр "Экспортировать текст сценария"
     */
    virtual void setIncludeScript(bool _checked) const;

    /**
     * @brief Стоит ли экспортировать текст документа
     */
    virtual bool shouldIncludeText() const;

    /**
     * @brief Обработать изменение параметра "Экспортировать текст сценария"
     */
    virtual void processIncludeTextChanged(bool _checked) const;

    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

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
