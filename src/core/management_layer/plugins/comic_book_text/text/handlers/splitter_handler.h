#pragma once

#include "standard_key_handler.h"


namespace KeyProcessingLayer {

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке кадр
 */
class SplitterHandler : public StandardKeyHandler
{
public:
    explicit SplitterHandler(Ui::ComicBookTextEdit* _editor)
        : StandardKeyHandler(_editor)
    {
    }

protected:
    /**
     * @brief Реализация интерфейса AbstractKeyHandler
     */
    /** @{ */
    void handleEnter(QKeyEvent* = nullptr) override
    {
    }
    void handleTab(QKeyEvent* = nullptr) override
    {
    }
    /** @} */
};

} // namespace KeyProcessingLayer
