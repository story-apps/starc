#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>

namespace BusinessLayer {
struct ImportOptions;
}


namespace Ui
{

/**
 * @brief Диалог настройки параметров импорта
 */
class ImportDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit ImportDialog(const QString& _importFilePath, QWidget* _parent = nullptr);
    ~ImportDialog() override;

    /**
     * @brief Получить заданные опции импортирования
     */
    BusinessLayer::ImportOptions importOptions() const;

signals:
    /**
     * @brief Пользователь хочет импортировать сценарий с заданными параметрами
     */
    void importRequested();

    /**
     * @brief Пользователь передумал импортировать данные
     */
    void canceled();

protected:
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

    /**
     * @brief Переопределяем, для ручной корректировки цепочки фокусирования виджетов
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
