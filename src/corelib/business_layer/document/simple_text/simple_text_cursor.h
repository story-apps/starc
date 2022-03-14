#pragma once

#include <QTextCursor>

#include <corelib_global.h>

class BaseTextEdit;


namespace BusinessLayer {

/**
 * @brief Класс курсора со вспомогательными функциями
 */
class CORE_LIBRARY_EXPORT SimpleTextCursor : public QTextCursor
{
public:
    SimpleTextCursor();
    SimpleTextCursor(const QTextCursor& other);
    explicit SimpleTextCursor(QTextDocument* _document);
    ~SimpleTextCursor();

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

} // namespace BusinessLayer
