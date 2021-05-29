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
     * @brief Установить заголовок
     */
    void setTitle(const QString& _title);

    /**
     * @brief Установить подзаголовок
     */
    void setSubtitle(const QString& _subtitle);

    /**
     * @brief Определяем идеальный размер
     */
    QSize sizeHint() const override;

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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
