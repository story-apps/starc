#pragma once

#include <QScrollBar>


class ScrollBarPrivate;

/**
 * @brief Виджет полосы прокрутки
 */
class ScrollBar : public QScrollBar
{
    Q_OBJECT

public:
    explicit ScrollBar(QWidget* _parent = nullptr);
    ~ScrollBar() override;

    /**
     * @brief Используем размеры из дизайн системы
     */
    QSize sizeHint() const override;

protected:
    /**
     * @brief Реализуем собственное рисование
     */
    void paintEvent(QPaintEvent* _event) override;

    void enterEvent(QEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

private:
    /**
     * @brief Данные класса
     */
    QScopedPointer<ScrollBarPrivate> d;
};
