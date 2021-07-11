#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет панели инструментов приложения
 */
class CORE_LIBRARY_EXPORT AppBar : public Widget
{
    Q_OBJECT

public:
    explicit AppBar(QWidget* _parent = nullptr);
    ~AppBar() override;

    /**
     * @brief Сами определяем минимальный размер
     */
    QSize minimumSizeHint() const override;

protected:
    /**
     * @brief Переопределяем для отображения тултипов кнопок
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Реализуем собственное рисование
     */
    void paintEvent(QPaintEvent* _event) override;

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
