#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>


namespace Ui {

/**
 * @brief Панель с информацией о соединении с сервером
 */
class ConnectionStatusToolBar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit ConnectionStatusToolBar(QWidget* _parent = nullptr);
    ~ConnectionStatusToolBar() override;

    /**
     * @brief Визуализировать состояние подключения
     */
    void setConnectionAvailable(bool _available);

signals:
    /**
     * @brief Пользователь хочет проверить соединение прямо сейчас
     */
    void checkConnectionPressed();

protected:
    /**
     * @brief Переопределяем для корректировки позиции и положения при изменениях родителя
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Рисуем крутящийся лоадер
     */
    void paintEventPostprocess(QPainter& _painter) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
