#pragma once

#include <corelib_global.h>

#include <ui/widgets/card/card.h>


/**
 * @brief Виджет контекстного меню
 */
class CORE_LIBRARY_EXPORT ContextMenu : public Card
{
    Q_OBJECT

public:
    explicit ContextMenu(QWidget* _parent = nullptr);
    ~ContextMenu() override;

    /**
     * @brief Задать пункты контекстного меню
     */
    void setActions(const QVector<QAction*>& _actions);

    /**
     * @brief Отобразить контекстное меню
     * @param _pos - глобальная координата для отображения меню
     */
    void showContextMenu(const QPoint& _pos);

    /**
     * @brief Скрыть контекстное меню
     */
    void hideContextMenu();

protected:
    /**
     * @brief Реализуем ручную отрисовку пунктов меню
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

