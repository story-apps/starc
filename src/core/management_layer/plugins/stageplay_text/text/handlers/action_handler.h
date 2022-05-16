#pragma once

#include "standard_key_handler.h"


namespace KeyProcessingLayer {

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке сигнал
 */
class ActionHandler : public StandardKeyHandler
{
public:
    explicit ActionHandler(Ui::StageplayTextEdit* _editor);

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
