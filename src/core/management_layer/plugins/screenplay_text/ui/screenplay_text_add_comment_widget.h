#pragma once

#include <ui/widgets/widget/widget.h>

namespace Ui
{

/**
 * @brief Виджет добавления комментария
 */
class ScreenplayTextAddCommentWidget : public Widget
{
public:
    explicit ScreenplayTextAddCommentWidget(QWidget* _parent = nullptr);
    ~ScreenplayTextAddCommentWidget() override;

protected:
    void updateTranslations() override;
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
