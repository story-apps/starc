#pragma once

#include <ui/widgets/widget/widget.h>

class Button : public Widget
{
    Q_OBJECT

public:
    explicit Button(QWidget* _parent = nullptr);
    ~Button() override;

    /**
     * @brief Задать текст кнопки
     */
    void setText(const QString& _text);

signals:
    /**
     * @brief Кнопка была нажата
     */
    void clicked();

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Реализуем испускание сигнала, при нажатии на кнопке
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
