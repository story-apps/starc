#pragma once

#include <QParallelAnimationGroup>

#include <corelib_global.h>


/**
 * @brief Анимация клика на объекте
 */
class ClickAnimation : public QParallelAnimationGroup
{
    Q_OBJECT

public:
    explicit ClickAnimation(QObject* _parent = nullptr);
    ~ClickAnimation() override;

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
