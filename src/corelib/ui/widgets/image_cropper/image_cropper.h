#pragma once

#include <ui/widgets/widget/widget.h>


/**
 * @brief Виджет обрезчика изображения
 */
class ImageCropper : public Widget
{
    Q_OBJECT

public:
    ImageCropper(QWidget* _parent = nullptr);
    ~ImageCropper() override;

    /**
     * @brief Установить изображение для обрезки
     */
    void setImage(const QPixmap& _image);

    /**
     * @brief Установить пропорции области выделения
     */
    void setProportion(const QSizeF& _proportion);

    /**
     * @brief Использовать фиксированные пропорции области виделения
     */
    void setProportionFixed(const bool _isFixed);

    /**
     * @brief Получить обрезанное изображение
     */
    const QPixmap croppedImage();

    /**
     * @brief Переопределяем для собственной реализации
     */
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* _event) override;
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseMoveEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
