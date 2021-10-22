#pragma once

class QEvent;
class QInputMethodEvent;
class QKeyEvent;

namespace BusinessLayer {
class ScreenplayTemplate;
}

namespace Ui {
class ScreenplayTextEdit;
}


namespace KeyProcessingLayer {

/**
 * @brief Базовый класс обработчика нажатия клавиш
 */
class AbstractKeyHandler
{
public:
    explicit AbstractKeyHandler(Ui::ScreenplayTextEdit* _editor);
    virtual ~AbstractKeyHandler();

    /**
     * @brief Предварительная обработка
     */
    virtual void prehandle()
    {
    }

    /**
     * @brief Обработка события нажатия клавиши
     */
    void handle(QEvent* _event);

protected:
    /**
     * @brief Подготовка к обработке
     */
    virtual void prepareForHandle(QKeyEvent* = nullptr)
    {
    }

    /**
     * @brief Необходимые действия при нажатии конкретной клавиши/сочетания
     */
    /** @{ */
    virtual void handleEnter(QKeyEvent* _event = nullptr) = 0;
    virtual void handleTab(QKeyEvent* _event = nullptr) = 0;
    virtual void handleDelete(QKeyEvent* _event = nullptr) = 0;
    virtual void handleBackspace(QKeyEvent* _event = nullptr) = 0;
    virtual void handleEscape(QKeyEvent* _event = nullptr) = 0;
    virtual void handleUp(QKeyEvent* _event = nullptr) = 0;
    virtual void handleDown(QKeyEvent* _event = nullptr) = 0;
    virtual void handlePageUp(QKeyEvent* _event = nullptr) = 0;
    virtual void handlePageDown(QKeyEvent* _event = nullptr) = 0;
    virtual void handleOther(QKeyEvent* _event = nullptr) = 0;
    virtual void handleInput(QInputMethodEvent*)
    {
    }
    /** @} */

protected:
    /**
     * @brief Получить текстовый редактор, с которым ассоциирован данный обработчик
     */
    Ui::ScreenplayTextEdit* editor() const;

private:
    /**
     * @brief Обработать событие нажатия клавиши
     */
    void handleKeyEvent(QKeyEvent* _event);

private:
    /**
     * @brief Текстовый редактор, с которым ассоциирован данный обработчик
     */
    Ui::ScreenplayTextEdit* m_editor = nullptr;
};

} // namespace KeyProcessingLayer
