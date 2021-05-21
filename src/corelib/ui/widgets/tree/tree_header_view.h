#pragma once

#include <QHeaderView>

/**
 * @brief Собственная реализация заголовков для дерева
 */
class TreeHeaderView : public QHeaderView
{
    Q_OBJECT

public:
    explicit TreeHeaderView(QWidget* _parent = nullptr);

    QSize sizeHint() const override;

protected:
    /**
     * @brief Реализуем собственную отрисовку заголовка
     */
    void paintSection(QPainter* _painter, const QRect& _rect, int _section) const override;
};
