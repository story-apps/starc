#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Представление параметров блоков шаблона
 */
class ScreenplayTemplateParagraphsView : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayTemplateParagraphsView(QWidget* _parent = nullptr);
    ~ScreenplayTemplateParagraphsView() override;

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
