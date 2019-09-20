#pragma once

#include <QWidget>

class QGridLayout;


/**
 * @brief Абстрактный диалог
 */
class AbstractDialog : public QWidget
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
    explicit AbstractDialog(QWidget* _parent = nullptr);
    ~AbstractDialog() override;

    /**
     * @brief Отобразить диалог
     */
    void showDialog();

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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
