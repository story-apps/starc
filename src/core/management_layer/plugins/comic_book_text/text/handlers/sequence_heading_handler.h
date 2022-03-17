#pragma once

#include "standard_key_handler.h"


namespace KeyProcessingLayer {

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке заголовка папки
 */
class SequenceHeadingHandler : public StandardKeyHandler
{
public:
    SequenceHeadingHandler(Ui::ComicBookTextEdit* _editor);

protected:
    /**
     * @brief Реализация интерфейса AbstractKeyHandler
     */
    /** @{ */
    void handleEnter(QKeyEvent* _event = 0) override;
    void handleTab(QKeyEvent* _event = 0) override;
    /** @} */

private:
    /**
     * @brief Найти закрывающий блок и обновить его текст
     */
    void updateFooter();
};

} // namespace KeyProcessingLayer
