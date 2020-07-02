#pragma once

#include "../widget/widget.h"


/**
 * @brief Виджет реализующий стэк вложенных виджетов, один из которых является видимым
 */
class CORE_LIBRARY_EXPORT StackWidget : public Widget
{
    Q_OBJECT

public:
    /**
     * @brief Тип анимации контента
     */
    enum class AnimationType {
        Fade,
        Slide
    };

public:
    explicit StackWidget(QWidget* _parent = nullptr);
    ~StackWidget() override;

    /**
      * @brief Установить тип анимации виджета
      */
    void setAnimationType(AnimationType _type);

    /**
     * @brief Сделать заданный виджет текущим
     */
    void setCurrentWidget(QWidget* widget);

    /**
     * @brief Получить текущий виджет
     */
    QWidget* currentWidget() const;

    /**
     * @brief Определяем по желаемому размеру текущего виджета
     */
    QSize sizeHint() const override;

protected:
    /**
     * @brief Реализуем собственную отрисовку для реализации эффекста смены текущего виджета
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Реализацем собственную отрисовку, чтобы корректировать размер вложенных виджетов
     */
    void resizeEvent(QResizeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
