#pragma once

#include "../widget/widget.h"


/**
 * @brief Виджет-контейнер для разделения двух вложенных виджетов и настройки их размера
 */
class CORE_LIBRARY_EXPORT Splitter : public Widget
{
    Q_OBJECT

public:
    explicit Splitter(QWidget* _parent = nullptr);
    ~Splitter() override;

    /**
     * @brief Задать доступность кнопки скрытия панели
     */
    void setHidePanelButtonAvailable(bool _available);

    /**
     * @brief Задать виджеты в контейнер
     */
    void setWidgets(QWidget* _first, QWidget* _second);

    /**
     * @brief Пропорции вложенных виджетов
     */
    QVector<int> sizes() const;
    void setSizes(const QVector<int>& _sizes);

    /**
     * @brief Сохранить текущее состояние
     */
    QByteArray saveState() const;

    /**
     * @brief Загрузить состояние
     */
    void restoreState(const QByteArray& _state);

protected:
    /**
     * @brief Переопределяем для корректировки вложенных элементов, при смене направления компоновки
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Переопределяем для пропорциональной корректировки размеров вложенных виджетов
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Реагируем на события вложенных виджетов и самого разделителя
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Обновляем тексты тултипов
     */
    void updateTranslations() override;

    /**
     * @brief Настроим внешний вид вложенных виджетов
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
