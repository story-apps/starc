#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет счётчика для дэшборда
 */
class CounterWidget : public Widget
{
    Q_OBJECT

public:
    enum class ValueStyle {
        OneLine,
        TwoLines,
    };

public:
    explicit CounterWidget(QWidget* _parent = nullptr);
    ~CounterWidget() override;

    /**
     * @brief Задать стиль отображения значения
     */
    void setValueStyle(ValueStyle _style);

    /**
     * @brief Задать данные счётчика
     */
    void setColor(const QColor& _color);
    void setLabel(const QString& _label);
    void setValue(const QString& _value);

    /**
     * @brief Переопределяем рассчёт идеального размера для виджета
     */
    QSize sizeHint() const override;

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
