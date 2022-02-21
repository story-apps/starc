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
    explicit SceneHeadingHandler(Ui::ScreenplayTextEdit* _editor);

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
     * @brief Сохранить данные сцены
     */
    void storeSceneParameters() const;

private:
    /**
     * @brief Модель дополнений
     */
    QStringListModel* m_completerModel = nullptr;

    /**
     * @brief Можно ли показать подсказку
     */
    bool m_completionAllowed = true;
};

} // namespace KeyProcessingLayer
