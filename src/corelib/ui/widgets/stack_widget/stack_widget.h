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
    enum class AnimationType { Fade, FadeThrough, Slide, Expand };

public:
    explicit StackWidget(QWidget* _parent = nullptr);
    ~StackWidget() override;

    /**
     * @brief Установить тип анимации виджета
     */
    void setAnimationType(AnimationType _type);

    /**
     * @brief Задать область анимации для конкретного виджета
     * @note Используется при анимации расширения/скрытия
     */
    void setAnimationRect(QWidget* _widget, const QRect& _animationRect);

    /**
     * @brief Добавить виджет в стек
     * @note При этом виджет не делается текущим
     */
    void addWidget(QWidget* _widget);

    /**
     * @brief Сделать заданный виджет текущим
     */
    void setCurrentWidget(QWidget* _widget);

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
     * @brief Длительность анимации
     */
    int animationDuration() const;

    /**
     * @brief Реализуем собственную отрисовку для реализации эффекста смены текущего виджета
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Реализацем собственную отрисовку, чтобы корректировать размер вложенных виджетов
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Отлавливаем запрос на перекомпоновку и прокидываем его выше
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
