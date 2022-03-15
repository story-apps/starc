#pragma once

#include <QTextCursor>

#include <corelib_global.h>

class BaseTextEdit;

namespace BusinessLayer {

class TextTemplate;

/**
 * @brief Класс курсора со вспомогательными функциями
 */
class CORE_LIBRARY_EXPORT TextCursor : public QTextCursor
{
public:
    TextCursor();
    TextCursor(const QTextCursor& other);
    explicit TextCursor(QTextDocument* _document);
    ~TextCursor();

    /**
     * @brief Находимся ли в данный момент в режиме редактирования
     */
    bool isInEditBlock() const;

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
     * @brief Завершить текущую операцию редактирования и присоединить к ней новую
     */
    void restartEditBlock();

    /**
     * @brief Удалить символы в заданном редакторе сценария
     */
    void removeCharacters(BaseTextEdit* _editor);
    void removeCharacters(bool _backward, BaseTextEdit* _editor);

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

    /**
     * @brief Получить шаблон оформления используемый для документа
     */
    virtual const TextTemplate& textTemplate() const;
};

} // namespace BusinessLayer
