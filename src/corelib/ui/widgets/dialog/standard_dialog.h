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
    static void information(QWidget* _parent, const QString& _title, const QString& _text);

private:
    StandardDialog()
    {
    }
};
