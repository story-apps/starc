/*
 * Copyright (C) 2020  Dimka Novikov, to@dimkanovikov.pro
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * Full license: https://github.com/dimkanovikov/WidgetAnimationFramework/blob/master/LICENSE
 */

#pragma once

#include "../../WAF.h"
#include "../../AbstractAnimator.h"

class QPropertyAnimation;


/**
 * Widgets Animation Framework
 */
namespace WAF
{
    class CircleTransparentDecorator;

    /**
     * @brief Аниматор заполнения цветным кругом
     */
    class CircleTransparentAnimator : public AbstractAnimator
    {
        Q_OBJECT

    public:
        explicit CircleTransparentAnimator(QWidget* _widgetForFill);

        /**
         * @brief Установить точку начала анимации
         */
        void setStartPoint(const QPoint& _globalPoint);

        /**
         * @brief Установить картинку заливки
         */
        void setFillImage(const QPixmap& _image);

        /**
         * @brief Скрывать ли декоратор после окончания анимации
         */
        void setHideAfterFinish(bool _hide);

        /**
         * @brief Длительность анимации
         */
        int animationDuration() const;

        /**
         * @brief Заполнить виджет
         */
        /** @{ */
        void animateForward();
        void fillIn();
        /** @} */

        /**
         * @brief Свернуть цветовой круг - очистить виджет
         */
        /** @{ */
        void animateBackward();
        void fillOut();
        /** @} */

    private:
        /**
         * @brief Завершить выполнение анимации
         */
        void finalize();

        /**
         * @brief Скрыть декоратор
         */
        void hideDecorator();

        /**
         * @brief Получить виджет, который нужно заполнить
         */
        QWidget* widgetForFill() const;

    private:
        /**
         * @brief Декоратор, рисующий заполнение
         */
        CircleTransparentDecorator* m_decorator;

        /**
         * @brief Объект для анимирования декоратора
         */
        QPropertyAnimation* m_animation;

        /**
         * @brief Скрывать ли декоратор после завершения анимации
         */
        bool m_hideAfterFinish = true;
    };
}
