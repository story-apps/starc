#pragma once

#include <corelib_global.h>

#include <QWidget>

class DesignSystemChangeEvent;


/**
 * @brief Базовый виджет
 */
class CORE_LIBRARY_EXPORT Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget* _parent = nullptr);
    ~Widget() override;

    /**
     * @brief Цвет фона виджета
     */
    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& _color);

    /**
     * @brief Цвет текста виджета
     */
    QColor textColor() const;
    void setTextColor(const QColor& _color);

    /**
     * @brief Переопределяем для испускания сигналов моментах, когда виджет стал показан или скрыт
     */
    void setVisible(bool _visible) override;

signals:
    /**
     * @brief Виджет был показан
     */
    void appeared();

    /**
     * @brief Виджет был скрыт
     */
    void disappeared();

protected:
    /**
     * @brief Интерфейс для наследников, чтобы можно было отреагировать на смену цвета
     */
    virtual void processBackgroundColorChange() {}
    virtual void processTextColorChange() {}

    /**
     * @brief Переопределяем для обработки события обновления дизайн системы
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Переопределяем, чтобы обратать событие смены языка приложения
     */
    void changeEvent(QEvent* _event) override;

    /**
     * @brief Предоставляем интерфейс для наследников, чтобы обновить перевод
     */
    virtual void updateTranslations() {}

    /**
     * @brief Переопределяем для собственной реализации отрисовки - по сути заливаем цветом фона
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Событие реакция на изменение дизайн системы
     * @note Реализация по-умолчание просто обновляет весь виджет
     */
    virtual void designSystemChangeEvent(DesignSystemChangeEvent* _event);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
