#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

class ScreenplayTextView : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayTextView(QWidget* _parent = nullptr);
    ~ScreenplayTextView() override;

    void setText(const QString& _text);
    Q_SIGNAL void textChanged(const QString& _text);

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
