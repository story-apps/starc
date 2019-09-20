#pragma once

#include <ui/widgets/widget/widget.h>

class QGridLayout;


/**
 * @brief Абстрактный диалог
 */
class AbstractDialog : public Widget
{
    Q_OBJECT

public:
    /**
     * @brief Структура для описания кнопок в диалоге
     */
    struct Button
    {
        int id;
        QString text;
    };

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

signals:
    /**
     * @brief Диалог завершился с установленным выбором пользователя
     */
    void finished(const Button& _pressedButton);

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
     * @brief Весим фильтр на родительский виджет, чтобы корректировать свои размеры в соответствии с ним
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

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
