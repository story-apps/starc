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

    /**
     * @brief Установить необходимость залить фон кнопки
     */
    void setContained(bool _contained);

    /**
     * @brief Переопределяем размер для правильного размещения в компоновщиках
     */
    QSize sizeHint() const override;

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
     * @brief Анимируем клик на кнопке
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Реализуем испускание сигнала, при нажатии на кнопке
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем для корректировки собственных размеров
     */
    void designSysemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
