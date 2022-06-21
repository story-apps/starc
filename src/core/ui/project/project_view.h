#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>


namespace Ui {

/**
 * @brief Представление проекта
 */
class ProjectView : public StackWidget
{
    Q_OBJECT
public:
    explicit ProjectView(QWidget* _parent = nullptr);
    ~ProjectView() override;

    /**
     * @brief Показать умолчальную страницу
     */
    void showDefaultPage();

    /**
     * @brief Показать страницу с информацией о том, что это ещё не реализовано
     */
    void showNotImplementedPage();

    /**
     * @brief Активно ли представление в данный момент
     */
    void setActive(bool _active);

signals:
    /**
     * @brief Пользователь нажал кнопку создания нового проекта
     */
    void createNewItemPressed();

protected:
    /**
     * @brief Отслеживаем изменение размера, чтобы скорректировать размер оверлея
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
