#pragma once

#include <ui/design_system/design_system.h>
#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Виджет отображающий цвет и его комплиментарный вариант
 */
class ThemeColor : public Widget
{
    Q_OBJECT

public:
    explicit ThemeColor(QWidget* _parent = nullptr);
    ~ThemeColor() override;

    /**
     * @brief Основной цвет
     */
    QColor color() const;
    void setColor(const QColor& _color);

    /**
     * @brief Комплиментарный цвет
     */
    QColor onColor() const;
    void setOnColor(const QColor& _color);

    /**
     * @brief Задать название цвета
     */
    void setTitle(const QString& _title);

    /**
     * @brief Возваращаем необходимый размер
     */
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    /**
     * @brief Изменён основной цвет
     */
    void colorPressed(const QColor& _color);

    /**
     * @brief Изменён комплиментарный цвет
     */
    void onColorPressed(const QColor& _color);

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Показываем панель выбора цвета
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};


/**
 * @brief Панель для настройки темы приложения
 */
class ThemeSetupView : public Widget
{
    Q_OBJECT

public:
    explicit ThemeSetupView(QWidget* _parent = nullptr);
    ~ThemeSetupView() override;

    /**
     * @brief Задать исходную тему, с которой пользователь зашёл в виджет смены темы
     */
    void setSourceThemeHash(const QString& _themeHash);

    /**
     * @brief Отобразить/скрыть панель с анимацией
     */
    void showView();
    void hideView();

signals:
    /**
     * @brief Пользователь изменил цвета кастомной темы
     */
    void customThemeColorsChanged(const DesignSystem::Color& _color);

protected:
    /**
     * @brief Отлавливаем события кнопок цветов, чтобы скрывать попап выбора цвета при уходе фокуса
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
