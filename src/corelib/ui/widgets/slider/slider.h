#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет слайдера
 */
class CORE_LIBRARY_EXPORT Slider : public Widget
{
    Q_OBJECT

public:
    explicit Slider(QWidget* _parent = nullptr);
    ~Slider() override;

    /**
     * @brief Задать максимальное значение слайдера
     */
    void setMaximumValue(int _value);

    /**
     * @brief Задать текущее значение слайдера
     */
    void setValue(int _value);

    /**
     * @brief Переопределяем для корректного подсчёта размера в компоновщиках
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Значение слайдера изменилось
     */
    void valueChanged(int _value);

protected:
    /**
     * @brief Переопределяем для собственной отрисовки
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для установки позиции ползунка в месте указанном пользователем
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем для обработки изменения позиции ползунка
     */
    void mouseMoveEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем для определения момента отпускания мыши и скрытия области клика
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем, чтобы нажатие стрелок лево и право изменяло значение
     */
    void keyPressEvent(QKeyEvent* _event) override;

    /**
     * @brief Переопределяем для обработки события смены дизайн-системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    /**
     * @brief Обновить значение в соответствии с позицией мыши
     */
    void updateValue(const QPoint& _mousePosition);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
