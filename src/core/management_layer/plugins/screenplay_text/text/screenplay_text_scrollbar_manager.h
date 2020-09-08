#pragma once

#include <ui/widgets/widget/widget.h>

#include <QAbstractScrollArea>
#include <QScrollBar>


namespace BusinessLayer {
    class ScreenplayTextModel;
}

namespace Ui
{

/**
 * @brief Управляющий скролбаром сценария
 * @note Тут хитрая тема, т.к. по-сути скролбар не может иметь фиксированного размера ползунка,
 *       являя собой процентное представление возможности прокрутки документа, будем использовать
 *       ухищрение в виде невидимого скролбара, который устанавливается непосредственно в виджет
 *       и видимого таймлайна, который будет прилипать к правой стороне виджета и действовать
 *       синхронно с невидимым скролбаром для управления самим виджетом
 */
class ScreenplayTextScrollBarManager : public QObject
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
    void setMaximum(std::chrono::seconds _maximum);

    /**
     * @brief Задать текущее значение слайдера
     */
    void setValue(std::chrono::milliseconds _value);

    /**
     * @brief Переопределяем для корректного подсчёта размера в компоновщиках
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Значение слайдера изменилось
     */
    void valueChanged(std::chrono::milliseconds _value);

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
     * @brief Прокидываем событие прокрутки в виджет, к которому привязан таймлайн, чтобы не обрабатывать вручную
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
