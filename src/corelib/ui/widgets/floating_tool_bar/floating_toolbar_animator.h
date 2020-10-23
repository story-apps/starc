#pragma once

#include "floating_tool_bar.h"


/**
 * @brief Обёртка для анимирования тулбаров
 */
class CORE_LIBRARY_EXPORT FloatingToolbarAnimator : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit FloatingToolbarAnimator(QWidget* _parent);
    ~FloatingToolbarAnimator() override;

    /**
     * @brief Анимировать переход от одного тулбара к другому
     */
    void switchToolbars(const QString& _targetIcon, const QPointF _targetIconStartPosition,
        QWidget* _sourceWidget, QWidget* _targetWidget);

    /**
     * @brief Осуществить обратный переход
     */
    void switchToolbarsBack();

protected:
    /**
     * @brief Рисуем анимированное состояние сменяющихся тулбаров
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
