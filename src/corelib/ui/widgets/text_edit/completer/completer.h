#pragma once

#include <QCompleter>

#include <corelib_global.h>

class QAbstractItemDelegate;


/**
 * @brief Переопределяем комплитер, чтобы показывать список красиво
 */
class CORE_LIBRARY_EXPORT Completer : public QCompleter
{
public:
    explicit Completer(QWidget* _parent = nullptr);
    ~Completer() override;

    /**
     * @brief Цвет фона попапа
     */
    void setBackgroundColor(const QColor& _color);

    /**
     * @brief Цвет текста попапа
     */
    void setTextColor(const QColor& _color);

    /**
     * @brief Задать делегат для отрисовки элементов выпадающего списка
     */
    void setItemDelegate(QAbstractItemDelegate* _delegate);

    /**
     * @brief Переопределяется для отображения подсказки по глобальной координате
     *		  левого верхнего угла области для отображения
     */
    void showCompleter(const QRect& _rect);

    /**
     * @brief Закрыть список автодополнения
     */
    void closeCompleter();

protected:
    /**
     * @brief Переопределяем, чтобы отлавливать события попапа
     */
    bool eventFilter(QObject* _target, QEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
