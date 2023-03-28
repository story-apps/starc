#ifndef SIDESLIDEDECORATOR_H
#define SIDESLIDEDECORATOR_H

#include <QTimeLine>
#include <QWidget>


/**
 * Widgets Animation Framework
 */
namespace WAF {
/**
 * @brief Класс декорирующий анимацию выкатывания
 */
class SideSlideDecorator : public QWidget
{
    Q_OBJECT

public:
    explicit SideSlideDecorator(QWidget* _parent);

    /**
     * @brief Сохранить изображение родительского виджета
     */
    void grabParentSize();

    /**
     * @brief Задекорировать фон
     */
    void decorate(bool _dark);

signals:
    /**
     * @brief На виджете произведён щелчёк мышью
     */
    void clicked();

protected:
    /**
     * @brief Переопределяется для прорисовки декорации
     */
    void paintEvent(QPaintEvent* _event);

    /**
     * @brief Переопределяется для отлавливания щелчка мышью
     */
    void mousePressEvent(QMouseEvent* _event);

private:
    /**
     * @brief Таймлайн для реализации анимированного декорирования
     */
    QTimeLine m_timeline;

    /**
     * @brief Фоновое изображение
     */
    QPixmap m_backgroundPixmap;

    /**
     * @brief Цвет декорирования фона
     */
    QColor m_decorationColor;
};
} // namespace WAF

#endif // SIDESLIDEDECORATOR_H
