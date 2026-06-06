#pragma once

#include <corelib_global.h>


class CORE_LIBRARY_EXPORT LanguageHelper
{
public:
    /**
     * @brief Преобразовать значения языка из перечисления Qt6 в Qt5
     */
    static int qt6LanguageIdToQt5Id(int _qt6Id);

    /**
     * @brief Преобразовать значения языка из перечисления Qt5 в Qt6
     */
    static int qt5LanguageIdToQt6Id(int _qt5Id);
};
