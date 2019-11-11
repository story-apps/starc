#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет плавающей панели инструментов
 */
class FloatingToolBar : public Widget
{
    Q_OBJECT

public:
    explicit FloatingToolBar(QWidget* _parent = nullptr);
    ~FloatingToolBar() override;

    /**
     * @brief Переопределяем, чтобы знать лучший размер
     */
    QSize sizeHint() const override;

protected:
    /**
     * @brief Реализуем собственное рисование
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для реализации эффекта поднятия виджета при ховере
     */
    void enterEvent(QEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

    /**
     * @brief Переопределяем, для анимации нажатия мыши на кнопке
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем, чтобы активировать нажатую кнопку
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем для обработки события смены дизайн-системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
