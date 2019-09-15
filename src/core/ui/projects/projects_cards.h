#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

/**
 * @brief Представление модели со списком проектов
 */
class ProjectsCards : public Widget
{
    Q_OBJECT

public:
    explicit ProjectsCards(QWidget* _parent = nullptr);
    ~ProjectsCards() override;

protected:
    void resizeEvent(QResizeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
