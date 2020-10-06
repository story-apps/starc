#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>


namespace Ui
{

/**
 * @brief Обёртка для анимирования тулбара
 */
class ToolbarAnimationWrapper : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit ToolbarAnimationWrapper(QWidget* _parent);
    ~ToolbarAnimationWrapper() override;

    /**
     * @brief Анимировать переход от одного тулбара к другому
     */
    void animateToolbarShowing(const QPointF _sourceIconPosition, QWidget* _sourceWidget, QWidget* _targetWidget);
    void animateToolbarHiding();

protected:
    /**
     * @brief Рисуем анимированное состояние сменяющихся тулбаров
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};


/**
 * @brief Панель поиска по тексту сценария
 */
class ScreenplayTextSearchToolbar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit ScreenplayTextSearchToolbar(QWidget* _parent = nullptr);
    ~ScreenplayTextSearchToolbar() override;

signals:
    void closePressed();
    void searchTextChanged(const QString& _text);
    void goToNextPressed();
    void goToPreviousPressed();
    void matchCasePressed(bool _caseSensitive);

protected:
    /**
     * @brief Ловим событие изменения размера родительского виджета, чтобы скорректировать свои размеры
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Фокусируем нужный элемент при получении фокуса
     * @note Не делаем это через проксифокус, т.к. нужно реагировать и на событие выхода фокуса из панели
     */
    void focusInEvent(QFocusEvent* _event) override;

    /**
     * @brief Скрываем попап, когда фокус ушёл из виджета
     */
    void focusOutEvent(QFocusEvent* _event) override;

    /**
     * @brief Корректируем цвета вложенных виджетов
     */
    void processBackgroundColorChange() override;
    void processTextColorChange() override;

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
