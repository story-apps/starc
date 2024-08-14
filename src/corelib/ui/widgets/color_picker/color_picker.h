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
     * @brief Задать возможность удаления выбранного цвета, если выбрать его ещё раз
     */
    void setColorCanBeDeselected(bool _can);

    /**
     * @brief Текущий выбранный цвет
     */
    QColor selectedColor() const;

    /**
     * @brief Задать текущий выбранный цвет
     */
    void setSelectedColor(const QColor& _color);

    /**
     * @brief Переопределяем идеальный размер, чтобы лейаут, в котором используется виджет,
     *        всегда знал о корректном размере
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Пользователь выбрал цвет
     */
    void selectedColorChanged(const QColor& _color);

protected:
    /**
     * @brief Следим за движением мыши по виджетам-оверлеям
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Реализуем реакцию на задание кастомного цвета
     */
    void processBackgroundColorChange() override;
    void processTextColorChange() override;

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
