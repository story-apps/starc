#pragma once

#include <QGraphicsView>

#include <corelib_global.h>

class QGestureEvent;


/**
 * @brief Область отрисовки графических элементов с масштабированием и улучшенной прокруткой
 */
class CORE_LIBRARY_EXPORT ScalableGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ScalableGraphicsView(QWidget* _parent = nullptr);

    /**
     * @brief Задать доступность возможности изменения масштаба
     */
    void setScaleAvailable(bool _available);

    /**
     * @brief Методы масштабирования
     */
    /** @{ */
    void zoomIn();
    void zoomOut();
    /** @} */

    /**
     * @brief Центрировать вьюпорт на заданном элементе
     */
    void animateCenterOn(QGraphicsItem* _item);

    /**
     * @brief Убедиться, что элемент на экране, если нет, анимированно сместить экран так, чтобы
     *        элемент был виден
     */
    void animateEnsureVisible(QGraphicsItem* _item);

    /**
     * @brief Сохранить текущее состояние
     */
    QByteArray saveState() const;

    /**
     * @brief Загрузить состояние
     */
    virtual void restoreState(const QByteArray& _state);

    /**
     * @brief Сохранить сцену в картинку
     */
    QImage saveSceneToPng(qreal _scaleFactor = 2.0);

signals:
    /**
     * @brief Изменился масштаб
     */
    void scaleChanged();

    /**
     * @brief Нажата кнопка Delete или Backspace
     */
    void deletePressed();

protected:
    /**
     * @brief Переопределяем для обработки жестов
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Обрабатываем жест увеличения масштаба
     */
    void gestureEvent(QGestureEvent* _event);

    /**
     * @brief Переопределяем для того, чтобы избежать автоматического центрирования сцены
     */
    void showEvent(QShowEvent* _event) override;

    /**
     * @brief Обрабатываем изменение масштаба при помощи ролика
     */
    void wheelEvent(QWheelEvent* _event) override;

    /**
     * @brief Переопределяем для обработки горячих клавиш изменения масштаба и скроллинга
     */
    /** @{ */
    void keyPressEvent(QKeyEvent* _event) override;
    void keyReleaseEvent(QKeyEvent* _event) override;
    /** @} */

    /**
     * @brief Реализация перетаскивания/прокрутки
     */
    /** @{ */
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseMoveEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;
    /** @} */

private:
    /**
     * @brief Масштабировать представление на заданный коэффициент
     */
    void scaleView(qreal _factor);

private:
    /**
     * @brief Можно ли масштабировать содержимое
     */
    bool m_isScaleAvailable = true;

    /**
     * @brief Последняя позиция мыши в момент скроллинга полотна
     */
    QPoint m_scrollingLastPos;

    /**
     * @brief Происходит ли в данный момент скроллинг с зажатым пробелом
     */
    bool m_inScrolling = false;

    /**
     * @brief Инерционный тормоз масштабирования при помощи жестов
     */
    int m_gestureZoomInertionBreak = 0;
};
