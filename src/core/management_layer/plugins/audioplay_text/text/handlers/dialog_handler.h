#pragma once

#include "standard_key_handler.h"


namespace KeyProcessingLayer {

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке реплики
 */
class DialogHandler : public StandardKeyHandler
{
public:
    explicit DialogHandler(Ui::AudioplayTextEdit* _editor);

protected:
    /**
     * @brief Реализация интерфейса AbstractKeyHandler
     */
    /** @{ */
    void handleEnter(QKeyEvent* _event = 0);
    void handleTab(QKeyEvent* _event = 0);
    void handleBackspace(QKeyEvent* _event = 0) override;
    /** @} */
};

} // namespace KeyProcessingLayer
