#pragma once

#include <ui/widgets/widget/widget.h>

class CORE_LIBRARY_EXPORT Button : public Widget
{
    Q_OBJECT

public:
    explicit Button(QWidget* _parent = nullptr);
    ~Button() override;

    /**
     * @brief Задать иконку
     */
    void setIcon(const QString& _icon);

    /**
     * @brief Задать текст кнопки
     */
    void setText(const QString& _text);

    /**
     * @brief Установить необходимость залить фон кнопки
     */
    void setContained(bool _contained);

    /**
     * @brief Установить необходимость нарисовать рамку кнопки
     */
    void setOutlined(bool _outlined);

    /**
     * @brief Установить необходимость нарисовать плоский фон
     */
    void setFlat(bool _flat);

    /**
     * @brief Имитировать клик пользователя на кнопке для испускания сигнала
     */
    void click();

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
     * @brief Переопределяем для реализации эффекта поднятия виджета при ховере
     */
    void enterEvent(QEnterEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

    /**
     * @brief Анимируем клик на кнопке
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Реализуем испускание сигнала, при нажатии на кнопке
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем, чтобы нажатие пробела и энтера активировало кнопку
     */
    void keyPressEvent(QKeyEvent* _event) override;

    /**
     * @brief Переопределяем для корректировки собственных размеров
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
