#pragma once

#include <ui/widgets/widget/widget.h>

class Button;
class QGridLayout;


/**
 * @brief Абстрактный диалог
 */
class AbstractDialog : public Widget
{
    Q_OBJECT

public:
    explicit AbstractDialog(QWidget* _parent);
    ~AbstractDialog() override;

    /**
     * @brief Отобразить диалог
     */
    void showDialog();

    /**
     * @brief Скрыть диалог
     */
    void hideDialog();

    /**
     * @brief Задать минимальную ширину контента диалога
     */
    void setContentMinimumWidth(int _width);

    /**
     * @brief Задать максимальную ширину контента диалога
     */
    void setContentMaximumWidth(int _width);

    /**
     * @brief Задать кнопку, для нажатия при нажатии Enter в диалоге
     */
    void setAcceptButton(Button* _button);

    /**
     * @brief Задать кнопку, для нажатия при нажатии Escape в диалоге
     */
    void setRejectButton(Button* _button);

protected:
    /**
     * @brief Установить заголовок диалога
     */
    void setTitle(const QString& _title);

    /**
     * @brief Получить компоновщик контента
     */
    QGridLayout* contentsLayout() const;

    /**
     * @brief Заданный виджет будет сфокусирован после завершения отображения диалога
     */
    virtual QWidget* focusedWidgetAfterShow() const = 0;

    /**
     * @brief Весим фильтр на родительский виджет, чтобы корректировать свои размеры в соответствии с ним
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Переопределяем для реализации реакции нажатия на кнопки Enter & Escape
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Переопределяем отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Обновляем навигатор при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    /**
     * @brief Скрываем эти методы, чтобы пользователь их случайно не использовал
     */
    void show();
    void hide();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
