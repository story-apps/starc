#pragma once

#include <ui/widgets/app_bar/app_bar.h>


namespace Ui {

/**
 * @brief Панель инструментов настроек
 */
class ScreenplayTemplateToolBar : public AppBar
{
    Q_OBJECT

public:
    explicit ScreenplayTemplateToolBar(QWidget* _parent = nullptr);
    ~ScreenplayTemplateToolBar() override;

    /**
     * @brief Сделать действие параметров страницы активным
     */
    void checkPageSettings();

    /**
     * @brief Настроить видимость иконки отображения редактора титульной страницы
     */
    void setTitlePageVisible(bool _visible);

signals:
    /**
     * @brief Пользователь хочет выйти из параметров шаблона
     */
    void backPressed();

    /**
     * @brief Запрос на открытие параметров страницы
     */
    void pageSettingsPressed();

    /**
     * @brief Запрос на открытие шаблона титульной страницы
     */
    void titlePageSettingsPressed();

    /**
     * @brief Запрос на открытие параметров параграфов
     */
    void paragraphSettingsPressed();

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
