#pragma once

class QString;
class QWidget;


/**
 * @brief Вспомогательный класс для отображения стандартных диалогов
 */
class StandardDialog
{
public:
    static void information(QWidget* _parent, const QString& _title, const QString& _text);

private:
    StandardDialog() {}
};
