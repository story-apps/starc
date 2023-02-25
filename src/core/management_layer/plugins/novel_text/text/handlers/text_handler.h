#pragma once

#include "standard_key_handler.h"


namespace KeyProcessingLayer {
/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке описание действия
 */
class TextHandler : public StandardKeyHandler
{
public:
    explicit TextHandler(Ui::NovelTextEdit* _editor);

protected:
    /**
     * @brief Реализация интерфейса AbstractKeyHandler
     */
    /** @{ */
    void handleEnter(QKeyEvent* _event = 0);
    void handleTab(QKeyEvent* _event = 0);
    /** @} */
};
} // namespace KeyProcessingLayer
