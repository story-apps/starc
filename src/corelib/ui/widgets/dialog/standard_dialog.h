#pragma once

#include <corelib_global.h>

class QString;
class QWidget;


/**
 * @brief Вспомогательный класс для отображения стандартных диалогов
 */
class CORE_LIBRARY_EXPORT StandardDialog
{
public:
    /**
     * @brief Сформировать нешаблонный текст для кнопки ОК
     */
    static QString generateOkTerm();

    /**
     * @brief Показать диалог с информацией и одной кнопкой
     */
    static void information(QWidget* _parent, const QString& _title, const QString& _text);

private:
    StandardDialog()
    {
    }
};
