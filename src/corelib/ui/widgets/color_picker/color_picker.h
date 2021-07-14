#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

#include <corelib_global.h>


/**
 * @brief Виджет для отображения панелей выбора кастомного цвета
 */
class CORE_LIBRARY_EXPORT ColorPicker : public StackWidget
{
    Q_OBJECT

public:
    explicit ColorPicker(QWidget* _parent = nullptr);
    ~ColorPicker() override;

    /**
     * @brief Задать текущий выбранный цвет
     */
    void setSelectedColor(const QColor& _color);

signals:
    /**
     * @brief Пользователь выбрал цвет
     */
    void colorSelected(const QColor& _color);

    /**
     * @brief Пользователь передумал добавлять цвет
     */
    void cancelPressed();

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Настроить внешний вид в соответствии с дизайн системой
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
