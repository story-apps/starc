#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет для отображения прогресса в виде круга
 */
class CORE_LIBRARY_EXPORT CircularProgressBar : public Widget
{
    Q_OBJECT

public:
    explicit CircularProgressBar(QWidget* _parent = nullptr);
    ~CircularProgressBar() override;

    /**
     * @brief Задать цвет заполнения прогресса
     */
    void setBarColor(const QColor& _color);

    /**
     * @brief Задать значение прогресса
     */
    void setProgress(qreal _progress);

    /**
     * @brief Задать текст прогресса
     */
    void setText(const QString& _text);

    /**
     * @brief Переопределяем для корректного подсчёта размера в компоновщиках
     */
    QSize sizeHint() const override;

protected:
    /**
     * @brief Переопределяем, чтобы рисовать вручную
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
