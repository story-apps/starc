#pragma once

#include <ui/widgets/widget/widget.h>


class CORE_LIBRARY_EXPORT Card : public Widget
{
    Q_OBJECT

public:
    explicit Card(QWidget* _parent = nullptr);
    ~Card() override;

    /**
     * @brief Собственная реализация метода установки компоновщика
     */
    void setLayoutReimpl(QLayout* _layout) const;

protected:
    /**
     * @brief Переопределяем для реализации отрисовки
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для реализации эффекта поднятия виджета при ховере
     */
    void enterEvent(QEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

    /**
     * @brief Переопределяем для настройки отступов лейаута
     */
    void designSystemChangeEvent(DesignSystemChangeEvent *_event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
