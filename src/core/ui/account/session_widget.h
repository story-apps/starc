#pragma once

#include <ui/widgets/card/card.h>

namespace Domain {
struct SessionInfo;
}


namespace Ui {

/**
 * @brief Виджет сессии
 */
class SessionWidget : public Card
{
    Q_OBJECT

public:
    explicit SessionWidget(QWidget* _parent = nullptr);
    ~SessionWidget() override;

    /**
     * @brief Информация о сессии
     */
    Domain::SessionInfo sessionInfo() const;
    void setSessionInfo(const Domain::SessionInfo& _sessionInfo);

    /**
     * @brief Скрыть кнопку остановки сессии
     */
    void hideterminateButton();

signals:
    /**
     * @brief Пользователь хочет завершить все сессии, кроме текущей
     */
    void terminateOthersRequested();

    /**
     * @brief Пользователь хочеть завершить текущую сессию
     */
    void terminateRequested();

protected:
    /**
     * @brief Обновляем переводы
     */
    void updateTranslations() override;

    /**
     * @brief Переопределяем для настройки отступов лейаута
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
