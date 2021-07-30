#pragma once

#include <ui/widgets/widget/widget.h>

#include <QPageSize>


namespace Ui {

/**
 * @brief Представление настроек страницы
 */
class ScreenplayTemplatePageView : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayTemplatePageView(QWidget* _parent = nullptr);
    ~ScreenplayTemplatePageView() override;

    /**
     * @brief Использовать миллиметры (true) ли дюймы (false) для отображения параметров
     */
    void setUseMm(bool _mm);

    //
    // Параметры шаблона
    //
    QString templateName() const;
    void setTemplateName(const QString& _name);
    QPageSize::PageSizeId pageSizeId() const;
    void setPageSize(QPageSize::PageSizeId _pageSize);
    QMarginsF pageMargins() const;
    void setPageMargins(const QMarginsF& _margins);
    Qt::Alignment pageNumbersAlignment() const;
    void setPageNumbersAlignment(Qt::Alignment _alignment);
    int leftHalfOfPageWidthPercents() const;
    void setLeftHalfOfPage(int _value);

protected:
    /**
     * @brief Наблюдаем за событиями фокусировки дочерних виджетов
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
