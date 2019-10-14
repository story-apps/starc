#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui
{

/**
 * @brief Диалог регистрации/авторизации в личном кабинете пользователя
 */
class LoginDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* _parent = nullptr);
    ~LoginDialog() override;

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
