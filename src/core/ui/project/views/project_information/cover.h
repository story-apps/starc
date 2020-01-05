#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

/**
 * @brief Виджет для отображения обложки проекта
 */
class Cover : public Widget
{
    Q_OBJECT

public:
    explicit Cover(QWidget* _parent = nullptr);
    ~Cover() override;

    /**
     * @brief Задать обложку
     */
    void setCover(const QPixmap& _cover);

signals:
    /**
     * @brief Пользователь кликнул на обложке
     */
    void clicked();

protected:
    /**
     * @brief Реализуем отрисовку
     */
    void paintEvent(QPaintEvent *_event) override;

    /**
     * @brief Реализуем эффекст отображения оверлея при наведении мыши
     */
    void enterEvent(QEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

    /**
     * @brief Испускаем сигнал о клике пользователя
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Определяем переводы
     */
    void updateTranslations() override;

    /**
     * @brief Кореектируем размер постера
     */
    void designSystemChangeEvent(DesignSystemChangeEvent *_event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
