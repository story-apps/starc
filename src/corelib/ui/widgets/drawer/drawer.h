#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет выезжающего меню
 */
class CORE_LIBRARY_EXPORT Drawer : public Widget
{
    Q_OBJECT

public:
    explicit Drawer(QWidget* _parent = nullptr);
    ~Drawer() override;

    /**
     * @brief Установить необходимость использования панели аккаунта
     */
    void setAccountVisible(bool _use);

    /**
     * @brief Параметры панели аккаунта
     */
    void setAvatar(const QPixmap& _avatar);
    void setAccountName(const QString& _name);
    void setAccountEmail(const QString& _email);

    /**
     * @brief Определяем идеальный размер
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Пользователь кликнул на аккаунте
     */
    void accountPressed();

protected:
    /**
     * @brief Реализуем собственное рисование
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для обработки нажатий пунктов меню
     */
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем для перерисовки выделенного пункта меню
     */
    void mouseMoveEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем, чтобы скорректировать собственный размер
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
