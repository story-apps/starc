#pragma once

#include <business_layer/export/comic_book/comic_book_export_options.h>
#include <ui/export/script_export_dialog.h>

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта комикса
 */
class ComicBookExportDialog : public ScriptExportDialog
{
    Q_OBJECT

public:
    explicit ComicBookExportDialog(const QString& _uuidKey, QWidget* _parent = nullptr);
    ~ComicBookExportDialog() override;

    /**
     * @brief Получить опции экспорта
     */
    BusinessLayer::ComicBookExportOptions& exportOptions() const override;

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
