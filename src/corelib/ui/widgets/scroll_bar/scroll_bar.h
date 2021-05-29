#pragma once

#include <corelib_global.h>

#include <QScrollBar>


/**
 * @brief Виджет полосы прокрутки
 */
class CORE_LIBRARY_EXPORT ScrollBar : public QScrollBar
{
    Q_OBJECT

public:
    explicit ScrollBar(QWidget* _parent = nullptr);
    ~ScrollBar() override;

    /**
     * @brief  Устанавливаем цвет хэндла
     */
    void setHandleColor(const QColor &);

    /**
     * @brief  Устанавливаем цвет фона
     */
    void setBackgroundColor(const QColor &);

    /**
     * @brief Используем размеры из дизайн системы
     */
    QSize sizeHint() const override;
protected:
    /**
     * @brief Реализуем собственное рисование
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для анимирования ширины/высоты
     */
    void enterEvent(QEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
