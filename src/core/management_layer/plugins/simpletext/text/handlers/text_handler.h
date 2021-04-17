#pragma once

#include "standard_key_handler.h"


namespace KeyProcessingLayer
{

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке текст
 */
class TextHandler : public StandardKeyHandler
{
public:
    explicit TextHandler(Ui::TextEdit* _editor);

protected:
    void handleEnter(QKeyEvent* _event = 0) override;
    void handleTab(QKeyEvent* _event = 0) override;
};

} // namespace KeyProcessingLayer
