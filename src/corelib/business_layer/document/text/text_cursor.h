#pragma once

#include <corelib_global.h>

#include <QTextCursor>

class BaseTextEdit;


namespace BusinessLayer {

/**
 * @brief Класс курсора со вспомогательными функциями
 */
class CORE_LIBRARY_EXPORT TextCursor : public QTextCursor
{
public:
    TextCursor();
    TextCursor(const QTextCursor &other);
    explicit TextCursor(QTextDocument* _document);
    ~TextCursor();

    /**
     * @brief Находимся ли в данный момент в режиме редактирования
     */
    bool isInEditBlock() const;

    /**
     * @brief Получить интервал выделения в нормализованном виде (from <= to)
     */
    struct Selection {
        int from = 0;
        int to = 0;
    };
    Selection selectionInterval() const;

    /**
     * @brief Завершить текущую операцию редактирования и присоединить к ней новую
     */
    void restartEditBlock();
};

} // namespace Ui
