#pragma once

#include <ui/widgets/widget/widget.h>

#include <QAbstractScrollArea>
#include <QScrollBar>

#include <corelib_global.h>


namespace BusinessLayer {
class ScreenplayTextModel;
class TextModelItem;
} // namespace BusinessLayer

namespace Ui {

/**
 * @brief Тип полосы прокрутки
 */
enum class CORE_LIBRARY_EXPORT ScrollBarType {
    Timeline,
    Pageline,
};

/**
 * @brief Тип выделения области на полосе прокрутки
 */
enum class CORE_LIBRARY_EXPORT ScrollBarMode {
    Scroll, //!< Основан на прокрутке оригинального документа и его отображаемой области
    Range, //!< Пользовательское выделение
};

/**
 * @brief Управляющий таймлайном сценария
 * @note Тут хитрая тема, т.к. по-сути скролбар не может иметь фиксированного размера ползунка,
 *       являя собой процентное представление возможности прокрутки документа, будем использовать
 *       ухищрение в виде невидимого скролбара, который устанавливается непосредственно в виджет
 *       и видимого таймлайна, который будет прилипать к правой стороне виджета и действовать
 *       синхронно со скролбаром для управления самим виджетом
 */
class CORE_LIBRARY_EXPORT ScreenplayTextScrollBarManager : public QObject
{
    Q_OBJECT

public:
    explicit ScreenplayTextScrollBarManager(QAbstractScrollArea* _parent);
    ~ScreenplayTextScrollBarManager() override;

    /**
     * @brief Инициилизировать синхронизацию полос прокрутки обёртки и таймлайна
     */
    void initScrollBarsSyncing();

    /**
     * @brief Задать модель сценария
     */
    void setModel(BusinessLayer::ScreenplayTextModel* _model);

    /**
     * @brief Задать тип полосы прокрутки (true - время, false - страницы)
     */
    void setScrollBarType(ScrollBarType _type);

    /**
     * @brief Настроить видимость скролбара
     */
    void setScrollBarVisible(bool _visible, bool _animate = true);

    /**
     * @brief Задать режим работы полосы прокрутки
     */
    void setScrollBarMode(ScrollBarMode _mode);

    /**
     * @brief Задать выделенный элемент
     */
    void selectItem(BusinessLayer::TextModelItem* _item);

    /**
     * @brief Высота скролбара
     */
    int scrollBarHeight() const;

    /**
     * @brief Ловим события об изменении размера родивиджета,
     *        для коррктировки положения и размера таймлайна
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

signals:
    /**
     * @brief Пользователь нажал на заданную сцену
     */
    void itemClicked(const QModelIndex& _index);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

/**
 * @brief Виджет таймлайна
 */
class ScreenplayTextTimeline : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayTextTimeline(QWidget* _parent = nullptr);
    ~ScreenplayTextTimeline() override;

    /**
     * @brief Установить возможность прокручивания таймлайна
     */
    void setScrollable(bool _scrollable);

    /**
     * @brief Задать максимальное значение слайдера
     */
    void setMaximum(std::chrono::milliseconds _maximum);

    /**
     * @brief Задать позицию прокрутки от 0 до maximum (не начало отрезка)
     */
    void setValue(std::chrono::milliseconds _value);

    /**
     * @brief Задать размер отображаемой области
     */
    void setDisplayRange(std::chrono::milliseconds _value);

    /**
     * @brief Показать абсолютный отрезок без преобразования позиции прокрутки.
     *        Длина ограничивается [0, maximum], начало — [0, maximum - длина].
     *        Не изменяет возможность взаимодействия и не испускает valueChanged().
     *        setValue() возвращает режим позиции прокрутки.
     */
    void setRange(std::chrono::milliseconds _start, std::chrono::milliseconds _length);

    /**
     * @brief Задать цвета слайдера
     */
    void setColors(const std::map<std::chrono::milliseconds, QColor>& _colors);

