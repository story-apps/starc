#pragma once

#include "standard_key_handler.h"


namespace KeyProcessingLayer
{

/**
 * @brief Класс выполняющий предварительную обработку нажатия клавиш в любом блоке
 */
class PreHandler : public StandardKeyHandler
{
public:
    explicit PreHandler(Ui::ScreenplayTitlePageEdit* _editor);

protected:
    /**
 * @brief Реализация интерфейса AbstractKeyHandler
 */
    /** @{ */
    void handleEnter(QKeyEvent* = 0) {}
    void handleTab(QKeyEvent* = 0) {}
    void handleBackspace(QKeyEvent* = 0) {}
    void handleEscape(QKeyEvent* = 0) {}
    void handleUp(QKeyEvent* = 0) {}
    void handleDown(QKeyEvent* = 0) {}

    void handleDelete(QKeyEvent* _event = 0);
    void handleOther(QKeyEvent* _event = 0);
    /** @} */
};

} // namespace KeyProcessingLayer
