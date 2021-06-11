#pragma once

#include "standard_key_handler.h"

#include <QString>


namespace KeyProcessingLayer {

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке ремарки
 */
class ParentheticalHandler : public StandardKeyHandler
{
public:
    explicit ParentheticalHandler(Ui::ScreenplayTextEdit* _editor);

protected:
    /**
     * @brief Реализация интерфейса AbstractKeyHandler
     */
    /** @{ */
    void handleEnter(QKeyEvent* _event = 0);
    void handleTab(QKeyEvent* _event = 0);
    void handleOther(QKeyEvent* _event = 0);
    /** @} */
};

} // namespace KeyProcessingLayer
