#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

class QAbstractItemModel;


namespace Ui
{

/**
 * @brief Виджет комментариев сценария
 */
class ScreenplayTextCommentsView : public StackWidget
{
    Q_OBJECT

public:
    explicit ScreenplayTextCommentsView(QWidget* _parent = nullptr);
    ~ScreenplayTextCommentsView() override;

    /**
     * @brief Показать виджет добавления/редактирования комментария
     */
    void showAddCommentView(const QColor& _withColor);

signals:
    /**
     * @brief Пользователь хочет добавить комментарий
     */
    void addCommentRequested(const QColor& _color, const QString& _text);

protected:
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

