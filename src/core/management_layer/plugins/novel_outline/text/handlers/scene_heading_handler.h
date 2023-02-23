#pragma once

#include "standard_key_handler.h"

class QStringListModel;


namespace KeyProcessingLayer {

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке время и место
 */
class SceneHeadingHandler : public StandardKeyHandler
{
public:
    explicit SceneHeadingHandler(Ui::NovelOutlineEdit* _editor);

protected:
    /**
     * @brief Реализация интерфейса AbstractKeyHandler
     */
    /** @{ */
    void handleEnter(QKeyEvent* _event = nullptr) override;
    void handleTab(QKeyEvent* _event = nullptr) override;
    /** @} */
};

} // namespace KeyProcessingLayer
