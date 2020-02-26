#pragma once

#include <QScopedPointer>

class QEvent;
class QKeyEvent;

namespace BusinessLayer {
    enum class ScreenplayParagraphType;
}

namespace Ui {
    class ScreenplayTextEdit;
}


namespace KeyProcessingLayer
{

class AbstractKeyHandler;

/**
 * @brief Класс обработчика нажатия клавиш в текстовом редакторе
 */
class KeyPressHandlerFacade final
{
public:
    /**
     * @brief Реализуем использование класса посредством синглтона
     */
    static KeyPressHandlerFacade* instance(Ui::ScreenplayTextEdit* _editor);
    ~KeyPressHandlerFacade();

    /**
     * @brief Подготовиться к обработке
     */
    void prepare(QKeyEvent* _event);

    /**
     * @brief Предварительная обработка
     */
    void prepareForHandle(QKeyEvent* _event);

    /**
     * @brief Подготовить к обработке
     */
    void prehandle();

    /**
     * @brief Обработка
     */
    void handle(QEvent* _event, bool _pre = false);

    /**
     * @brief Нужно ли отправлять событие в базовый класс
     */
    bool needSendEventToBaseClass() const;

    /**
     * @brief Нужно ли чтобы курсор был обязательно видим пользователю
     */
    bool needEnsureCursorVisible() const;

    /**
     * @brief Нужно ли делать подготовку к обработке блока
     */
    bool needPrehandle() const;

private:
    explicit KeyPressHandlerFacade(Ui::ScreenplayTextEdit* _editor);

    /**
     * @brief Получить обработчик для заданного типа
     */
    AbstractKeyHandler* handlerFor(BusinessLayer::ScreenplayParagraphType _type);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace KeyProcessingLayer
