#pragma once

#include "standard_key_handler.h"

class QStringListModel;


namespace KeyProcessingLayer
{

/**
 * @brief Класс выполняющий обработку нажатия клавиш в блоке участники сцени
 */
class SceneCharactersHandler : public StandardKeyHandler
{
public:
    explicit SceneCharactersHandler(Ui::ScreenplayTextEdit* _editor);

protected:
    /**
     * @brief Реализация интерфейса AbstractKeyHandler
     */
    /** @{ */
    void handleEnter(QKeyEvent* _event = 0) override;
    void handleTab(QKeyEvent* _event = 0) override;
    void handleOther(QKeyEvent* _event = 0) override;
    void handleInput(QInputMethodEvent* _event) override;
    /** @} */

private:
    /**
     * @brief Показать автодополнение, если это возможно
     */
    void complete(const QString& _currentBlockText, const QString& _cursorBackwardText);

    /**
     * @brief Сохранить персонажей
     */
    void storeCharacters() const;

    /**
     * @brief Отфильтрованная модель персонажей
     */
    QStringListModel* m_filteredCharactersModel = nullptr;
};

} // namespace KeyProcessingLayer
