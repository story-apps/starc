#pragma once

#include <ui/widgets/widget/widget.h>


namespace BusinessLayer {
    class ScreenplayTextModel;
}

namespace Ui
{

class ScreenplayTextView : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayTextView(QWidget* _parent = nullptr);
    ~ScreenplayTextView() override;

    /**
     * @brief Установить модель сценария
     */
    void setModel(BusinessLayer::ScreenplayTextModel* _model);

protected:
    /**
     * @brief Переопределяем для корректировки положения тулбара действий над проектами
     */
    void resizeEvent(QResizeEvent* _event) override;

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
