#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

/**
 * @brief Представление приложения
 */
class ApplicationView : public Widget
{
    Q_OBJECT

public:
    explicit ApplicationView(QWidget* _parent = nullptr);
    ~ApplicationView() override;

    /**
     * @brief Показать заданный контент
     */
    void showContent(QWidget* _toolbar, QWidget* _navigator, QWidget* _view);

protected:
    /**
     * @brief Обновляем навигатор при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
