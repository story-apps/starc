#pragma once

#include "standard_key_handler.h"


namespace KeyProcessingLayer {

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке завершения папки
 */
class ActFooterHandler : public StandardKeyHandler
{
public:
    explicit ActFooterHandler(Ui::ScreenplayTreatmentEdit* _editor);

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
