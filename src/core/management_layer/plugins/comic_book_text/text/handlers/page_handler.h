#pragma once

#include "standard_key_handler.h"

class QStringListModel;


namespace KeyProcessingLayer {

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке лирики
 */
class PageHandler : public StandardKeyHandler
{
public:
    explicit PageHandler(Ui::ComicBookTextEdit* _editor);

    /**
     * @brief При входе в блок, сразу показываем подсказку
     */
    void prehandle() override;

protected:
    /**
     * @brief Реализация интерфейса AbstractKeyHandler
     */
    /** @{ */
    void handleEnter(QKeyEvent* _event = nullptr) override;
    void handleTab(QKeyEvent* _event = nullptr) override;
    void handleBackspace(QKeyEvent* _event = nullptr) override;
    void handleOther(QKeyEvent* _event = nullptr) override;
    void handleInput(QInputMethodEvent* _event) override;
    /** @} */

private:
    /**
     * @brief Показать автодополнение, если это возможно
     */
    void complete(const QString& _currentBlockText, const QString& _cursorBackwardText);

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
