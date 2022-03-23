#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

/**
 * @brief Диалог генератора логлайна
 */
class CORE_LIBRARY_EXPORT LoglineGeneratorDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit LoglineGeneratorDialog(QWidget* _parent = nullptr);
    ~LoglineGeneratorDialog() override;

    /**
     * @brief Получить текст логлайна
     */
    QString logline() const;

signals:
    /**
     * @brief Пользователь нажал кнопку использовать логлайн
     */
    void donePressed();

protected:
    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

    /**
     * @brief Опеределим последний фокусируемый виджет в диалоге
     */
    QWidget* lastFocusableWidget() const override;

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
