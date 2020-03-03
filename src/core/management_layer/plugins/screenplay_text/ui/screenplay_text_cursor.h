#pragma once

#include <QTextCursor>


namespace Ui {

/**
 * @brief Класс курсора со вспомогательными функциями
 */
class ScreenplayTextCursor : public QTextCursor
{
public:
    ScreenplayTextCursor();
    ScreenplayTextCursor(const QTextCursor &other);
    explicit ScreenplayTextCursor(QTextDocument* _document);
    ~ScreenplayTextCursor();

    /**
     * @brief Находится ли блок в таблице
     */
    bool isBlockInTable() const;
};

} // namespace Ui
