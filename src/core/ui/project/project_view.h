#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>


namespace Ui
{

/**
 * @brief Представление проекта
 */
class ProjectView : public StackWidget
{
    Q_OBJECT
public:
    explicit ProjectView(QWidget* _parent = nullptr);

signals:

public slots:
};

} // namespace Ui
