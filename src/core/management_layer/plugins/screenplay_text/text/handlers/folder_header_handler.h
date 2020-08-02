#pragma once

#include "standard_key_handler.h"


namespace KeyProcessingLayer
{

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке заголовка папки
 */
class FolderHeaderHandler : public StandardKeyHandler
{
public:
    FolderHeaderHandler(Ui::ScreenplayTextEdit* _editor);

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
