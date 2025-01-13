#pragma once

#include <ui/widgets/widget/widget.h>

#include <QAbstractScrollArea>
#include <QScrollBar>


namespace BusinessLayer {
class AudioplayTextModel;
}

namespace Ui {

/**
 * @brief Управляющий скролбаром сценария
 * @note Тут хитрая тема, т.к. по-сути скролбар не может иметь фиксированного размера ползунка,
 *       являя собой процентное представление возможности прокрутки документа, будем использовать
 *       ухищрение в виде невидимого скролбара, который устанавливается непосредственно в виджет
 *       и видимого таймлайна, который будет прилипать к правой стороне виджета и действовать
 *       синхронно с невидимым скролбаром для управления самим виджетом
 */
class AudioplayTextScrollBarManager : public QObject
{
    Q_OBJECT

public:
    explicit AudioplayTextScrollBarManager(QAbstractScrollArea* _parent);
    ~AudioplayTextScrollBarManager() override;

    /**
     * @brief Инициилизировать синхронизацию полос прокрутки обёртки и таймлайна
     */
    void initScrollBarsSyncing();

    /**
     * @brief Задать модель сценария
     */
    void setModel(BusinessLayer::AudioplayTextModel* _model);

    /**
     * @brief Настроить видимость скролбара
     */
    void setScrollBarVisible(bool _visible, bool _animate = true);

    /**
     * @brief Высота скролбара
     */
    int scrollBarHeight() const;

    /**
     * @brief Ловим события об изменении размера родивиджета,
     *        для коррктировки положения и размера таймлайна
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

/**
 * @brief Виджет таймлайна
 */
class AudioplayTextTimeline : public Widget
{
    Q_OBJECT

public:
    explicit AudioplayTextTimeline(QWidget* _parent = nullptr);
    ~AudioplayTextTimeline() override;

    /**
     * @brief Установить возможность прокручивания таймлайна
     */
    void setScrollable(bool _scrollable);

    /**
     * @brief Задать максимальное значение слайдера
     */
    void setMaximum(std::chrono::milliseconds _maximum);

    /**
     * @brief Задать текущее значение слайдера
     */
    void setValue(std::chrono::milliseconds _value);

    /**
     * @brief Задать размер отображаемой области
     */
    void setDisplayRange(std::chrono::milliseconds _value);

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
     * @brief Значение слайдера изменилось
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

} // namespace Ui
