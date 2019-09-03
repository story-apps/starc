#pragma once

#include <QWidget>


namespace Ui
{

/**
 * @brief Представление приложения
 */
class ApplicationView : public QWidget
{
    Q_OBJECT

public:
    explicit ApplicationView(QWidget* _parent = nullptr);
    ~ApplicationView() override;

    /**
     * @brief Показать заданный контент
     */
    void showContent(QWidget* _toolbar, QWidget* _navigator, QWidget* _view);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
