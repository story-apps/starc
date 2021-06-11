#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Виджет для отображения аватара в личном кабинете
 */
class Avatar : public Widget
{
    Q_OBJECT

public:
    explicit Avatar(QWidget* _parent = nullptr);
    ~Avatar() override;

    /**
     * @brief Задать аватар
     */
    void setAvatar(const QPixmap& _avatar);

signals:
    /**
     * @brief Пользователь кликнул на аватарке
     */
    void clicked();

protected:
    /**
     * @brief Реализуем отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для подготовки аватарки к отрисовке
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Реализуем эффекст отображения оверлея при наведении мыши
     */
    void enterEvent(QEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

    /**
     * @brief Испускаем сигнал о клике пользователя
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Определяем переводы
     */
    void updateTranslations() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
