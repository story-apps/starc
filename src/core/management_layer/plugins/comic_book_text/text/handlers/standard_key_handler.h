#pragma once

#include "abstract_key_handler.h"

class QString;

namespace BusinessLayer {
enum class ComicBookParagraphType;
}


namespace KeyProcessingLayer {

/**
 * @brief Реализация стандартного обработчика
 */
class StandardKeyHandler : public AbstractKeyHandler
{
public:
    explicit StandardKeyHandler(Ui::ComicBookTextEdit* _editor);

protected:
    /**
     * @brief Получить стиль блока к которому переходить
     */
    /** @{ */
    static BusinessLayer::ComicBookParagraphType jumpForTab(
        BusinessLayer::ComicBookParagraphType _blockType);
    static BusinessLayer::ComicBookParagraphType jumpForEnter(
        BusinessLayer::ComicBookParagraphType _blockType);
    static BusinessLayer::ComicBookParagraphType changeForTab(
        BusinessLayer::ComicBookParagraphType _blockType);
    static BusinessLayer::ComicBookParagraphType changeForEnter(
        BusinessLayer::ComicBookParagraphType _blockType);
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
};

} // namespace KeyProcessingLayer
