#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет полосы оттенков цвета с возможностью выбора
 */
class ColorHueSlider : public Widget
{
    Q_OBJECT

public:
    explicit ColorHueSlider(QWidget* _parent = nullptr);
    ~ColorHueSlider() override;

    /**
     * @brief Установить оттенок
     */
    void setHue(qreal _hue);

signals:
    /**
     * @brief Изменился оттенок
     */
    void hueChanged(qreal hue);

protected:
    /**
     * @brief Определяем собственное рисование
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Корректируем положение ползунка при действиях пользователя
     */
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseMoveEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Корректируем отрисовку при изменении размера
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Обновляем закешированные данные
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
