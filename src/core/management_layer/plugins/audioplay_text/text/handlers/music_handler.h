#pragma once

#include "standard_key_handler.h"

#include <QString>


namespace KeyProcessingLayer {

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке музыки
 */
class MusicHandler : public StandardKeyHandler
{
public:
    explicit MusicHandler(Ui::AudioplayTextEdit* _editor);

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