    /**
     * @brief Задать цвета закладок
     */
    void setBookmarks(const std::map<std::chrono::milliseconds, QColor>& _bookmarks);

    /**
     * @brief Переопределяем для корректного подсчёта размера в компоновщиках
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Абсолютная позиция нажатия левой кнопки мыши в миллисекундах.
     *        Работает и при setScrollable(false), не зависит от выделенного отрезка.
     *        Не испускается при перетаскивании, программных изменениях и вне шкалы.
     */
    void positionClicked(std::chrono::milliseconds _position);

    /**
     * @brief Значение слайдера изменилось при взаимодействии с пользователем или setValue().
     *        В режиме setRange() передаётся абсолютное начало отрезка.
     */
    void valueChanged(std::chrono::milliseconds _value);

    /**
     * @brief Запрос на обновление значения ползунка
     */
    void updateValueRequested();

protected:
    /**
     * @brief Переопределяем для собственной отрисовки
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для установки позиции ползунка в месте указанном пользователем
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем для обработки изменения позиции ползунка
     */
    void mouseMoveEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем для определения момента отпускания мыши и скрытия области клика
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Прокидываем событие прокрутки в виджет, к которому привязан таймлайн, чтобы не
     * обрабатывать вручную
     */
    void wheelEvent(QWheelEvent* _event) override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    /**
     * @brief Обновить значение в соответствии с позицией мыши
     */
    void updateValue(const QPoint& _mousePosition);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

/**
 * @brief Виджет пейджлайна
 */
class ScreenplayTextPageline : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayTextPageline(QWidget* _parent = nullptr);
    ~ScreenplayTextPageline() override;

    /**
     * @brief Установить возможность прокручивания таймлайна
     */
    void setScrollable(bool _scrollable);

    /**
     * @brief Задать максимальное значение слайдера
     */
    void setMaximum(qreal _maximum);

    /**
     * @brief Задать позицию прокрутки от 0 до maximum (не начало отрезка)
     */
    void setValue(qreal _value);

    /**
     * @brief Задать размер отображаемой области
     */
    void setDisplayRange(qreal _value);

    /**
     * @brief Показать абсолютный отрезок без преобразования позиции прокрутки.
     *        Длина ограничивается [0, maximum], начало — [0, maximum - длина].
     *        Не изменяет возможность взаимодействия и не испускает valueChanged().
     *        setValue() возвращает режим позиции прокрутки.
     *        Начало, длина и maximum задаются в восьмых долях страницы.
     */
    void setRange(qreal _start, qreal _length);

    /**
     * @brief Задать цвета слайдера
     */
    void setColors(const std::map<qreal, QColor>& _colors);

    /**
     * @brief Задать цвета закладок
     */
    void setBookmarks(const std::map<qreal, QColor>& _bookmarks);

    /**
     * @brief Переопределяем для корректного подсчёта размера в компоновщиках
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Абсолютная позиция нажатия левой кнопки мыши в восьмых долях страницы.
     * Работает и при setScrollable(false), не зависит от выделенного отрезка.
     * Не испускается при перетаскивании, программных изменениях и вне шкалы.
     */
    void positionClicked(qreal _position);

    /**
     * @brief Значение слайдера изменилось при взаимодействии с пользователем или setValue().
     * В режиме setRange() передаётся абсолютное начало отрезка.
     */
    void valueChanged(qreal _value);

    /**
     * @brief Запрос на обновление значения ползунка
     */
    void updateValueRequested();

protected:
    /**
     * @brief Переопределяем для собственной отрисовки
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для установки позиции ползунка в месте указанном пользователем
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем для обработки изменения позиции ползунка
     */
    void mouseMoveEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяем для определения момента отпускания мыши и скрытия области клика
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Прокидываем событие прокрутки в виджет, к которому привязан таймлайн, чтобы не
     * обрабатывать вручную
     */
    void wheelEvent(QWheelEvent* _event) override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    /**
     * @brief Обновить значение в соответствии с позицией мыши
     */
    void updateValue(const QPoint& _mousePosition);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
