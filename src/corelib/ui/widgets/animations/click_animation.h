#pragma once

#include <QParallelAnimationGroup>

#include <corelib_global.h>


/**
 * @brief Анимация клика на объекте
 */
class CORE_LIBRARY_EXPORT ClickAnimation : public QParallelAnimationGroup
{
    Q_OBJECT

public:
    explicit ClickAnimation(QObject* _parent = nullptr);
    ~ClickAnimation() override;

    /**
     * @brief Настроить скорость анимации
     *        Быстрая - для радиокнопок, иконок и т.п., где маленькая область и быстрая анимация
     *        Долгая - для кнопок и т.п. элементов, где декорация занимает большую площадь
     */
    void setFast(bool _fast);

    /**
     * @brief Точка начала анимации
     */
    void setClickPosition(const QPointF& _position);
    QPointF clickPosition() const;

    /**
     * @brief Область в которой может отображаться декорация
     */
    void setClipRect(const QRectF& _rect);
    QRectF clipRect() const;

    /**
     * @brief Задать значения радиуса
     */
    void setRadiusInterval(qreal _from, qreal _to);

    /**
     * @brief Получить значение радиуса
     */
    qreal radius() const;
    qreal minimumRadius() const;
    qreal maximumRadius() const;

    /**
     * @brief Получить текущее значение прозрачности
     */
    qreal opacity() const;

signals:
    /**
     * @brief Какое-либо из значений обновилось
     */
    void valueChanged();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
