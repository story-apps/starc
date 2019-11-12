#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

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

    QSize sizeHint() const override;

protected:
    /**
     * @brief Реализуем отрисовку
     */
    void paintEvent(QPaintEvent *_event) override;

    /**
     * @brief Переопределяем для подготовки аватарки к отрисовке
     */
    void resizeEvent(QResizeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
