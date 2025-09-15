#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>


namespace Ui {

class ModulePromoToolbar : public FloatingToolBar
{
    Q_OBJECT

public:
    explicit ModulePromoToolbar(QWidget* _parent = nullptr);
    ~ModulePromoToolbar() override;

    /**
     * @brief Задать заголовок
     */
    void setTitle(const QString& _title);

    /**
     * @brief Задать подзаголовок
     */
    void setSubtitle(const QString& _subtitle);

    /**
     * @brief Задать видимость кнопки возврата
     */
    void setGoBackButtonVisible(bool _visible);

    /**
     * @brief Задать иконку кнопки возврата
     */
    void setGoBackButtonIcon(const QString& _icon);

    /**
     * @brief Задать текст кнопки возврата
     */
    void setGoBackButtonText(const QString& _text);

    /**
     * @brief Задать текст кнопки покупки
     */
    void setPuchaseButtonText(const QString& _text);

    /**
     * @brief Задать позицию относительно которой будет двигаться панель
     */
    void setOriginalPos(const QPoint& _pos);

    /**
     * @brief Переопределяем для лучшего подсчёта размера виджета
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Пользователь нажал кнопку возврата назад
     */
    void goBackPressed();

    /**
     * @brief Пользователь нажал кнопку покупки
     */
    void purchasePressed();

protected:
    /**
     * @brief Настроим цвета зависимые от цвета фона и текста
     */
    void processBackgroundColorChange() override;
    void processTextColorChange() override;

    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Переопределяем для анимирования ширины/высоты
     */
#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
    void enterEvent(QEnterEvent* _event) override;
#else
    void enterEvent(QEvent* _event) override;
#endif
    void leaveEvent(QEvent* _event) override;

    /**
     * @brief Переопределяем для обработки события смены дизайн-системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
