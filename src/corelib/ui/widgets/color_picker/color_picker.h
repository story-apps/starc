#pragma once

#include "../widget/widget.h"


/**
 * @brief Виджет панели выбора цвета
 */
class ColorPicker : public Widget
{
    Q_OBJECT

public:
    explicit ColorPicker(QWidget* _parent = nullptr);
    ~ColorPicker() override;

    /**
     * @brief Текущий выбранный цвет
     */
    QColor selectedColor() const;
    void setSelectedColor(const QColor& _color);

signals:
    /**
     * @brief Пользователь выбрал цвет
     */
    void colorSelected(const QColor& _color);

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Рисуем обводку вокруг цвета над которым находится курсор
     */
    void mouseMoveEvent(QMouseEvent* _event) override;

    /**
     * @brief При изменении дизайн системы, нужно перестроить палитру
     */
    void designSystemChangeEvent(DesignSystemChangeEvent *_event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
