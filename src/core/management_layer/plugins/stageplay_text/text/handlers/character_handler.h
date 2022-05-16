#pragma once

#include "standard_key_handler.h"

class QStringListModel;


namespace KeyProcessingLayer {

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке персонажа
 */
class CharacterHandler : public StandardKeyHandler
{
public:
    explicit CharacterHandler(Ui::StageplayTextEdit* _editor);

    /**
     * @brief При входе в блок, пробуем определить персонажа, который будет говорить
     */
    void prehandle() override;

protected:
    /**
     * @brief Реализация интерфейса AbstractKeyHandler
     */
    /** @{ */
    void handleEnter(QKeyEvent* _event = nullptr) override;
    void handleTab(QKeyEvent* _event = nullptr) override;
    void handleBackspace(QKeyEvent* _event = 0) override;
    void handleOther(QKeyEvent* _event = nullptr) override;
    void handleInput(QInputMethodEvent* _event) override;
    /** @} */

private:
    /**
     * @brief Показать автодополнение, если это возможно
     */
    void complete(const QString& _currentBlockText, const QString& _cursorBackwardText);

    /**
     * @brief Сохранить персонажа
     */
    void storeCharacter() const;

private:
    /**
     * @brief Модель персонажей текущей сцены
     */
    QStringListModel* m_completerModel = nullptr;

    /**
     * @brief Можно ли показать подсказку
     */
    bool m_completionAllowed = true;
};

} // namespace KeyProcessingLayer
