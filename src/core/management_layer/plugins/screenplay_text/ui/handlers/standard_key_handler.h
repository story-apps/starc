#pragma once

#include "abstract_key_handler.h"

class QString;

template <typename T>
class QVector;

namespace BusinessLayer {
    enum class ScreenplayParagraphType;
}


namespace KeyProcessingLayer
{

/**
 * @brief Реализация стандартного обработчика
 */
class StandardKeyHandler : public AbstractKeyHandler
{
public:
    explicit StandardKeyHandler(Ui::ScreenplayTextEdit* _editor);

protected:
    /**
     * @brief Получить стиль блока к которому переходить
     */
    /** @{ */
    static BusinessLayer::ScreenplayParagraphType jumpForTab(BusinessLayer::ScreenplayParagraphType _blockType);
    static BusinessLayer::ScreenplayParagraphType jumpForEnter(BusinessLayer::ScreenplayParagraphType _blockType);
    static BusinessLayer::ScreenplayParagraphType changeForTab(BusinessLayer::ScreenplayParagraphType _blockType);
    static BusinessLayer::ScreenplayParagraphType changeForEnter(BusinessLayer::ScreenplayParagraphType _blockType);
    /** @} */

    /**
     * @brief Реализация интерфейса AbstractKeyHandler
     */
    /** @{ */
    void handleDelete(QKeyEvent* _event = 0);
    void handleBackspace(QKeyEvent* _event = 0);
    void handleEscape(QKeyEvent* _event = 0);
    void handleUp(QKeyEvent* _event = 0);
    void handleDown(QKeyEvent* _event = 0);
    void handlePageUp(QKeyEvent* _event = 0);
    void handlePageDown(QKeyEvent* _event = 0);
    void handleOther(QKeyEvent* _event = 0);
    /** @} */

private:
    /**
     * @brief Переместить курсор вверх или вниз
     */
    void moveCursorUpDown(bool _up, bool _isShiftPressed);

    /**
     * @brief Удалить символы
     */
    void removeCharacters(bool _backward);

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
    FoldersToDelete findGroupCountsToDelete(int _topCursorPosition, int _bottomCursorPosition);

    /**
     * @brief Удалить пары стёртых групп
     * @param _isFirstGroupHeader
     * @param _groupHeadersCount
     * @param _groupFootersCount
     */
    void removeGroupsPairs(int _cursorPosition, const FoldersToDelete& _groupCountsToDelete);
};

} // namespace KeyProcessingLayer
