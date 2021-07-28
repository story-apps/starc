#pragma once

#include <ui/widgets/widget/widget.h>

#include <QPageSize>


/**
 * @brief Элемент параметров страницы
 */
enum class PageLayoutItem {
    None,
    LeftMargin,
    TopMargin,
    RightMargin,
    BottomMargin,
    Splitter,
    PageNumber,
};

/**
 * @brief Виджет для визуального отображения параметров страницы текста
 */
class CORE_LIBRARY_EXPORT PageLayout : public Widget
{
    Q_OBJECT

public:
    explicit PageLayout(QWidget* _parent = nullptr);
    ~PageLayout() override;

    /**
     * @brief Задать размер страницы
     */
    void setPageSize(QPageSize::PageSizeId _pageSize);

    /**
     * @brief Задать отступы, мм
     */
    void setMargins(const QMarginsF& _margins);

    /**
     * @brief Задать расположение номеров страниц
     */
    void setPageNumberAlignment(Qt::Alignment _alignment);

    /**
     * @brief Задать разделение страницы, %
     * @note Если линия разделения не нужна, то нужно задать разделитель как NaN
     */
    void setPageSplitter(qreal _splitAt);

    /**
     * @brief Задать текущий/выделенный элемент на странице
     */
    void setCurrentItem(PageLayoutItem _item);

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
