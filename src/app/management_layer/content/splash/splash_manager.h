#pragma once

#include <QString>


/**
 * @brief Менеджер стартового экрана
 */
class SplashManager
{
public:
    static QString toolBar();
    static QString navigator();
    static QString view();

    SplashManager();
};
