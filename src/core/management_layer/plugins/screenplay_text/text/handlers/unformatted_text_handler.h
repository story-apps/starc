#pragma once

#include "standard_key_handler.h"


namespace KeyProcessingLayer
{

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке неформатированный текст
 */
class UnformattedTextHandler : public StandardKeyHandler
{
public:
    explicit UnformattedTextHandler(Ui::ScreenplayTextEdit* _editor);

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
