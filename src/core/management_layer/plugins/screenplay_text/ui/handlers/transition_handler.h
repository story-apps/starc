#ifndef TRANSITIONHANDLER_H
#define TRANSITIONHANDLER_H

#include "standard_key_handler.h"


namespace KeyProcessingLayer
{
    /**
     * @brief Класс выполняющий обработку нажатия клавиш в блоке переход
     */
    class TransitionHandler : public StandardKeyHandler
    {
    public:
        TransitionHandler(UserInterface::ScenarioTextEdit* _editor);

    protected:
        /**
         * @brief Реализация интерфейса AbstractKeyHandler
         */
        /** @{ */
        void handleEnter(QKeyEvent* _event = nullptr) override;
        void handleTab(QKeyEvent* _event = nullptr) override;
        void handleOther(QKeyEvent* _event = nullptr) override;
        void handleInput(QInputMethodEvent* _event) override;
        /** @} */

    private:
        /**
         * @brief Показать автодополнение, если это возможно
         */
        void complete(const QString& _currentBlockText, const QString& _cursorBackwardText);

        /**
         * @brief Сохранить переход
         */
        void storeTransition() const;
    };
}

#endif // TRANSITIONHANDLER_H
