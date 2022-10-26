#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

class ThemeSetupView;

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
     * @brief Получить виджет настройки темы
     */
    ThemeSetupView* themeSetupView() const;

    /**
     * @brief Получить виджет основного представления
     */
    QWidget* view() const;

    /**
     * @brief Выдвинуть представление
     */
    void slideViewOut();

    /**
     * @brief Задать доступность кнопки скрытия навигационной панели сплитера
     */
    void setHideNavigationButtonAvailable(bool _available);

    /**
     * @brief Сохранить состояние
     */
    QVariantMap saveState() const;

    /**
     * @brief Восстановить состояние
     */
    void restoreState(bool _onboaringPassed, const QVariantMap& _state);

    /**
     * @brief Показать заданный контент
     */
    void showContent(QWidget* _toolbar, QWidget* _navigator, QWidget* _view);

    /**
     * @brief Включить/отключить полноэкранный режим
     */
    void toggleFullScreen(bool _isFullScreen);

signals:
    /**
     * @brief Запрос на выход из полноэкранного режима
     */
    void turnOffFullScreenRequested();

    /**
     * @brief Запрос на закрытие приложения
     */
    void closeRequested();

protected:
    /**
     * @brief Переопределяем, чтобы вместо реального закрытия испустить сигнал о данном намерении
     */
    void closeEvent(QCloseEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем навигатор при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
