#pragma once

#include "../widget/widget.h"


/**
 * @brief Виджет вкладок
 */
class CORE_LIBRARY_EXPORT TabBar : public Widget
{
    Q_OBJECT

public:
    explicit TabBar(QWidget* _parent = nullptr);
    ~TabBar() override;

    /**
     * @brief Установить необходимость фиксации вкладок (так что они занимают всю ширину)
     */
    void setFixed(bool fixed);

    /**
     * @brief Добавить вкладку с заданным названием
     */
    void addTab(const QString& _tabName);

    /**
     * @brief Задать название вкладки
     */
    void setTabName(int _tabIndex, const QString& _tabName);

    /**
     * @brief Задать видимость вкладки
     */
    void setTabVisible(int _tabIndex, bool _visible);

    /**
     * @brief Задать индекс текущей вкладки
     */
    void setCurrentTab(int _index);

    /**
     * @brief Получить текущую вкладку
     */
    int currentTab() const;

    /**
     * @brief Вычисляем идеальный размер в зависимости от контента
     */
    /** @{ */
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
    /** @} */

signals:
    /**
     * @brief Изменилась текущая вкладка
     */
    void currentIndexChanged(int _currentIndex, int _previousIndex);

protected:
    /**
     * @brief Переопределяем для обработки событий обновления дизайн системы и прокручивания
     */
    bool event(QEvent* _event) override;

    /**
     * @brief Переопределяем для корректировки области вьюпорта
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Собственная реализация рисования
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Обновляем виджет для корректного отображения ховер элемента
     */
    void enterEvent(QEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

    /**
     * @brief Обновление отрисовки при смене элемента на который наведена мышь
     */
    void mouseMoveEvent(QMouseEvent* _event) override;

    /**
     * @brief Запускаем анимацию клика на табе
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Смена активной вкладки при клике мышкой
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
