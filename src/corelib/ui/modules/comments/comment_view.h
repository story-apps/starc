#pragma once

#include <ui/widgets/widget/widget.h>

namespace Ui {

/**
 * @brief Виджет отобразающий информацию о заголовке комментария
 */
class CommentView : public Widget
{
    Q_OBJECT

public:
    explicit CommentView(QWidget* _parent = nullptr);
    ~CommentView() override;

    /**
     * @brief Задать индекс комментария, который должен быть отображён
     */
    void setCommentIndex(const QModelIndex& _index);

    /**
     * @brief Идеальный размер определяем через делегат
     */
    int heightForWidth(int _width) const override;

signals:
    /**
     * @brief Пользователь кликнул на виджете
     */
    void clicked();

protected:
    /**
     * @brief Рисуем посредством делегата
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для открытия ссылки при клике на виджет
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
