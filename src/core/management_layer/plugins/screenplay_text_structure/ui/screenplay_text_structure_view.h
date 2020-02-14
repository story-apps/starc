#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

class ScreenplayTextStructureView : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayTextStructureView(QWidget* _parent = nullptr);
    ~ScreenplayTextStructureView() override;

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
