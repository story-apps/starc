#pragma once

#include <QTextCursor>

namespace Ui {
class ScreenplayTextEdit;
}


namespace BusinessLayer {

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
    bool inTable() const;

    /**
     * @brief Находится ли блок в первой колонке
     */
    bool inFirstColumn() const;

    /**
     * @brief Получить интервал выделения в нормализованном виде (from <= to)
     */
    struct Selection {
        int from = 0;
        int to = 0;
    };
    Selection selectionInterval() const;

    /**
     * @brief Удалить символы в заданном редакторе сценария
     */
    void removeCharacters(Ui::ScreenplayTextEdit* _editor);
    void removeCharacters(bool _backward, Ui::ScreenplayTextEdit* _editor);

private:
    /**
     * @brief Найти количество групповых элементов для удаления
     * @param _topCursorPosition
     * @param _bottomCursorPosition
     * @return
     *
     * 0 - заголовки групп сцен
     * 1 - окончания групп сцен
     * 2 - заголовки папок
     * 3 - окончания папок
     */
    struct FoldersToDelete {
        int headers = 0;
        int footers = 0;
    };
    FoldersToDelete findFoldersToDelete(int _topCursorPosition, int _bottomCursorPosition,
        bool isTopBlockShouldBeRemoved);

    /**
     * @brief Удалить пары стёртых групп
     * @param _isFirstGroupHeader
     * @param _groupHeadersCount
     * @param _groupFootersCount
     */
    void removeGroupsPairs(int _cursorPosition, const FoldersToDelete& _foldersToDelete,
        bool isTopBlockShouldBeRemoved);
};

} // namespace Ui
