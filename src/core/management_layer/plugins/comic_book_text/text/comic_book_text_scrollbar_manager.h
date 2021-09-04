#pragma once

#include <ui/widgets/widget/widget.h>

#include <QAbstractScrollArea>
#include <QScrollBar>


namespace BusinessLayer {
class ComicBookTextModel;
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
class ComicBookTextScrollBarManager : public QObject
{
    Q_OBJECT

public:
    explicit ComicBookTextScrollBarManager(QAbstractScrollArea* _parent);
    ~ComicBookTextScrollBarManager() override;

    /**
     * @brief Инициилизировать синхронизацию полос прокрутки обёртки и таймлайна
     */
    void initScrollBarsSyncing();

    /**
     * @brief Задать модель сценария
     */
    void setModel(BusinessLayer::ComicBookTextModel* _model);

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
class ComicBookTextTimeline : public Widget
{
    Q_OBJECT

public:
    explicit ComicBookTextTimeline(QWidget* _parent = nullptr);
    ~ComicBookTextTimeline() override;

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
     * @brief Задать цвета слайдера
     */
    void setColors(const std::map<std::chrono::milliseconds, QColor>& _colors);

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
