#pragma once

#include <QWidget>

class DesignSystemChangeEvent;


/**
 * @brief Базовый виджет
 */
class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget* _parent = nullptr);
    ~Widget() override;

    /**
     * @brief Цвет фона виджета
     */
    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& _color);

    /**
     * @brief Цвет текста виджета
     */
    QColor textColor() const;
    void setTextColor(const QColor& _color);

protected:
    /**
     * @brief Переопределяем для обработки события обновления дизайн системы
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Переопределяем для собственной реализации отрисовки - по сути заливаем цветом фона
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Событие реакция на изменение дизайн системы
     * @note Реализация по-умолчание просто обновляет весь виджет
     */
    virtual void designSysemChangeEvent(DesignSystemChangeEvent* _event);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
