#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет панели выбора цвета
 */
class ColorPallete : public Widget
{
    Q_OBJECT

public:
    explicit ColorPallete(QWidget* _parent = nullptr);
    ~ColorPallete() override;

    /**
     * @brief Задать возможность удаления выбранного цвета, если выбрать его ещё раз
     */
    void setColorCanBeDeselected(bool _can);

    /**
     * @brief Текущий выбранный цвет
     */
    QColor selectedColor() const;
    void setSelectedColor(const QColor& _color);

    /**
     * @brief Добавить кастомный цвет
     */
    void addCustomColor(const QColor& _color);

    /**
     * @brief Сами определяем идеальный размер для виджета
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Пользователь выбрал цвет
     */
    void selectedColorChanged(const QColor& _color);

    /**
     * @brief Пользователь хочет добавить кастомный цвет
     */
    void addCustomColorPressed();

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
     * @brief Обрабатываем клики
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief При изменении дизайн системы, нужно перестроить палитру
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
