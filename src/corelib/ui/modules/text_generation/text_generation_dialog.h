#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

/**
 * @brief Диалог генерации текста
 */
class CORE_LIBRARY_EXPORT TextGenerationDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit TextGenerationDialog(QWidget* _parent = nullptr);
    ~TextGenerationDialog() override;

    /**
     * @brief Задать параметры диалога
     */
    void setOptions(const QString& _title, int _credits, const QString& _promptLabel,
                    const QString& _prompt);

signals:
    /**
     * @brief Пользователь нажал кнопку генерировать с заданным запросом
     */
    void generatePressed(const QString& _prompt);

    /**
     * @brief Нажата кнопка покупки кредитов
     */
    void buyCreditsPressed();

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
