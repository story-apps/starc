#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>


namespace Ui
{

/**
 * @brief Виджет панели инструментов аккаунта (вход в личный кабинет + чаты)
 */
class AccountBar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit AccountBar(QWidget* _parent = nullptr);
    ~AccountBar() override;

    /**
     * @brief Установить аватарку
     */
    void setAvatar(const QPixmap& _avatar);

    /**
     * @brief Посигнализировать заданным цветом
     */
    void notify(const QColor& _color);

signals:
    /**
     * @brief Нажата основная кнопка панели
     */
    void accountPressed();

protected:
    /**
     * @brief Переопределяем для рисования анимации сигнализации
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
