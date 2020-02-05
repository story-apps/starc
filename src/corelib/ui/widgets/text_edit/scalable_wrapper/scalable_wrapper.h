#pragma once

#include <QGraphicsView>

class QGestureEvent;
class PageTextEdit;


/**
 * @brief Обёртка над редактором текста, чтобы сделать его масштабируемым
 */
class ScalableWrapper : public QGraphicsView
{
    Q_OBJECT

public:
    ScalableWrapper(PageTextEdit* _editor, QWidget* _parent = 0);
    ~ScalableWrapper() override;

    /**
     * @brief Инициилизировать синхронизацию полос прокрутки обёртки и редактора
     */
    void initScrollBarsSyncing();

    /**
     * @brief Установить коэффициент масштабирование
     */
    void setZoomRange(qreal _zoomRange);

    /**
     * @brief Увеличить масштаб
     */
    void zoomIn();

    /**
     * @brief Уменьшить масштаб
     */
    void zoomOut();

signals:
    /**
     * @brief Изменился коэффициент масштабирования
     */
    void zoomRangeChanged(qreal _zoomRange) const;

    /**
     * @brief Сменилась позиция курсора оборачиваемого редактора текста
     */
    void cursorPositionChanged() const;

protected:
    /**
     * @brief Переопределяем для обработки жестов
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Переопределяется для реализации увеличения/уменьшения
     */
    void wheelEvent(QWheelEvent* _event) override;

    /**
     * @brief Обрабатываем жест увеличения масштаба
     */
    void gestureEvent(QGestureEvent* _event);

    /**
     * @brief Переопределяется для отлавливания контекстного меню текстового редактора
     */
    bool eventFilter(QObject* _object, QEvent* _event) override;

private:
    /**
     * @brief Включить/выключить синхронизацию полос прокрутки между редактором и представлением
     */
    void setupScrollingSynchronization(bool _needSync);

    /**
     * @brief Настроить полосы прокрутки в соответствии с полосами редактора текста
     */
    void syncScrollBarWithTextEdit(bool _syncPosition = true);

    /**
     * @brief Пересчитать размер редактора текста
     */
    void updateTextEditSize();

    /**
     * @brief Собственно масштабирование представления текстового редактора
     */
    void scaleTextEdit();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
